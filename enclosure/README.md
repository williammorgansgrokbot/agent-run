# CYD "1978 Curtis Mathes" console-TV cabinet: E32R40T 4.0" ESP32-32E

This is a parametric OpenSCAD enclosure for the Hosyond / LCDWiki **E32R40T**: an ST7796S 480x320 SPI display with XPT2046 resistive touch and an ESP32-32E module.
The board is mounted **landscape** with the USB-C end on the left, so the screen shows 83.5 x 55.7 mm (3:2).
Turn on landscape rotation in your firmware (TFT_eSPI `setRotation(1)` or `(3)`) and recalibrate the touch.

## Files
* `cyd_console_tv.scad`: the model. All dimensions are parameters at the top. Choose what to build with `-D 'part="..."'`.
* `stl/`: each printable part, already placed in its print orientation.
* `previews/`: renders made with `openscad --render` (manifold backend):
  * `front_3q.png`: main front 3/4 view.
  * `front_3q_left_usb_side.png`: front 3/4 view from the left, showing the USB-C hole.
  * `back.png`: rear view.
  * `exploded.png`: exploded view.
  * `front_3q_legs_variant.png`: the version with legs.
  * `section_fit.png`: horizontal section through the USB-C/LED plane, showing the fit.
  * `front_straight.png`: straight-on front view.
