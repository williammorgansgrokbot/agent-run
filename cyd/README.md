# AGENT RUN: isometric spy-car game for the 4" ESP32 CYD

This is an original Spy Hunter-style road game. It uses a 2:1 isometric view (32x16 tiles), cars about 40 px long and RGB565 colour, matching the mockups in `../docs/images/mockups/`.
Target board: the 4.0" ESP32 "Cheap Yellow Display" (ESP32-WROOM-32E, ST7796 480x320 SPI). Input: a Bluetooth gamepad through **Bluepad32**.

## Board
Tested target: **Hosyond 4.0" 320x480 ESP32-32E display, ST7796S** (Amazon ASIN B0FGJJ24S1). This is the same board as LCDWiki **E32R40T / E32N40T**: ESP32-WROOM-32E (ESP32-D0WD-V3, classic BT + BLE), 4 MB flash, 4-wire SPI ST7796S, XPT2046 resistive touch.

## Boot screen and troubleshooting
- The backlight (GPIO27) and display are switched on in `app_main()` **before** Bluetooth starts. You should immediately see a blue "AGENT RUN / Starting Bluetooth..." screen.
  - If it stays on that screen, the Bluetooth stack failed to start.
  - If the backlight stays dark, the firmware isn't booting: check the flash offset and look at the serial log.
- At 115200 baud the serial log shows `[AGENT RUN] app_main: early boot...`, then `display init done`, then `setup() reached`.
- **Flash the merged image at offset 0x0** (not 0x1000). If you flash the separate files instead, use bootloader at 0x1000, partitions.bin at 0x8000 and firmware at 0x10000.

## Pins (from the LCDWiki E32R40T manual; they match this board)
These are the pins for the LCDWiki **4.0inch ESP32-32E Display** (E32R40T / E32N40T). They are set in `main/CYD4_Setup.h`:

| Signal | GPIO | Notes |
|---|---|---|
| TFT SCLK | 14 | HSPI native pins |
| TFT MOSI | 13 | |
| TFT MISO | 12 | |
| TFT CS | 15 | |
| TFT DC | 2 | |
| TFT RST | -1 | tied to EN |
| Backlight | 27 | active HIGH |
| (touch CS) | 33 | not used |
| Speaker (DAC out) | 26 | to the onboard FM8002E amp, 1.25 mm 2P speaker socket |
| Amp enable | 4 | active LOW (the game turns the amp off when silent) |
| RGB LED R / G / B | 22 / 16 / 17 | common anode, active LOW |

If your 4" board uses different pins, edit `main/CYD4_Setup.h`.
- If red and blue are swapped, change `TFT_RGB_ORDER` to `TFT_RGB`.
- If the image is mirrored or inverted, try `tft.setRotation(3)` in `setup()`, or add `#define TFT_INVERSION_ON` / `TFT_INVERSION_OFF`.
- `SPI_FREQUENCY` defaults to 40 MHz. **Try 80 MHz to reach about 30 fps.** A full 480x320x16 frame takes about 61 ms at 40 MHz and about 31 ms at 80 MHz. GPIO 12-15 are the HSPI IO_MUX pins, so 80 MHz usually works.

## Rendering
The game never keeps a full framebuffer, because 300 KB won't fit in RAM. Each frame is drawn into two 480x32 `TFT_eSprite` strips (about 30 KB each, 10 strips per frame). One strip is drawn while the other is sent out over SPI DMA (`pushImageDMA`).
- The ground is drawn as horizontal spans of constant-b road bands.
- Objects are sorted by isometric depth and only drawn into the strips they touch.

## Build setup (PlatformIO)
After cloning, run `./setup_deps.sh` first. It fetches `components/` (Bluepad32, the Arduino core, TFT_eSPI with the board CMakeLists) and `external/` (BTstack). These third-party folders are not committed to this repo. `managed_components/` is downloaded by the IDF component manager on the first build (see `dependencies.lock`).

This project is based on the official **esp-idf-arduino-bluepad32-template**. It uses ESP-IDF with Arduino-as-component and Bluepad32/BTstack as IDF components, on the pioarduino `platform-espressif32` 55.03.31-2 platform (see `platformio.ini`). TFT_eSPI is included as an IDF component in `components/TFT_eSPI`. Its Kconfig was removed, and the board setup is injected from `main/CYD4_Setup.h` by its `CMakeLists.txt`.

```bash
pip install "platformio==6.1.18"   # PlatformIO Core 6.2.x pulls SCons 4.11, which breaks this platform (FortranCommon error)
cd cyd
pio run -e esp32dev                 # build
pio run -e esp32dev -t upload       # flash (hold BOOT if it fails to connect)
pio device monitor -b 115200        # serial log: controller info, fps, heap
```
The first build downloads about 1 GB of toolchain and IDF and takes several minutes.

