# UV-K5 Airband Edition

*This folder mirrors the [project wiki](https://github.com/ebc81/uv-k5-firmware-custom-airband/wiki),
so the documentation is versioned and tagged alongside the firmware it describes. The wiki is
easier to browse; these files are what shipped with this particular revision.*

Quansheng UV-K5/K6/5R firmware tuned for **AM airband reception (108–137 MHz)**, with the AM
AGC reworked and **all transmit functionality removed**.

Forked from [egzumer/uv-k5-firmware-custom](https://github.com/egzumer/uv-k5-firmware-custom).

> **This firmware cannot transmit.** It is built with `ENABLE_TX = 0`, which removes the whole
> transmit path: the power amplifier is never enabled and the transmit state is unreachable, so
> the radio cannot key up. PTT does nothing (it still stops a scan). See
> [Receive Only](Receive-Only.md).

## Start here

| Page | What it covers |
| --- | --- |
| [Airband Guide](Airband-Guide.md) | Tuning airband channels, 8.33 kHz, bandwidth, scanning |
| [AM AGC Tuning](AM-AGC-Tuning.md) | What the AM fix does and how to tune it for your radio |
| [Menu Reference](Menu-Reference.md) | Every menu item in this build |
| [Receive Only](Receive-Only.md) | What was removed, and how to build a TX-capable image |
| [Building](Building.md) | Compiling from source |

## Download

Grab `firmware.packed.bin` from the
[releases page](https://github.com/ebc81/uv-k5-firmware-custom-airband/releases) and flash it
with the [online flasher](https://egzumer.github.io/uvtools) or any Quansheng flashing tool.

`firmware.bin` is the raw image and is only useful over SWD.

## What changed, in one paragraph

The BK4819 has no usable AGC for AM, so the firmware rides the front-end gain register itself
every 10 ms. The original loop fed *instantaneous* RSSI into its control law — but on a modulated
AM carrier the RSSI **is** the audio envelope, so the AGC ended up modulating the gain at audio
rate. That is the pumping and distortion people hear on AM. This build drives the loop from the
peak RSSI over an 80 ms window, restores gain slowly instead of at 200–300 dB/s, and shifts the
squelch thresholds by however much gain it has taken away so the squelch stops chasing the AGC.
On top of that: selectable AM bandwidth, faster AM squelch response, a longer AM scan dwell, and
AM + 8.33 kHz selected automatically on the airband.

## A realistic expectation

The UV-K5 is not a professional receiver. The RX front end has no track-tuned band-pass
filtering, so it is wide open to everything, and its dynamic range is limited. No firmware change
fixes that. What the rework addresses is the part that was self-inflicted — a control loop
fighting its own input. Expect cleaner audio on strong signals and fewer dropouts on weak ones,
not a different radio.

## Credits

* [egzumer](https://github.com/egzumer) — the firmware this forks
* [OneOfEleven](https://github.com/OneOfEleven) — the original AM fix and many mods
* [fagci](https://github.com/fagci) — spectrum analyzer
* [DualTachyon](https://github.com/DualTachyon) — the original open firmware