* `render_previews.sh`, `export_stl.sh`: these rebuild everything (`OPENSCAD=openscad ./export_stl.sh` also works with stock 2021.01, but it is slow).
* `src/`: `step_bb.py` and `step_bb.txt` hold the component bounding boxes measured from the LCDWiki STEP model. The LCDWiki source files themselves (outline drawing PDF, spec PDF, STEP zip) are **not redistributed** here because their licence is unclear. Download them from the [LCDWiki E32R40T page](http://www.lcdwiki.com/4.0inch_ESP32-32E_Display) and put `E32R40T_3D/E32R40T_3D.step` next to `step_bb.py` to re-run it.

## Parts
| STL | qty | colour / material | print orientation | supports |
|---|---|---|---|---|
| front_shell.stl | 1 | wood PLA | **front face down**, open back up | none |
| back_panel.stl | 1 | wood PLA or dark PLA | outer face down, guide tubes up | none |
| bezel.stl (CRT mask ring) | 1 | black (or silver) PLA | back face down | none |
| knob_channel.stl, knob_volume.stl | 1 each | silver/"chrome" PLA | face down, peg up | none |
| plinth.stl (default base) | 1 | black / dark walnut | upright, pegs up | none |
| leg.stl (alternative base) | 4 | dark wood, gold-painted ferrule | foot down, peg up | none |
| plunger.stl (RESET/BOOT push-rods) | 2 | any | flange down | none |
| light_pipe.stl | 1 | clear/natural PETG, or a 3 mm acrylic rod (~31.5 mm) | upright | none |

Hardware:
* 4x M2.5x6 self-tapping screws hold the PCB to the front-shell standoffs.
* 6x M2.5x8 pan-head self-tapping screws hold the back panel.
* You can use M2.5 heat-set inserts instead: set `boss_pilot_d=3.6` (and preferably `boss_d=7`), and `pilot_d=3.6` for M2.5x4 inserts in the PCB standoffs. For M2, use `boss_pilot_d=1.7`.
* An 8 ohm speaker, 28 to 40 mm (default `spk_d=36`), with a 1.25 mm 2P plug. The board drives up to 1.5 W at 8 ohm.

## Overall dimensions (defaults)
* Body: 176.2 W x 84 H x 40 D mm. The top slab overhangs 1.5 mm each side, so the widest point is 179.2 mm.
* Height is 93 mm on the plinth (9 mm toe-kick) or 95 mm on the legs (11 mm).
* The front shell fits a 180 x 84 mm print footprint, so it suits any 220 mm bed.
* The screen window is a squircle (n=7), 85.2 x 56.8 mm. It is bigger than the 83.52 x 55.68 active area (only the very corners are masked, the way a CRT is) and smaller than the 85.22 x 56.88 touch visible area.

## Print settings
* 0.4 mm nozzle (use 0.5 to 0.6 for wood-fill filament to avoid clogs), 0.2 mm layers (0.12 to 0.16 for the knobs).
* 3 to 4 perimeters (2 mm walls), 15 to 20 % gyroid infill, no supports on any part.
* Brim is optional on the front shell. Its bed contact is the whole front face.
* The front shell is designed to print face-down. All front decoration is engraved (trim lines, grille louvres, dial ticks, nameplate, bezel recess), so nothing overhangs. The window and louvres have 45° chamfers. The bosses, standoffs, ribs, speaker ring and leg rails all grow straight up from the front wall.
* Finish:
  * **Wood-fill PLA** (e.g. 30 % wood fibre). Vary the nozzle temperature 200 to 225 °C every few mm ("tiger stripe" g-code script) to get darker grain bands. The model also has shallow grain grooves on the top, sides and front rails (`woodgrain=true`).
  * Or use plain PLA in walnut or tan, sand it, then apply a gel stain or a "faux bois" paint glaze.
  * Pick out the double trim line and the nameplate letters with a gold paint pen.
  * Satin-black bezel, silver knobs, black plinth.

## Assembly
1. Glue the bezel ring into its 0.6 mm recess, and press or glue the knobs into their peg holes.
2. Hot-glue the speaker into the ring behind the grille, then plug it into the SPEAKER socket.
3. Drop the PCB in screen-first. The perimeter ribs locate it, and the touch glass sits 0.1 mm behind the front wall so the resistive touch is not pre-loaded. Screw it down with 4x M2.5 screws through the PCB holes.
4. Put the 2 plungers into the back-panel tubes from the inside. Insert the light pipe from outside.
5. Screw the back panel onto the 6 bosses.
6. Push the plinth pegs (or the 4 legs) into the floor sockets.

There is room for a small LiPo pack (up to roughly 50 x 30 x 10 mm) behind the PCB. It plugs into the BAT socket.

## Board data used (all from LCDWiki unless noted)
Sources (fetched 2026-10-17):
* https://www.lcdwiki.com/4.0inch_ESP32-32E_Display
* https://www.lcdwiki.com/res/E32R40T/E32R40T_Size.pdf ("LCM OUTLINE E32R40T V1.0", 2025-04-15)
* https://www.lcdwiki.com/res/E32R40T/E32R40T_3D.zip (STEP)
* The board interface image E32R40T-04.png

The u/v coordinates below are measured on the board. **u** is the distance from the USB-C short edge. **v** is the distance from the long edge that carries the I2C/SPI/SPEAKER/IO35 sockets (this edge is at the bottom in the cabinet).
* PCB: 60.88 x 111.11 x 1.60 mm, corner R3.5. Total module thickness is 5.65 mm. LCD+RTP+tape sits 4.05 mm above the PCB front face.
* Mounting holes: 4 x ø3.2 (pad ø6.6), on 104.11 x 53.28 mm pitch. They are 3.50 mm from the short edges and 3.80 mm from the long edges.
* LCD (backlight) outline: 94.57 x 60.88 mm, starting 8.27 mm from each short edge.
* Active area: 83.52 x 55.68 mm, at u = 16.67 to 100.19, v = 2.60 to 58.28. It is 10.92 mm from the antenna edge, so it is offset 2.9 mm toward the antenna end.
* RTP visible area: 85.22 x 56.88 mm.
* USB-C: centred on the short edge (v = 30.44; the drawing gives 15.38 + 15.06), 8.94 wide. It is a mid-mount part spanning 0.50 to 4.88 mm behind the PCB front face and overhangs the edge by 0.37 mm.
* RESET (EN) and BOOT (IO0): top-actuated tact switches on the **back** of the board, 3.26 mm from the USB edge, 15.38 mm in from each long edge, 2.5 mm tall. RESET is the one on the speaker-socket side.
  * The cabinet reaches them with captive push-rods through the back panel. You only need BOOT if auto-download fails.
* RGB LED: a 5050 on the **back** of the PCB at u = 52.6, v = 29.9, 1.6 mm tall (from the STEP).
  * It faces the rear, so the cabinet uses a straight light pipe to a "pilot lamp" window in the back panel. A front light pipe would have to go round the PCB, and a printed bent pipe would barely glow, so I didn't add one.
* Speaker socket (1.25 mm 2P): on the bottom long edge at u = 48.2. I2C is at u = 93.1, SPI at u = 70.1, IO35/39 at u = 29.4.
* UART (u = 44.9), BAT (u = 25.5) and microSD (u ≈ 67.5) are on the top long edge.
* Tallest rear part is 4.7 mm (speaker socket).

## Assumptions / unverified (check with calipers before a final print)
* **microSD socket position is estimated** from the board artwork, because it is not in the STEP. The SD card is **not reachable** with the board installed: there is 7.6 mm of clearance above the top edge, and a card needs ~15 mm. Take the board out to change cards, or use USB/OTA.
* Button and LED positions come from the STEP model and agree with the drawing to 0.05 mm. The plunger tips are 0.35 mm above the buttons with about 0.9 mm of travel.
* The LCD stack thickness is ±0.1 to 0.2 mm (drawing tolerances). If the screen rattles, lower `standoff_gap` from 0.10 to 0. If touch registers phantom presses, raise it.
* I assumed the front side of the PCB around the 4 mounting holes is clear for ø5.5 standoffs. The drawing shows nothing there.
* The PCB antenna notch was ignored, and the board is treated as a full rectangle. The cabinet is plastic only near the antenna, which sits at the right end behind the control strip side.
* The speaker socket may be vertical or side-entry, and the STEP is ambiguous. There is 7 mm of free space below the board edge either way.
* I checked fit numerically: there is no overlap between the shell/panel/plungers/pipe and a board dummy built from the STEP component boxes, and none between the shell and the back panel (`part="clash"` and `part="clash2"` both render empty). This has **not** been test-printed.
* "CURTIS MATHES" on the nameplate is a decorative homage. Change it with `brand_text`.
