# CLAUDE.md

Guidance for working in this repository.

## What this is

Custom firmware for the **Quansheng UV-K5** handheld radio, specialised for **AM airband
reception (108-137 MHz)**.

Lineage: DualTachyon (original reverse-engineering) -> OneOfEleven / fagci forks ->
`egzumer/uv-k5-firmware-custom` -> `ebc81/uv-k5-firmware-custom-airband` (this tree).

Hardware:

| Part | Detail |
|---|---|
| MCU | DP32G030, ARM Cortex-M0, **60 KB flash / 16 KB RAM** |
| Transceiver | Beken **BK4819** (HF/VHF/UHF, FM + AM + SSB demod) |
| FM broadcast RX | Beken BK1080 (separate chip, `driver/bk1080.c`) |
| Display | ST7565 128x64 LCD |
| EEPROM | 8 KB I2C |

**This is a receive-only build.** `ENABLE_TX = 0` removes every transmit path: the PA is never
enabled and `FUNCTION_TRANSMIT` is unreachable, so the radio cannot key up. That is a stronger
guarantee than the `F_LOCK` frequency table, which `F_LOCK_NONE` can defeat. To build a
TX-capable image again, set `ENABLE_TX = 1` in the `Makefile`.

Flags that are moot in an RX-only build (keep them at 0): `ENABLE_TX_WHEN_AM`, `ENABLE_ALARM`,
`ENABLE_TX1750`, `ENABLE_REDUCE_LOW_MID_TX_POWER`, `ENABLE_VOX`, `ENABLE_AIRCOPY`.

## Build

No ARM toolchain on Windows; build inside WSL.

```bash
# one-time
sudo apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi \
                        libnewlib-arm-none-eabi make python3-crcmod

# build
cd /mnt/c/proj/uv-k5-firmware-custom-airband
make clean && make
arm-none-eabi-size firmware
```

Two artifacts are produced:

- `firmware.bin` - raw image, for SWD/openocd.
- **`firmware.packed.bin` - the file to flash with the Quansheng flasher.** `fw-pack.py`
  inserts a 16-byte version string at offset `0x2000`, XORs the whole thing with a fixed
  128-byte obfuscation key, and appends a big-endian XMODEM CRC-16.

`AUTHOR_STRING` and `VERSION_STRING` are joined as `*<AUTHOR> <VERSION>` and **truncated to 16
bytes total** - keep them short. `VERSION_STRING` defaults to the git tag or short hash.

### Hard constraints

- **60 KB flash** (`firmware.ld:10`). The link fails outright on overflow. Always check
  `arm-none-eabi-size firmware`: `text + data` must be <= 61440.
- `CFLAGS` are `-Os -Wall -Werror -Wextra -std=c2x -flto`. **Any new warning is a build
  failure**, so a different GCC version than the one used before can break an untouched tree.
  Upstream CI builds on Arch (`.github/workflows/main.yml`, `Dockerfile`); Ubuntu's
  `gcc-arm-none-eabi` is usually older.
- Every `ENABLE_*` flag needs plumbing in **two** places in the `Makefile`: the default near the
  top, and a matching `ifeq (...,1) CFLAGS += -D...` block further down. Forgetting the second
  one silently disables the feature.

## Architecture

### The 10 ms tick chain

```
SystickHandler()            scheduler.c   - ISR, every 10 ms. ALL countdown decrements happen
                                            here (scan pause, dual watch, power save, TTE).
main loop                   main.c:222
  APP_Update()                            - FREE-RUNNING, many times per tick.
                                            Contains the squelch decision (HandleFunction()).
  if (gNextTimeslice)
    APP_TimeSlice10ms()     app/app.c     - at most once per tick:
       AM_fix_10ms()                        1. writes REG_13 (front-end gain)
       CheckRadioInterrupts()               2. refreshes g_SquelchLost from the BK4819
       display / scanner / keys
    APP_TimeSlice500ms()                  - battery, backlight, RSSI bar
```

Two consequences worth remembering:

1. `APP_Update()` spins on a `g_SquelchLost` that is only refreshed once per 10 ms, so the
   squelch consumers re-evaluate the same stale flag hundreds of times between refreshes.
2. AM-fix changes the front-end gain **immediately before** the squelch flag is sampled, and
   that flag is acted on one iteration later. There is no interlock between the two - which is
   why AM-fix compensates the squelch thresholds itself (see below).

