# AM AGC Tuning

The BK4819's own AGC is unusable for AM, so the firmware switches it off and drives the front-end
gain register from software, once every 10 ms. That software loop is the "AM fix".

## Why the old one distorted

On a modulated AM carrier, the received signal strength **is** the audio envelope — that is what
amplitude modulation means. The original loop sampled instantaneous RSSI and reacted to it, so it
followed the envelope and modulated the gain at audio rate. The result is the pumping and
distortion AM listeners are used to on this radio.

Two other things made it worse:

* Once its hold expired, it raised the gain **one table step every 10 ms**. Steps average 2–3 dB,
  so gain ramped at roughly 200–300 dB/s.
* The squelch is a hardware comparator on **post-gain** RSSI. Every gain step moved the squelch
  reference underneath it, so the squelch chattered in step with the AGC.

## What this build does instead

* **Peak detector** — the loop is driven by the *maximum* RSSI over an 80 ms sliding window, not
  the instantaneous sample. 80 ms spans the whole audio band, so the envelope never reaches the
  control law. It tracks the carrier-plus-peak-modulation level, which is exactly the thing that
  must stay below the clipping point.
* **Asymmetric attack and decay** — gain still drops fast, because overload has to be corrected
  immediately, but it is restored one step every 4, 8 or 16 ticks depending on `AMSpd`.
* **A real deadband** — the loop does nothing while the level sits inside a window around the
  target, instead of permanently re-arming a hold at the window edge. That is what stopped it
  hunting.
* **Squelch compensation** — both squelch RSSI thresholds are shifted by exactly the amount of
  gain that has been removed, so the squelch decision is independent of what the AGC is doing.

## The settings

### `AMTarg` — target level

Range **−100 to −70 dBm**, default **−89 dBm**.

This is the level the AGC holds the signal peak at. Below it, the demodulator is happy; above it,
the demodulator starts to clip and the audio distorts.

> **This is the one setting worth tuning for your individual radio.** The −89 dBm default is the
> original author's *estimate* of where the BK4819's AM demodulator saturates. It was never
> measured, and there is unit-to-unit variation.

**How to find your radio's value:**

1. Find a consistently strong, continuously transmitting AM signal. **ATIS or VOLMET is ideal** —
   it runs on a loop, so you can listen to the same audio repeatedly.
2. Set `AMTarg` to `-80dBm` and listen. You will probably hear distortion.
3. Step it down 2–3 dB at a time. At some point the distortion disappears.
4. Go **3–5 dB below** that point and leave it there. That margin covers signals stronger than
   your test signal.

Going lower than necessary costs sensitivity on weak signals, so do not simply set it to −100.

### `AMSpd` — decay speed

How quickly gain is restored once a signal drops away.

| Setting | One gain step every | Roughly |
| --- | --- | --- |
| `FAST` | 4 ticks (40 ms) | ~60 dB/s |
| `MED` | 8 ticks (80 ms) | ~30 dB/s — **default** |
| `SLOW` | 16 ticks (160 ms) | ~15 dB/s |

* **`MED`** suits most listening.
* **`SLOW`** is better when you sit on one busy frequency with a mix of strong local and weak
  distant stations — it stops the gain racing back up between overs and pumping the noise floor
  at you.
* **`FAST`** is better when scanning, or when signals vary a lot channel to channel, because the
  loop has less time to converge on each one.

Attack (gain reduction) is **not** affected by this setting. Overload is always corrected as
fast as the loop can manage, by design.

### `AM Fix` — on/off

Turns the whole thing off. With it off, the chip's own AGC is re-enabled with its AM register
values. Useful mainly as an A/B comparison — reception will be noticeably worse on strong signals.

## Reading the debug display

This build ships with the AM-fix debug readout enabled. When you are receiving an AM signal, a
line on the main screen shows three numbers:

```
23  -42dB 168
│    │     │
│    │     └─ last raw RSSI reading
│    └─────── gain currently applied, in dB
└──────────── gain table index (1 = minimum gain, 42 = maximum)
```

To convert the raw RSSI to dBm: **`dBm = raw / 2 − 160`**. So `168` is −76 dBm.

What to look for:

* **The index should settle and hold** on a steady signal. If it oscillates continuously by more
  than a step or two, the loop is still hunting — try `SLOW`.
* **Index pinned at 42** (maximum gain) means the signal is weak and the AGC has nothing left to
  give. That is normal on distant aircraft.
* **Index down near 1–10** means a very strong signal and heavy gain reduction. If you still hear
  distortion there, lower `AMTarg`.

To remove the readout, rebuild with `ENABLE_AM_FIX_SHOW_DATA = 0`.

## What this cannot fix

The RX front end has no track-tuned band-pass filter, and the receiver's dynamic range is
limited. In a high-RF environment — near a broadcast transmitter, or at an airport with strong
nearby transmitters — the front end will be overloaded before the AGC has any say in it. No
firmware setting helps there; an external band-pass filter or attenuator does.
