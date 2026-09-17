# Receive Only

This firmware is built with `ENABLE_TX = 0`. It cannot transmit.

## What that means in practice

* The power amplifier is never enabled.
* The transmit state in the firmware's state machine is unreachable — there is no code path that
  reaches it.
* **PTT does nothing.** It still stops a running scan, which is useful on a scanner and cannot
  transmit, but it will never key the radio.
* All transmit-related menu items are gone.

This is a stronger guarantee than the `F_LOCK` frequency table that upstream firmware uses.
`F_LOCK` is a runtime check on which frequencies may be transmitted on, and its hidden
`F_LOCK_NONE` setting removes the restriction entirely. Here there is no transmit code to
restrict.

## Why

Airband is an aviation safety service. Transmitting on it without a licence and a reason is
illegal essentially everywhere, and an accidental transmission on a tower or approach frequency
is genuinely dangerous. A receiver that physically cannot key up removes that risk.

It also freed a meaningful amount of flash, which paid for the reworked AM AGC, the extra menu
items and the debug readout.

## What was removed

**Menu items:** `TxPwr`, `TxDCS`, `TxCTCS`, `TxODir`, `TxOffs`, `TxTOut`, `Mic`, `MicBar`,
`Roger`, `STE`, `RP STE`, `UPCode`, `DWCode`, `PTT ID`, `D ST`, `D Prel`, `F Lock`, `Tx 200`,
`Tx 350`, `Tx 500`, `350 En`.

**Build options forced off:** `ENABLE_TX_WHEN_AM`, `ENABLE_ALARM`, `ENABLE_TX1750`,
`ENABLE_REDUCE_LOW_MID_TX_POWER`, `ENABLE_VOX`, `ENABLE_AIRCOPY`, `ENABLE_DTMF_CALLING`.

**Button actions no longer offered:** `VOX`, `ALARM`, `1750HZ`.

## What was kept

Everything on the receive side:

* Spectrum analyzer
* FM broadcast radio (BK1080)
* All scanning: channel, frequency, scan ranges, dual watch
* CTCSS/DCS **decode** (receive), descrambler, compander
* Live DTMF decoder
* RSSI bar, flashlight, all display and backlight options
* UART, so the radio can still be configured from a PC

## Building a transmit-capable image

Set `ENABLE_TX = 1` in the `Makefile` and rebuild:

```bash
make clean && make ENABLE_TX=1
```

Everything comes back: the menu items, PTT, the state machine and the driver code. The TX path
was guarded rather than deleted, so the tree still merges cleanly with upstream, and an
`ENABLE_TX=1` build reproduces the pre-removal image byte count exactly.

You will also want to re-enable whichever of `ENABLE_VOX`, `ENABLE_DTMF_CALLING` and the rest you
actually use — they are independent options and turning `ENABLE_TX` back on does not turn them on.

> Check the build size afterwards. The flash budget is 60 KB and the link fails outright on
> overflow. See [Building](Building.md).

## A note on the hardware

Removing the firmware's transmit path does not modify the radio. The PA hardware is still
present, and flashing different firmware restores transmit capability. This is a software
guarantee about *this build*, not a hardware modification.