### Squelch

The squelch is a **pure hardware comparator inside the BK4819**. Firmware never re-evaluates
it: `g_SquelchLost` is written only from the interrupt bits in `CheckRadioInterrupts()`, plus
two initialisations. There is no software RSSI comparison, no averaging, no hysteresis timer.

Thresholds come from the **radio's own EEPROM calibration area**, not from a table in the
source:

```
base = (band < BAND4_174MHz) ? 0x1E60 : 0x1E00   // "VHF" row : "UHF" row
addr = base + squelch_level                      // level 1..9
  +0x00 RSSI open     +0x10 RSSI close
  +0x20 noise open    +0x30 noise close
  +0x40 glitch close  +0x50 glitch open
```

Airband is `BAND2_108MHz`, so it uses the `0x1E60` VHF row - shared with 2 m.
RSSI thresholds are "higher = harder to open"; **noise and glitch are inverted**.

BK4819 terminology trap: **"squelch lost" means the squelch OPENED** (carrier present);
"squelch found" means it closed. `g_SquelchLost == true` means we are receiving.

The glitch and ex-noise counters (REG_63 / REG_65) are read **only** by `app/uart.c` for
telemetry. They never influence the squelch decision from firmware.

### BK4819 RX register cheat-sheet

| Reg | Purpose |
|---|---|
| `REG_13` | Front-end gain (LNA-short `<9:8>`, LNA `<7:5>`, mixer `<4:3>`, PGA `<2:0>`). **Driven by `am_fix.c`.** |
| `REG_10`-`REG_14` | AGC gain tables [0],[1],[2],[3],[-1]. Loaded by `BK4819_InitAGC()`. |
| `REG_7E` | `<15>` AGC fix mode (1 = manual), `<14:12>` fix index (always 3 -> REG_13). |
| `REG_49` | AGC RF high/low window. AM: 50/32, FM: 84/56. Unused while AGC is in fix mode. |
| `REG_43` | Filter bandwidth (RF BW, RF BW when weak, AFTxLPF2, BW mode). |
| `REG_47` | AF output select: 1 = FM, 5 = USB, 7 = AM. |
| `REG_48` | `<11:10>` AF gain-1, `<9:4>` AF gain-2 (`VOLUME_GAIN`), `<3:0>` AF DAC gain. |
| `REG_3D` | IF. `0x2AAB` for everything except USB (0). |
| `REG_73` | `<4>` AFC disable - set for non-FM. |
| `REG_4D` | Glitch threshold, squelch **close**. |
| `REG_4E` | `<13:11>` open delay, `<10:9>` close delay, `<7:0>` glitch threshold squelch **open**. |
| `REG_4F` | `<14:8>` noise close, `<6:0>` noise open. |
| `REG_78` | `<15:8>` RSSI open, `<7:0>` RSSI close. **0.5 dB/step.** |
| `REG_67` | RSSI readout. `dBm = (rssi / 2) - 160`. |

### AM fix (`am_fix.c`)

The BK4819's AGC is unusable for AM, so in AM `RADIO_SetupAGC()` turns it off entirely
(`BK4819_SetAGC(0)`) and `am_fix.c` rides `REG_13` in software from `APP_TimeSlice10ms()`.

Key design points of this tree's version:

- The control variable is the **maximum RSSI over a short sliding window**, not the
  instantaneous sample. On a modulated AM carrier the RSSI *is* the audio envelope; feeding it
  straight into the loop makes the AGC modulate the gain at audio rate, which is exactly the
  "pumping"/distortion this module exists to avoid.
- **Asymmetric attack/decay**: gain is reduced fast (overload must be corrected immediately)
  and restored slowly (one table step every N ticks).
- **The squelch thresholds are offset by the current gain reduction.** Because the hardware
  comparator sees post-gain RSSI, every gain step would otherwise move the squelch reference
  underneath it. `REG_78` is rewritten with both thresholds shifted by `-2 * gain_diff_dB`
  (0.5 dB units, clamped 0..255) so the squelch decision is independent of what the AGC does.
- Gain table entry **0 is the stock Quansheng value** (`0x03BE`, -7 dB); entries 1..42 run from
  -93 dB to 0 dB. Index 0 is never selected by the control loop, only restored on shutdown.

