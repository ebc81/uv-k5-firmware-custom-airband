# Menu Reference

Every menu item present in the airband build, in the order it appears on the radio.

Because this is a receive-only build, all transmit-related items are gone: `TxPwr`, `TxDCS`,
`TxCTCS`, `TxODir`, `TxOffs`, `TxTOut`, `Mic`, `MicBar`, `Roger`, `STE`, `RP STE`, `UPCode`,
`DWCode`, `PTT ID`, `D ST`, `D Prel`, `F Lock`, `Tx 200`, `Tx 350`, `Tx 500` and `350 En`.
See [Receive Only](Receive-Only.md).

## Visible menu

| # | Item | What it does |
| --- | --- | --- |
| 1 | `Step` | Tuning step. `8.33kHz` is selected automatically on the airband |
| 2 | `RxDCS` | Receive DCS code |
| 3 | `RxCTCS` | Receive CTCSS tone |
| 4 | `W/N` | Channel bandwidth, **FM only** — AM uses `AM BW` |
| 5 | `Scramb` | Descrambler |
| 6 | `BusyCL` | Busy channel lockout |
| 7 | `Compnd` | Compander |
| 8 | `Demodu` | Demodulator: FM / AM / USB |
| 9 | `ScAdd1` | Add channel to scan list 1 |
| 10 | `ScAdd2` | Add channel to scan list 2 |
| 11 | `ChSave` | Save the current VFO to a memory channel |
| 12 | `ChDele` | Delete a memory channel |
| 13 | `ChName` | Edit a channel name |
| 14 | `SList` | Which scan list(s) to scan |
| 15 | `SList1` | Scan list 1 priority channels |
| 16 | `SList2` | Scan list 2 priority channels |
| 17 | `ScnRev` | Scan resume mode: `TO` / `CO` / `SE` |
| 18 | `F1Shrt` | Side key 1, short press |
| 19 | `F1Long` | Side key 1, long press |
| 20 | `F2Shrt` | Side key 2, short press |
| 21 | `F2Long` | Side key 2, long press |
| 22 | `M Long` | MENU key, long press |
| 23 | `KeyLck` | Automatic keypad lock |
| 24 | `BatSav` | Battery save ratio |
| 25 | `ChDisp` | Channel display mode (name / frequency / both) |
| 26 | `POnMsg` | Power-on message |
| 27 | `BatTxt` | Battery readout style on the status bar |
| 28 | `BackLt` | Backlight timeout |
| 29 | `BLMin` | Backlight minimum brightness |
| 30 | `BLMax` | Backlight maximum brightness |
| 31 | `BltTRX` | Backlight on RX |
| 32 | `Beep` | Key beep |
| 33 | `1 Call` | One-touch call channel |
| 34 | `D Live` | Live DTMF decoder |
| 35 | **`AM BW`** | **AM receive bandwidth: 25k / 12.5k / 6.25k** |
| 36 | **`AM Fix`** | **AM AGC on/off** |
| 37 | **`AMTarg`** | **AM AGC target level, −100 … −70 dBm** |
| 38 | **`AMSpd`** | **AM AGC decay speed: FAST / MED / SLOW** |
| 39 | `BatVol` | Battery voltage / percentage |
| 40 | `RxMode` | Dual watch / crossband mode |
| 41 | `Sql` | Squelch level, 0–9 |

Items in **bold** are new in this fork. See [AM AGC Tuning](AM-AGC-Tuning.md) for `AM Fix`,
`AMTarg` and `AMSpd`, and the [Airband Guide](Airband-Guide.md) for `AM BW`.

## Hidden menu

Hold **PTT + the upper side button** while switching the radio on to reveal these.

| # | Item | What it does |
| --- | --- | --- |
| 42 | `ScraEn` | Enable the scrambler menu item |
| 43 | `BatCal` | Battery voltage calibration |
| 44 | `BatTyp` | Battery type: 1600 mAh / 2200 mAh |
| 45 | `Reset` | Factory reset |

`F Lock` and the per-band TX enables used to live here. They only gated transmission, so they
are gone.

## Notes on specific items

### `Step`

Entering a frequency in 108–137 MHz sets this to `8.33kHz` automatically. You can still override
it — set `25kHz` for regions on the old spacing.

### `W/N` vs `AM BW`

`W/N` is a single bit stored per channel and only reaches wide (25 kHz) or narrow (12.5 kHz). It
applies to **FM only** in this build. AM has its own `AM BW` setting, which additionally reaches
6.25 kHz — useful on dense 8.33 kHz spacing and not selectable at all in upstream firmware.

### `Sql`

Squelch thresholds come from your radio's own EEPROM calibration, not from the firmware. Airband
uses the same calibration row as the 2 m band. Levels 1–3 suit airband; `0` holds the squelch
open permanently.

### `Reset`

A factory reset restores `AM BW`, `AMTarg` and `AMSpd` to their defaults, since they are stored
in a region that the reset clears. `AM Fix` lives in a region the reset deliberately skips, so
its setting survives.

### Configurable buttons

`F1Shrt`, `F1Long`, `F2Shrt`, `F2Long` and `M Long` can be assigned: `NONE`, `FLASHLIGHT`,
`POWER`, `MONITOR`, `SCAN`, `FM RADIO`, `LOCK KEYPAD`, `SWITCH VFO`, `VFO/MR`, `SWITCH DEMODUL`
and `SPECTRUM`.

The spectrum analyzer is already on **`F` + `5`**; assigning `SPECTRUM` to a side key here is
just an alternative. Transmit-only actions (`VOX`, `ALARM`, `1750HZ`) are not offered.
