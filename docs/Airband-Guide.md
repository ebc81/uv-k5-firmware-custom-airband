# Airband Guide

How to use this build for AM airband listening.

## Tuning a channel

Put the radio in frequency (VFO) mode and type the frequency. Anything you enter between
**108.000 and 137.000 MHz** automatically switches that VFO to **AM** and to the **8.33 kHz**
channel grid — you do not have to set the demodulator or the step by hand.

## 8.33 kHz channels

Type the **ICAO channel designator** exactly as it is published. The firmware maps it to the
real carrier for you:

| You type | Radio tunes | Why |
| --- | --- | --- |
| `118.005` | 118.00000 | 1st carrier of the 25 kHz block |
| `118.010` | 118.00833 | 2nd carrier |
| `118.015` | 118.01666 | 3rd carrier |
| `118.030` | 118.02500 | back on the 25 kHz grid |
| `118.025` | 118.02500 | 25 kHz channel, unchanged |

Stepping up and down with the arrow keys stays on the grid and re-syncs to the 25 kHz boundary
every fourth channel, so you can walk a whole band segment without drifting off-channel.

> **Do not type the carrier frequency for an 8.33 channel.** Typing `118.008` is not the same as
> typing `118.010`. The designator is what the radio expects, and it is also what is printed on
> charts and in ATIS.

Only a designator maps cleanly. Entering something that is neither a designator nor a 25 kHz
channel will land on the nearest carrier in that block.

### 25 kHz channels

If you are listening to a region still on 25 kHz spacing, set **`Step`** to `25kHz` for that VFO
and type the frequency normally.

## AM bandwidth

The **`AM BW`** menu item picks the receive filter used in AM:

| Setting | Use it for |
| --- | --- |
| `25k` | Quiet areas, or if `12.5k` sounds muffled to you |
| `12.5k` | **Default.** The right compromise for most airband listening |
| `6.25k` | Dense 8.33 kHz areas, or when a strong adjacent channel is splattering |

Narrower rejects more adjacent-channel energy but also rolls off the audio, so voices sound
duller. Start at `12.5k` and only go narrower if you actually hear interference from the
neighbouring channel.

This setting applies **only in AM**. FM still uses the per-channel `W/N` setting.

## Squelch

Airband shares its squelch calibration with the 2 m band, and AM signals are often weak and
fading, so the useful range is narrower than on FM.

* Start at **`Sql` 1–3**. Higher settings will cut off distant aircraft.
* `Sql 0` opens the squelch permanently — useful when you are hunting for a weak signal and do
  not mind the noise.
* If the squelch chatters open and closed on a signal you can clearly hear, that is a sign the
  signal is right at the threshold. Drop `Sql` by one.

In earlier firmware, squelch chatter on AM was often caused by the AGC dragging the squelch
reference around underneath it. That is fixed in this build, so squelch behaviour on AM should
now track the actual signal.

## Scanning

Scanning works as upstream, with one difference: **AM uses a 150 ms dwell per channel instead of
90 ms.** The AM squelch takes longer to respond and the AGC needs time to settle, and at 90 ms
weak signals were being skipped entirely. Scanning an AM list is therefore noticeably slower than
scanning an FM list — that is deliberate.

Scan resume behaviour is set by **`ScnRev`**:

| Mode | Behaviour |
| --- | --- |
| `TO` | Listen 5 s, then carry on scanning regardless |
| `CO` | Listen until the carrier drops, then resume 3.6 s later |
| `SE` | Stop scanning on the first signal found |

For airband, `CO` is usually what you want: ATC exchanges come in bursts, and `TO` will cut away
mid-transmission.

### Scan ranges

`ENABLE_SCAN_RANGES` is compiled in, so you can scan a frequency range rather than a channel
list. This is the quickest way to find what is active at a nearby field.

## Memory channels

Save airband channels as normal (`ChSave`). The stored channel keeps its own modulation and step,
so an AM 8.33 channel saved from a VFO comes back correctly.

Give channels names with `ChName` and set `ChDisp` to show name + frequency — far easier than
remembering which memory is TWR and which is GND.

## Spectrum analyzer

fagci's spectrum analyzer is compiled in. Press **`F` + `5`** to open it.

You can also assign **`SPECTRUM`** to one of the configurable buttons via `F1Shrt`, `F1Long`,
`F2Shrt`, `F2Long` or `M Long` if you would rather have it on a side key.

It is the fastest way to see which airband channels are active before committing to a scan.

## If reception is poor

Work through these in order:

1. **Check the antenna.** The stock antenna is a compromise at 118 MHz. An airband-cut antenna
   is by far the biggest single improvement available.
2. **Check `AM BW`.** If a nearby channel is splattering, go narrower.
3. **Check `Sql`.** Too high will simply mute weak aircraft.
4. **Tune `AMTarg`.** See [AM AGC Tuning](AM-AGC-Tuning.md) — this is the setting that determines
   whether strong signals distort.