The `-89 dBm` default target is the original author's estimate, **not a measurement**. It is
the single most worthwhile number to tune per-radio - see the `AMTarg` menu item.

### Adding a menu item

The display order is the `MenuList[]` array order; `MENU_*` is only an ID. Six edits, all
required:

1. `ui/menu.h` - add the identifier to the anonymous enum. **Not** inside the
   `MENU_F1SHRT...MENU_MLONG` run, which must stay contiguous (it is indexed by pointer
   arithmetic in `app/menu.c`).
2. `ui/menu.c` - add a `{"Label", VOICE_ID_INVALID, MENU_XXX}` row to `MenuList[]`. Label is
   **max 6 chars**. Before the `F Lock` row = normally visible, after it = hidden menu. Never
   move the `{"", VOICE_ID_INVALID, 0xff}` terminator.
3. `ui/menu.c` - a `case` in the `UI_DisplayMenu()` switch that renders the value into `String`.
4. `app/menu.c` - a `case` in **`MENU_GetLimits()`**. This is mandatory: it drives clamping,
   up/down wraparound and numeric entry. Without it the item is inert.
5. `app/menu.c` - a `case` in `MENU_ShowCurrentSetting()` to seed `gSubMenuSelection`.
6. `app/menu.c` - a `case` in `MENU_AcceptSetting()`. `break` -> global settings save;
   `return` -> no global save (use for per-channel settings with `gRequestSaveChannel = 1`).
   Add `gVfoConfigureMode = VFO_CONFIGURE_RELOAD; gFlagResetVfos = true;` if the radio must be
   reprogrammed.

There is no menu-count constant to bump: `gMenuListCount` is derived at boot from the `""`
terminator in `main.c`.

### EEPROM map (8 KB)

| Address | Content |
|---|---|
| `0x0000`-`0x0C7F` | MR channels 0-199 (16 bytes each) |
| `0x0C80`-`0x0D5F` | VFO A/B per-band records: `0x0C80 + band*32 + vfo*16` |
| `0x0D60`-`0x0E27` | channel attributes (band, compander, scanlists) |
| `0x0E28`-`0x0E3F` | **free (24 bytes)** - erased by factory reset |
| `0x0E40`-`0x0E67` | FM broadcast channels |
| `0x0E70`-`0x0EAF` | global settings blocks (see `SETTINGS_SaveSettings`) |
| `0x0EAD`-`0x0EAF` | **airband settings: AM target, AM AGC speed, AM bandwidth** |
| `0x0EB0`-`0x0ECF` | welcome strings |
| `0x0ED0`-`0x0F17` | DTMF settings and codes |
| `0x0F18`-`0x0F1F` | scan lists |
| `0x0F20`-`0x0F2F` | **free (16 bytes)** - survives factory reset |
| `0x0F30`-`0x0F3F` | AES key |
| `0x0F40`-`0x0F47` | F_LOCK, band-TX enables, `gSetting_*` bitfield (bit 5 = AM fix) |
| `0x0F48`-`0x0F4F` | **free (8 bytes)** - survives factory reset |
| `0x0F50`-`0x1BFF` | MR channel names |
| `0x1C00`-`0x1DFF` | DTMF contacts |
| `0x1E00` / `0x1E60` | squelch calibration (UHF row / VHF row) |
| `0x1EC0` | RSSI calibration |
| `0x1ED0` | TX power calibration (unused in an RX-only build) |
| `0x1F40` | battery calibration |
| `0x1F88` | xtal trim, volume gain, DAC gain |
| `0x1FF0` | build-options bitmap |

`EEPROM_WriteBuffer` always writes 8 bytes; every address above is 8-aligned. `gEeprom` is
**not** a byte-image of the EEPROM - it is copied field by field, so struct members can be added
anywhere without breaking the layout. Members named `fieldNN_0xNN` are RAM padding left over
from the original decompilation.

`SETTINGS_FactoryReset` deliberately **skips** `0x0EA0`-`0x0EA8`, `0x0F18`-`0x0F30` and
`0x0F30`-`0x0F50`, so settings stored there survive "Reset ALL".

## Airband facts

- `BAND2_108MHz` = 108.00000-137.00000 MHz. **All frequencies in this codebase are in units of
  10 Hz**, so 108 MHz is `10800000`.
