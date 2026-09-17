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

#ifndef AM_FIXH
#define AM_FIXH

#include <stdint.h>
#include <stdbool.h>

#ifdef ENABLE_AM_FIX

	// The AGC target level, as a dBm range. gSetting_AM_target holds the offset
	// from AM_FIX_TARGET_DBM_MIN, so it stays a small unsigned EEPROM byte.
	//
	// The -89dBm default is the original author's estimate of where the BK4819's
	// AM demodulator starts to clip - it is not a measurement, and it is the
	// number most worth tuning per-radio.
	#define AM_FIX_TARGET_DBM_MIN       (-100)
	#define AM_FIX_TARGET_DBM_MAX       (-70)
	#define AM_FIX_TARGET_DBM_DEFAULT   (-89)
	#define AM_FIX_TARGET_MAX_INDEX     (AM_FIX_TARGET_DBM_MAX - AM_FIX_TARGET_DBM_MIN)
	#define AM_FIX_TARGET_DEFAULT       (AM_FIX_TARGET_DBM_DEFAULT - AM_FIX_TARGET_DBM_MIN)

	// how fast the gain is restored once a signal drops
	#define AM_FIX_SPEED_FAST           0
	#define AM_FIX_SPEED_MED            1
	#define AM_FIX_SPEED_SLOW           2
	#define AM_FIX_SPEED_MAX_INDEX      AM_FIX_SPEED_SLOW
	#define AM_FIX_SPEED_DEFAULT        AM_FIX_SPEED_MED

	void AM_fix_init(void);
	void AM_fix_reset(const unsigned vfo);
	void AM_fix_10ms(const unsigned vfo);
	#ifdef ENABLE_AM_FIX_SHOW_DATA
		void AM_fix_print_data(const unsigned vfo, char *s);
	#endif
	int8_t AM_fix_get_gain_diff();
	void AM_fix_enable(bool on);

	// tell am_fix the un-offset squelch RSSI thresholds that were just programmed,
	// so it can shift them by however much front end gain it takes away
	void AM_fix_set_squelch_base(uint8_t open, uint8_t close);

#endif

#endif
