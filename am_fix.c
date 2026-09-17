
/* Copyright 2023 OneOfEleven
 * https://github.com/DualTachyon
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

// code to 'try' and reduce the AM demodulator saturation problem
//
// that is until someone works out how to properly configure the BK chip !

#include <string.h>

#include "am_fix.h"
#include "app/main.h"
#include "board.h"
#include "driver/bk4819.h"
#include "external/printf/printf.h"
#include "frequencies.h"
#include "functions.h"
#include "misc.h"
#include "settings.h"
#ifdef ENABLE_AGC_SHOW_DATA
#include "ui/main.h"
#endif

#ifdef ENABLE_AM_FIX

typedef struct
{
	uint16_t reg_val;
	int8_t   gain_dB;
} __attribute__((packed)) t_gain_table;

// REG_10 AGC gain table
//
// <15:10> ???
//
//   <9:8> = LNA Gain Short
//           3 =   0dB   < original value
//           2 = -19dB   // was -11
//           1 = -24dB   // was -16
//           0 = -28dB   // was -19
//
//   <7:5> = LNA Gain
//           7 =   0dB
//           6 =  -2dB
//           5 =  -4dB   < original value
//           4 =  -6dB
//           3 =  -9dB
//           2 = -14dB
//           1 = -19dB
//           0 = -24dB
//
//   <4:3> = MIXER Gain
//           3 =   0dB   < original value
//           2 =  -3dB
//           1 =  -6dB
//           0 =  -8dB
//
//   <2:0> = PGA Gain
//           7 =   0dB
//           6 =  -3dB   < original value
//           5 =  -6dB
//           4 =  -9dB
//           3 = -15dB
//           2 = -21dB
//           1 = -27dB
//           0 = -33dB

// front end register dB values
//
// these values need to be accurate for the code to properly/reliably switch
// between table entries when adjusting the front end registers.
//
// these 4 tables need a measuring/calibration update
//
////	static const int16_t lna_short_dB[] = {  -19,   -16,   -11,     0};   // was (but wrong)
//	static const int16_t lna_short_dB[] = { (-28), (-24), (-19),    0};   // corrected'ish
//	static const int16_t lna_dB[]       = { (-24), (-19), (-14), ( -9), (-6), (-4), (-2), 0};
//	static const int16_t mixer_dB[]     = { ( -8), ( -6), ( -3),    0};
//	static const int16_t pga_dB[]       = { (-33), (-27), (-21), (-15), (-9), (-6), (-3), 0};

// lookup table is hugely easier than writing code to do the same
//

#define LOOKUP_TABLE 1

#if LOOKUP_TABLE
static const t_gain_table gain_table[] =
{
	{0x03BE, -7},   //  0 .. 3 5 3 6 ..   0dB  -4dB  0dB  -3dB ..  -7dB original

	{0x0000,-93},   //  1 .. 0 0 0 0 .. -28dB -24dB -8dB -33dB .. -93dB
	{0x0008,-91},   //  2 .. 0 0 1 0 .. -28dB -24dB -6dB -33dB .. -91dB
	{0x0010,-88},   //  3 .. 0 0 2 0 .. -28dB -24dB -3dB -33dB .. -88dB
	{0x0001,-87},   //  4 .. 0 0 0 1 .. -28dB -24dB -8dB -27dB .. -87dB
	{0x0009,-85},   //  5 .. 0 0 1 1 .. -28dB -24dB -6dB -27dB .. -85dB
	{0x0011,-82},   //  6 .. 0 0 2 1 .. -28dB -24dB -3dB -27dB .. -82dB
	{0x0002,-81},   //  7 .. 0 0 0 2 .. -28dB -24dB -8dB -21dB .. -81dB
	{0x000A,-79},   //  8 .. 0 0 1 2 .. -28dB -24dB -6dB -21dB .. -79dB
	{0x0012,-76},   //  9 .. 0 0 2 2 .. -28dB -24dB -3dB -21dB .. -76dB
	{0x0003,-75},   // 10 .. 0 0 0 3 .. -28dB -24dB -8dB -15dB .. -75dB
	{0x000B,-73},   // 11 .. 0 0 1 3 .. -28dB -24dB -6dB -15dB .. -73dB
	{0x0013,-70},   // 12 .. 0 0 2 3 .. -28dB -24dB -3dB -15dB .. -70dB
	{0x0004,-69},   // 13 .. 0 0 0 4 .. -28dB -24dB -8dB  -9dB .. -69dB
	{0x000C,-67},   // 14 .. 0 0 1 4 .. -28dB -24dB -6dB  -9dB .. -67dB
	{0x000D,-64},   // 15 .. 0 0 1 5 .. -28dB -24dB -6dB  -6dB .. -64dB
	{0x001C,-61},   // 16 .. 0 0 3 4 .. -28dB -24dB  0dB - 9dB .. -61dB
	{0x001D,-58},   // 17 .. 0 0 3 5 .. -28dB -24dB  0dB  -6dB .. -58dB
	{0x001E,-55},   // 18 .. 0 0 3 6 .. -28dB -24dB  0dB  -3dB .. -55dB
	{0x001F,-52},   // 19 .. 0 0 3 7 .. -28dB -24dB  0dB   0dB .. -52dB
	{0x003E,-50},   // 20 .. 0 1 3 6 .. -28dB -19dB  0dB  -3dB .. -50dB
	{0x003F,-47},   // 21 .. 0 1 3 7 .. -28dB -19dB  0dB   0dB .. -47dB
	{0x005E,-45},   // 22 .. 0 2 3 6 .. -28dB -14dB  0dB  -3dB .. -45dB
	{0x005F,-42},   // 23 .. 0 2 3 7 .. -28dB -14dB  0dB   0dB .. -42dB
	{0x007E,-40},   // 24 .. 0 3 3 6 .. -28dB  -9dB  0dB  -3dB .. -40dB
	{0x007F,-37},   // 25 .. 0 3 3 7 .. -28dB  -9dB  0dB   0dB .. -37dB
	{0x009F,-34},   // 26 .. 0 4 3 7 .. -28dB  -6dB  0dB   0dB .. -34dB
	{0x00BF,-32},   // 27 .. 0 5 3 7 .. -28dB  -4dB  0dB   0dB .. -32dB
	{0x00DF,-30},   // 28 .. 0 6 3 7 .. -28dB  -2dB  0dB   0dB .. -30dB
	{0x00FF,-28},   // 29 .. 0 7 3 7 .. -28dB   0dB  0dB   0dB .. -28dB
	{0x01DF,-26},   // 30 .. 1 6 3 7 .. -24dB  -2dB  0dB   0dB .. -26dB
	{0x01FF,-24},   // 31 .. 1 7 3 7 .. -24dB   0dB  0dB   0dB .. -24dB
	{0x02BF,-23},   // 32 .. 2 5 3 7 .. -19dB  -4dB  0dB   0dB .. -23dB
	{0x02DF,-21},   // 33 .. 2 6 3 7 .. -19dB  -2dB  0dB  -0dB .. -21dB
	{0x02FF,-19},   // 34 .. 2 7 3 7 .. -19dB   0dB  0dB   0dB .. -19dB
	{0x035E,-17},   // 35 .. 3 2 3 6 ..   0dB -14dB  0dB  -3dB .. -17dB
	{0x035F,-14},   // 36 .. 3 2 3 7 ..   0dB -14dB  0dB   0dB .. -14dB
	{0x037E,-12},   // 37 .. 3 3 3 6 ..   0dB  -9dB  0dB  -3dB .. -12dB
	{0x037F,-9},    // 38 .. 3 3 3 7 ..   0dB  -9dB  0dB   0dB ..  -9dB
	{0x038F,-6},    // 39 .. 3 4 3 7 ..   0dB - 6dB  0dB   0dB ..  -6dB
	{0x03BF,-4},    // 40 .. 3 5 3 7 ..   0dB  -4dB  0dB   0dB ..  -4dB
	{0x03DF,-2},    // 41 .. 3 6 3 7 ..   0dB - 2dB  0dB   0dB ..  -2dB
	{0x03FF,0}      // 42 .. 3 7 3 7 ..   0dB   0dB  0dB   0dB ..   0dB
};

const uint8_t gain_table_size = ARRAY_SIZE(gain_table);
#else

t_gain_table gain_table[100] = {{0x03BE, -7}}; //original
uint8_t gain_table_size = 0;

void CreateTable()
{
typedef union  {
    struct {
        uint8_t pgaIdx:3;
        uint8_t mixerIdx:2;
        uint8_t lnaIdx:3;
        uint8_t lnaSIdx:2;
    };
    uint16_t __raw;
} GainData;

	static const int8_t lna_short_dB[] = {-28, -24, -19,  0};   // corrected'ish
	static const int8_t lna_dB[]       = {-24, -19, -14,  -9, -6, -4, -2, 0};
	static const int8_t mixer_dB[]     = { -8,  -6,  -3,   0};
	static const int8_t pga_dB[]       = {-33, -27, -21, -15, -9, -6, -3, 0};

	unsigned i;
    for (uint8_t lnaSIdx = 0; lnaSIdx < ARRAY_SIZE(lna_short_dB); lnaSIdx++) {
        for (uint8_t lnaIdx = 0; lnaIdx < ARRAY_SIZE(lna_dB); lnaIdx++) {
            for (uint8_t mixerIdx = 0; mixerIdx < ARRAY_SIZE(mixer_dB); mixerIdx++) {
                for (uint8_t pgaIdx = 0; pgaIdx < ARRAY_SIZE(pga_dB); pgaIdx++) {
                    int16_t db = lna_short_dB[lnaSIdx] + lna_dB[lnaIdx] + mixer_dB[mixerIdx] + pga_dB[pgaIdx];
                    GainData gainData = {{
                        pgaIdx,
                        mixerIdx,
                        lnaIdx,
                        lnaSIdx,
                    }};

                    for (i = 1; i < ARRAY_SIZE(gain_table); i++) {
                        t_gain_table * gain = &gain_table[i];
                        if (db == gain->gain_dB)
                            break;
                        if (db > gain->gain_dB)
                            continue;
                        if (db < gain->gain_dB) {
                            if(gain->gain_dB)
                                memmove(gain + 1, gain, 100 - i);
                            gain->gain_dB = db;
                            gain->reg_val = gainData.__raw;
                            break;
                        }
                        gain->gain_dB = db;
                        gain->reg_val = gainData.__raw;
                        break;
                    }
                }
            }
        }
    }

    gain_table_size = i+1;
}
#endif

// ---------------------------------------------------------------------------
// AGC tuning constants
//
// The BK4819 has no usable AM AGC, so this module drives the front end gain
// register (REG_13) itself. The thing that makes AM different from FM here is
// that on a modulated AM carrier the RSSI *is* the audio envelope - so a loop
// that follows the instantaneous RSSI ends up modulating the gain at audio
// rate, which is heard as pumping and distortion.
//
// Two properties avoid that:
//   1. the control variable is the PEAK RSSI over a short window, not the
//      instantaneous sample, so the audio envelope never reaches the loop;
//   2. attack and decay are asymmetric - reduce gain fast (overload has to be
//      corrected immediately), restore it slowly.

// how far back the peak detector looks, in 10ms ticks.
// 8 ticks = 80ms, which spans the whole AM audio band with room to spare.
#define AM_FIX_PEAK_WINDOW      8

// hold-off before gain is allowed to increase again, in 10ms ticks
#define AM_FIX_HOLD_TICKS      30   // 300ms

// short settle after a retune, so a scan step does not start ramping instantly
#define AM_FIX_SETTLE_TICKS     5   // 50ms

// deadband around the target, in dB. The loop does nothing inside this window,
// which is what stops it hunting around the target.
#define AM_FIX_MARGIN_HI        0   // dB above target before gain is reduced
#define AM_FIX_MARGIN_LO        8   // dB below target before gain is increased

// above this error we jump straight to a new table entry instead of stepping
#define AM_FIX_FAST_JUMP_DB    10
// ... but stay this far away from the target, for noise/spike immunity
#define AM_FIX_JUMP_GUARD_DB    8

// ticks between single gain steps on the way up, indexed by gSetting_AM_speed.
// Table steps average 2-3dB, so these are roughly 60, 30 and 15 dB/s.
static const uint8_t decay_ticks[] = {4, 8, 16};

#ifdef ENABLE_AM_FIX_SHOW_DATA
	// display update rate
	static const unsigned int display_update_rate = 250 / 10;   // max 250ms display update rate
	unsigned int counter = 0;
#endif

unsigned int gain_table_index[2] = {0, 0};
// used simply to detect a changed gain setting
unsigned int gain_table_index_prev[2] = {0, 0};
// holds the most recent RSSI reading (for the debug display)
int16_t prev_rssi[2] = {0, 0};
// to help reduce gain hunting, peak hold count down tick
unsigned int hold_counter[2] = {0, 0};

// sliding window of raw RSSI samples, used as a peak detector
static int16_t  rssi_hist[2][AM_FIX_PEAK_WINDOW];
static uint8_t  rssi_hist_pos[2];
static uint8_t  rssi_hist_len[2];

// counts ticks between gain increases
static uint8_t  decay_counter[2];

// the gain entry we start from - the one closest to the stock QS setting
static unsigned int start_index = 1;

// the un-offset squelch RSSI thresholds, as programmed by BK4819_SetupSquelch
static uint8_t squelch_base_open;
static uint8_t squelch_base_close;
static bool    squelch_base_valid;
// the offset currently programmed, so we don't rewrite REG_78 needlessly
static int16_t squelch_shift_applied;

int8_t currentGainDiff;
bool enabled = true;

// the level we try to hold the carrier at, in the BK4819's raw RSSI units
// (0.5dB/step, -160dBm offset). Anything higher and the AM demodulator starts
// to saturate/clip/distort.
static int16_t AM_fix_desired_rssi(void)
{
	int16_t dBm = AM_FIX_TARGET_DBM_MIN + (int16_t)gSetting_AM_target;

	if (dBm < AM_FIX_TARGET_DBM_MIN || dBm > AM_FIX_TARGET_DBM_MAX)
		dBm = AM_FIX_TARGET_DBM_DEFAULT;

	return (int16_t)((dBm + 160) * 2);
}

// shift the squelch comparator by however much front end gain we have taken away.
//
// REG_78 works on post-gain RSSI, so without this every gain step would move the
// squelch reference underneath the comparator and the squelch would chatter in
// step with the AGC - which is exactly what makes weak AM signals drop out.
static void AM_fix_apply_squelch_shift(void)
{
	if (!squelch_base_valid)
		return;

	// currentGainDiff is positive when we have REMOVED gain relative to the stock
	// entry, so the chip sees a correspondingly lower RSSI. REG_78 is 0.5dB/step.
	const int16_t shift = (int16_t)currentGainDiff * 2;

	if (shift == squelch_shift_applied)
		return;
	squelch_shift_applied = shift;

	int16_t open  = (int16_t)squelch_base_open  - shift;
	int16_t close = (int16_t)squelch_base_close - shift;

	open  = (open  <   0) ?   0 : (open  > 255) ? 255 : open;
	close = (close <   0) ?   0 : (close > 255) ? 255 : close;

	BK4819_SetSquelchRSSIThresholds((uint8_t)open, (uint8_t)close);
}

void AM_fix_set_squelch_base(uint8_t open, uint8_t close)
{	// called by RADIO_SetupRegisters right after it programs the squelch
	squelch_base_open     = open;
	squelch_base_close    = close;
	squelch_base_valid    = true;
	squelch_shift_applied = 0;

	if (enabled)
	{	// re-apply our offset on top of the freshly programmed thresholds
		AM_fix_apply_squelch_shift();
	}
}

void AM_fix_init(void)
{	// called at boot-up

	// find the table entry closest to the stock QS gain. Entry 0 is the stock
	// setting but sits outside the monotonic 1..N-1 ordering the loop walks, so
	// starting the loop at 0 would make its first downward step an ~86dB cliff.
	{
		const int8_t stock_dB = gain_table[0].gain_dB;
		unsigned int best     = 1;
		int16_t      best_err = 32767;

		for (unsigned int i = 1; i < gain_table_size; i++)
		{
			int16_t err = (int16_t)gain_table[i].gain_dB - stock_dB;
			if (err < 0)
				err = -err;
			if (err < best_err)
			{
				best_err = err;
				best     = i;
			}
		}
		start_index = best;
	}

	for (int i = 0; i < 2; i++) {
		gain_table_index[i]      = start_index;
		gain_table_index_prev[i] = 0xFFFF;   // force the first register write
		rssi_hist_pos[i]         = 0;
		rssi_hist_len[i]         = 0;
		decay_counter[i]         = 0;
		hold_counter[i]          = 0;
		prev_rssi[i]             = 0;
	}

	currentGainDiff = 0;
#if !LOOKUP_TABLE
	CreateTable();
#endif
}

void AM_fix_reset(const unsigned vfo)
{	// reset the AM fixer upper
	if (vfo > 1)
		return;

	#ifdef ENABLE_AM_FIX_SHOW_DATA
		counter = 0;
	#endif

	prev_rssi[vfo]     = 0;
	decay_counter[vfo] = 0;

	// drop the peak history - it belongs to the old frequency
	rssi_hist_pos[vfo] = 0;
	rssi_hist_len[vfo] = 0;

	// deliberately keep gain_table_index: on a scan step the neighbouring channel
	// is usually at a similar level, and re-converging from scratch takes longer
	// than the scan dwell. Just hold briefly while the peak window refills.
	hold_counter[vfo] = AM_FIX_SETTLE_TICKS;
}

// adjust the RX gain to try and prevent the AM demodulator from
// saturating/overloading/clipping (distorted AM audio)
//
// we're actually doing the BK4819's job for it here, but as the chip
// won't/don't do it for itself, we're left to bodging it ourself by
// playing with the RF front end gain setting
//
void AM_fix_10ms(const unsigned vfo)
{
	if(!gSetting_AM_fix || !enabled || vfo > 1 )
		return;

	if (gCurrentFunction != FUNCTION_FOREGROUND && !FUNCTION_IsRx()) {
#ifdef ENABLE_AM_FIX_SHOW_DATA
		counter = display_update_rate;  // queue up a display update as soon as we switch to RX mode
#endif
		return;
	}

#ifdef ENABLE_AM_FIX_SHOW_DATA
	if (counter > 0) {
		if (++counter >= display_update_rate) {	// trigger a display update
			counter        = 0;
			gUpdateDisplay = true;
		}
	}
#endif

	static uint32_t lastFreq[2];
	if(gEeprom.VfoInfo[vfo].pRX->Frequency != lastFreq[vfo]) {
		lastFreq[vfo] = gEeprom.VfoInfo[vfo].pRX->Frequency;
		AM_fix_reset(vfo);
	}

	int16_t rssi;
	{	// sample the current RSSI level and take the peak over the recent window.
		// Using the peak rather than the instantaneous value is what keeps the AM
		// modulation envelope out of the control loop.
		const int16_t new_rssi = BK4819_GetRSSI();

		rssi_hist[vfo][rssi_hist_pos[vfo]] = new_rssi;
		rssi_hist_pos[vfo] = (uint8_t)((rssi_hist_pos[vfo] + 1) % AM_FIX_PEAK_WINDOW);
		if (rssi_hist_len[vfo] < AM_FIX_PEAK_WINDOW)
			rssi_hist_len[vfo]++;

		rssi = rssi_hist[vfo][0];
		for (unsigned int i = 1; i < rssi_hist_len[vfo]; i++)
			if (rssi_hist[vfo][i] > rssi)
				rssi = rssi_hist[vfo][i];

		prev_rssi[vfo] = new_rssi;
	}

#ifdef ENABLE_AM_FIX_SHOW_DATA
	{
		static int16_t lastRssi;

		if (lastRssi != rssi) { // rssi changed
			lastRssi = rssi;

			if (counter == 0) {
				counter        = 1;
				gUpdateDisplay = true; // trigger a display update
			}
		}
	}
#endif

	// automatically adjust the RF RX gain

	// update the gain hold counter
	if (hold_counter[vfo] > 0)
		hold_counter[vfo]--;

	// dB difference between actual and desired RSSI level
	const int16_t diff_dB = (rssi - AM_fix_desired_rssi()) / 2;

	if (diff_dB > AM_FIX_MARGIN_HI)
	{	// too strong - reduce the gain, quickly
		unsigned int index = gain_table_index[vfo];   // current position we're at

		if (diff_dB >= AM_FIX_FAST_JUMP_DB)
		{	// jump immediately to a new gain setting
			// this greatly speeds up initial gain reduction (but reduces noise/spike immunity)

			const int16_t desired_gain_dB =
				(int16_t)gain_table[index].gain_dB - diff_dB + AM_FIX_JUMP_GUARD_DB;

			// scan the table to see what index to jump straight too
			while (index > 1)
				if (gain_table[--index].gain_dB <= desired_gain_dB)
					break;
		}
		else
		{	// incrementally reduce the gain .. taking it slow improves noise/spike immunity
			if (index > 1)
				index--;     // slow step-by-step gain reduction
		}

		index = MAX(1u, index);

		if (gain_table_index[vfo] != index)
		{
			gain_table_index[vfo] = index;
			decay_counter[vfo]    = 0;
		}

		hold_counter[vfo] = AM_FIX_HOLD_TICKS;
	}
	else if (diff_dB >= -AM_FIX_MARGIN_LO)
	{	// inside the deadband - leave the gain exactly where it is.
		// this is what stops the loop hunting around the target level
		hold_counter[vfo]  = AM_FIX_HOLD_TICKS;
		decay_counter[vfo] = 0;
	}
	else if (hold_counter[vfo] == 0)
	{	// too weak and the hold has expired - restore gain, SLOWLY.
		// one table step every 'decay_ticks' rather than every tick: stepping every
		// tick ramps at 200-300dB/s, which is audible as pumping on a fading signal
		uint8_t speed = gSetting_AM_speed;
		if (speed >= ARRAY_SIZE(decay_ticks))
			speed = AM_FIX_SPEED_DEFAULT;

		if (++decay_counter[vfo] >= decay_ticks[speed])
		{
			decay_counter[vfo] = 0;
			gain_table_index[vfo] = MIN(gain_table_index[vfo] + 1, gain_table_size - 1u);
		}
	}


	{	// apply the new settings to the front end registers
		const unsigned int index = gain_table_index[vfo];

		if (index != gain_table_index_prev[vfo])
		{
			// remember the new table index
			gain_table_index_prev[vfo] = index;
			currentGainDiff = gain_table[0].gain_dB - gain_table[index].gain_dB;
			BK4819_WriteRegister(BK4819_REG_13, gain_table[index].reg_val);

			// keep the squelch comparator referenced to the signal, not to our gain
			AM_fix_apply_squelch_shift();

#ifdef ENABLE_AGC_SHOW_DATA
			UI_MAIN_PrintAGC(true);
#endif
		}
	}

#ifdef ENABLE_AM_FIX_SHOW_DATA
	if (counter == 0) {
		counter        = 1;
		gUpdateDisplay = true;
	}
#endif
}

#ifdef ENABLE_AM_FIX_SHOW_DATA
void AM_fix_print_data(const unsigned vfo, char *s) {
	if (s != NULL && vfo < ARRAY_SIZE(gain_table_index)) {
		const unsigned int index = gain_table_index[vfo];
		sprintf(s, "%2u %4ddB %3u", index, gain_table[index].gain_dB, prev_rssi[vfo]);
		counter = 0;
	}
}
#endif

int8_t AM_fix_get_gain_diff()
{
	return currentGainDiff;
}

void AM_fix_enable(bool on)
{
	if (enabled == on)
		return;

	enabled = on;

	if (on)
	{	// force the gain and squelch offset to be reprogrammed on the next tick
		for (int i = 0; i < 2; i++)
		{
			gain_table_index_prev[i] = 0xFFFF;
			rssi_hist_len[i]         = 0;
			rssi_hist_pos[i]         = 0;
			decay_counter[i]         = 0;
			hold_counter[i]          = AM_FIX_SETTLE_TICKS;
		}
	}
	else
	{	// hand the front end and the squelch back in a known-good state,
		// otherwise whatever gain reduction we had applied would stick around
		// after switching away from AM
		currentGainDiff = 0;
		BK4819_WriteRegister(BK4819_REG_13, gain_table[0].reg_val);
		AM_fix_apply_squelch_shift();
	}
}
#endif
