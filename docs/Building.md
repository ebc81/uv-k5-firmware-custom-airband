# Building

## WSL / Linux

This is how the released binaries are built.

```bash
# one-time
sudo apt-get install -y gcc-arm-none-eabi binutils-arm-none-eabi libnewlib-arm-none-eabi make python3-crcmod

# build
git clone --recurse-submodules https://github.com/ebc81/uv-k5-firmware-custom-airband.git
cd uv-k5-firmware-custom-airband
make clean && make
arm-none-eabi-size firmware
```

`--recurse-submodules` matters: the tree pulls in CMSIS and a printf implementation as
submodules, and the build fails without them.

## Output

| File | Use |
| --- | --- |
| **`firmware.packed.bin`** | **The file to flash.** Obfuscated, with a version header and CRC |
| `firmware.bin` | Raw image, only useful over SWD |

`fw-pack.py` produces the packed file: it inserts a 16-byte version string at offset `0x2000`,
XORs the image with a fixed key and appends an XMODEM CRC-16. It needs `crcmod`; without it the
build still succeeds but silently skips the packed file, so check that it exists.

## The flash budget

**60 KB (61440 bytes).** The link fails outright on overflow, so this is not something you can
ignore. After any change:

```bash
arm-none-eabi-size firmware
```

`text + data` must be ≤ 61440. `bss` is RAM and does not count.

v0.1 uses **54436 bytes**, leaving about 7 KB free.

## Compiler notes

The v0.1 release was built with **GCC 13.2.1** (Ubuntu 24.04 `gcc-arm-none-eabi`). Upstream
recommends 10.3.1.

The build uses `-Os -Wall -Werror -Wextra -std=c2x -flto`. **`-Werror` means any new warning is a
build failure**, so a different compiler version can break an otherwise untouched tree. If that
happens, fix the warning rather than weakening the flags — the size margin depends on `-Os` and
LTO doing their job.

## Build options

Options live at the top of the `Makefile`; `0` disables, `1` enables. The ones specific to this
fork:

| Option | Default | What it does |
| --- | --- | --- |
| `ENABLE_TX` | `0` | Transmit support. `0` removes the whole TX path — see [Receive Only](Receive-Only.md) |
| `ENABLE_AIRBAND_DEFAULTS` | `1` | AM + 8.33 kHz selected automatically for 108–137 MHz |
| `ENABLE_AM_FIX` | `1` | The AM AGC. Also adds the `AMTarg` and `AMSpd` menu items |
| `ENABLE_AM_FIX_SHOW_DATA` | `1` | The on-screen AM AGC debug readout |

> Adding a new `ENABLE_*` option means editing the `Makefile` in **two** places: the default near
> the top, and the matching `ifeq (...,1) CFLAGS += -D...` block further down. Miss the second and
> the option silently does nothing.

If a build overflows the flash, the practical things to drop, in order: `ENABLE_AM_FIX_SHOW_DATA`,
`ENABLE_FLASHLIGHT`, `ENABLE_FMRADIO`, `ENABLE_SPECTRUM`.

## Verifying a change

There are no unit tests and no emulator. After touching the RX path:

```bash
make clean && make && arm-none-eabi-size firmware   # must fit 61440 bytes
make clean && make ENABLE_AM_FIX=0                  # guards are correct
make clean && make ENABLE_AIRBAND_DEFAULTS=0
make clean && make ENABLE_TX=1                      # TX path still builds
```

All four must build with zero warnings. Then flash and test on the radio — see the on-radio
checklist in [`CLAUDE.md`](../CLAUDE.md) at the repository root, which also documents the firmware's architecture,
the BK4819 register map, the EEPROM layout and a list of known upstream bugs.

## Other build methods

The upstream Docker, Codespace and native-Windows methods still work — see the
[README](https://github.com/ebc81/uv-k5-firmware-custom-airband#building). Docker in particular
tends to produce slightly smaller binaries.

Note that GitHub Actions is disabled by default on forks, so pushing a tag here does **not**
build a release automatically. The v0.1 binaries were built locally and uploaded to the release.