### Flash without building
`agentrun-cyd4-merged.bin` (attached to the [v1.0 GitHub Release](https://github.com/williammorgansgrokbot/agent-run/releases/tag/v1.0)) is a full image to write at offset 0x0:
```bash
esptool.py --chip esp32 -b 921600 write_flash 0x0 agentrun-cyd4-merged.bin
```
You can also use the browser-based https://espressif.github.io/esptool-js/ with offset 0x0.

### Arduino IDE (alternative; not compile-tested here)
1. Add the Bluepad32 board package URL `https://raw.githubusercontent.com/ricardoquesada/esp32-arduino-lib-builder/master/bluepad32_files/package_esp32_bluepad32_index.json` plus the official `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json` in Preferences → Additional Boards Manager URLs. Install **esp32** and **esp32_bluepad32**, then select *Tools → Board → ESP32 + Bluepad32 Arduino → ESP32 Dev Module*.
2. Install the **TFT_eSPI** library. In its `User_Setup_Select.h`, replace the default include with `#include "<path>/CYD4_Setup.h"`, or copy that file's defines into `User_Setup.h`.
3. Open `arduino/AgentRun/AgentRun.ino`. Set Partition Scheme to *Huge APP (3MB No OTA)* and upload.

## Controls
| Action | Input |
|---|---|
| Steer (across the road) | Left stick X / D-pad left-right |
| Speed up / slow down | Left stick Y / D-pad up-down |
| Fire machine gun | **A** (Xbox) / **Cross** (PS) / **B** (Nintendo layout, bottom face button), or right trigger |
| Special weapon (oil / smoke / missile / torpedo) | **B** / **Circle**, or X / Square |
| Pause / resume | **Start** / Options / + (Home also works) |
| Start game | A or Start on the title screen |
| Music on/off | **Select** / Back / View / Share / − |
| Volume down / up (0-10) | **L1 / R1** (LB / RB) |

The volume and music setting are saved in Preferences (`vol`, `music`) and kept across reboots.

The controller rumbles on bumps, deaths and weapon pickups when it supports rumble.

## Gameplay
- **Enemies:** red rammers come from behind and try to push you off the road. Black gunners sit ahead of you and shoot backwards; they take 2 hits.
- **Civilians:** hitting one costs 500 points and freezes your score for 4 s.
- **Weapons van:** drive into the back of the blue "W" van. You get 6 shots of OIL, SMOKE or MISSILE (TORPEDO in boat mode) and +30 fuel.
- **Hazards:** oil and smoke make cars spin out. If they spin off the road they explode.
- **Fuel:** the fuel bar drains steadily. Refills come from scene changes and the van. Running out costs a life.
- **Lives:** you start with 3 and get an extra life every 15,000 points.
- **Scenes:** about every 420 tiles the scene changes: HIGHWAY → NIGHT RUN → RIVER BRIDGE → CANYON SUNSET → SNOW PASS → DOWNTOWN (helicopter dropping bombs; only missiles hit it) → COAST - BOAT MODE (enemy boats, torpedoes) → back to the start.
- **High score:** saved in NVS (`Preferences`, namespace `agentrun`).

## Pairing a controller
Power the CYD on. Bluepad32 scans automatically (BR/EDR and BLE are both enabled). Put the controller in pairing mode:

| Controller | Pairing |
|---|---|
| **JOWUA Tesla multi-device controller** | Set the "Multi-Link" switch to **S** (Switch Pro mode, best supported). Hold the **Pairing** button for about 3 s until the 2nd LED flashes fast. It shows up as "Pro Controller", and the LED stays lit once connected. If that fails, try **X** (Xbox) mode with the same pairing button. Factory reset: hold **Home** for more than 10 s. |
| Xbox One / Series (BT models) | Hold the small **Pair** button on top until the Xbox logo flashes fast. Update the controller firmware for BLE support. |
| PS4 DualShock 4 | Hold **Share + PS** until the light bar flashes. |
| PS5 DualSense | Hold **Create + PS** until the LEDs around the touchpad flash. |
| Switch Pro | Hold the small **Sync** button on top. |
| 8BitDo | Usually hold **Start + X** for Switch/X-input mode or **Start + B** for Android mode, then hold **Pair** for about 3 s. It varies by model. |

The title screen shows `PAD: <model>` once connected. If a controller won't re-pair after pairing to another device, uncomment `BP32.forgetBluetoothKeys()` in `setup()` for one boot.

### Serial debug
At 115200 baud, connecting a controller prints its model, VID/PID, type, BT address and raw buttons. Each change in buttons, D-pad or misc buttons prints the raw values (`[PAD] dpad=.. buttons=.. misc=..`); set `DEBUG_INPUT 0` to turn this off. The fps and free heap are printed every 5 s. Bluepad32's own console is disabled (`CONFIG_BLUEPAD32_USB_CONSOLE_ENABLE=n`) so that Arduino `Serial` works.

## Sound and LED
- **Sound:** an 8-bit synth in `main/audio_synth.cpp` + `main/audio_esp32.cpp` runs as its own FreeRTOS task on core 0 at priority 2. The game itself runs on core 1, so audio doesn't slow the frame rate.
  - Output: the ESP-IDF continuous DAC driver (I2S0 + DMA) on GPIO26 at 22.05 kHz, 8-bit.
  - Voices: a 25% pulse lead, a triangle walking bass and noise drums, plus one sound-effect voice mixed on top. Higher-priority effects (crash, explosion) override lower ones (gun).
  - Effects: gun, bump, oil/smoke drop, missile/torpedo, pickup, boat horn, explosion, crash.
- **Music (all original):** a lazy swung A-minor theme at 96 bpm on the title screen, a driving E-minor spy-jazz walking-bass theme at 152 bpm in game, and a short descending phrase on game over.
- **Previews:** WAV previews rendered from the same synth code are in `../docs/audio/`.
- **RGB LED:** a short green blink per gun shot, red on a crash, blue on a weapon pickup or entering boat mode.