- `dBmCorrTable[BAND2_108MHz] = -25` (`ui/main.c`) - the display calibration offset.
- **8.33 kHz handling is correct and should not be "fixed".** `FREQUENCY_RoundToStep()` maps the
  ICAO channel *designator* you type on the keypad (`.005`, `.010`, `.015`, `.030`) to the true
  carrier, and re-syncs to the 25 kHz grid on every fourth channel via the `chno == 3` term:

  | typed | carrier |
  |---|---|
  | 118.005 | 118.00000 |
  | 118.010 | 118.00833 |
  | 118.015 | 118.01666 |
  | 118.030 | 118.02500 |

  The step constant is `833` (8.330 kHz), not 8.3333, so the third carrier of a block lands
  6.7 Hz low. The error never accumulates because of the re-sync, and 6.7 Hz is far inside AM
  demodulator tolerance.

## Known upstream bugs

Recorded so they are not rediscovered. Those marked *fixed* were corrected in this tree.

- *fixed* - `RADIO_SetupAGC()`: `newSettings = (listeningAM << 1) | (disable << 1)` shifted both
  operands by 1, collapsing two flags into one bit. The memoisation then treated
  `(AM=1, disable=0)` and `(AM=0, disable=1)` as the same state and early-returned without
  reprogramming the AGC.
- *fixed* - `radio.c`: `if (frequency >= ...BAND2_108MHz.upper && frequency < ...BAND2_108MHz.upper)`
  compares `.upper` against itself and is always false. Intent was `.lower ... .upper`.
- *fixed* - `BK4819_SetupSquelch()`: `(6u << 9)` needs three bits but the close-delay field
  `<10:9>` is two bits wide, so the MSB spilled into bit 11, which belongs to the open-delay
  field. The fields are OR'd, and bit 11 was already set by `(5u << 11)`, so the collision was
  absorbed: the register came out `0x6C00` - open delay 5 as intended, but close delay 2, not
  the "*3" the comment claims. Watch out when reasoning about this one: adding the terms
  instead of OR-ing them gives `0x7400` and the wrong conclusion that the open delay was 6.
  FM now keeps 5/2 explicitly (unchanged behaviour); AM uses a shorter open delay of 2.
- **not fixed** - `CheckRadioInterrupts()` writes `REG_02 = 0` to clear the interrupt latch
  *before* reading `REG_02` to fetch the flags. It works on this part, but the ordering is
  race-prone. Left alone deliberately: it sits in the middle of the RX state machine.
- **not fixed** - `BK4819_GetRSSI_dBm()` has its `- BK4819_GetRxGain_dB()` compensation
  commented out. The UI compensates separately via `AM_fix_get_gain_diff()`.
- **not fixed** - `radio.c` uses `#if ENABLE_SQUELCH_MORE_SENSITIVE` where every other flag test
  uses `#ifdef`. It works only because GCC evaluates the bare defined macro as 1.
- Note `ENABLE_SQUELCH_MORE_SENSITIVE` doubles the noise-open threshold, which for the factory
  VHF value (65) exceeds the 127 clamp - so **the noise criterion is effectively disabled** and
  only the halved RSSI-open and doubled glitch-open survive.
- `BK4819_GetExNoiceIndicator` is a typo in upstream's exported API. There is no
  `BK4819_GetNoiseIndicator`.
- `BK4819_SetupSquelch()` takes glitch arguments **close-then-open** while RSSI and noise are
  **open-then-close**. Easy to get wrong.

## Testing

There are no unit tests and no emulator - verification is a clean build plus on-radio checks.

Static checks that are worth doing after touching the RX path:

```bash
make clean && make && arm-none-eabi-size firmware   # must fit 61440 bytes
make clean && make ENABLE_AM_FIX=0                  # guards are correct
make clean && make ENABLE_TX=1                      # TX path still builds
```

On-radio procedure for AM/airband changes:

1. Flash `firmware.packed.bin`.
2. Confirm PTT does nothing (RX-only build).
3. Strong local tower/ATIS: audio should be clean, and with `ENABLE_AM_FIX_SHOW_DATA` the gain
   index should settle and hold rather than oscillate.
4. Weak/distant aircraft: squelch should stay open through the transmission instead of
   chattering as the AGC moves.
5. Scan a range of airband channels; weak signals should not be skipped.
6. Sweep the `AMTarg` menu item down until distortion returns, then back off. That finds the
   real demodulator saturation point for your individual radio.
