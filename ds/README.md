# AGENT RUN — Nintendo DS / DSi / 3DS homebrew

`agentrun.nds` is a devkitARM + libnds 2 (calico) build. It is a DS+DSi hybrid with a 32x32 icon and the title
"AGENT RUN / Spy driving game / CYD Game". This DS port is self-contained: everything is in `source/`.

## Install
Copy `agentrun.nds` to the SD card or flashcart and check the copied file is about 350 KB. Launch it from
TWiLight Menu++ (recommended on DSi/3DS) or the latest nds-hb-menu. Saves go to `sd:/agentrun.sav` in DSi mode,
otherwise to `fat:/agentrun.sav`.

## Controls
D-pad left/right steers, up/down sets speed. A/Y fires, B/X uses the special weapon, START starts/pauses.
SELECT toggles music, and L/R change the volume (saved).
The bottom line of the touch screen shows `FPS nn avg pk<peak> L B S H A`: per-frame milliseconds for
logic, BG streaming, sprites, HUD and audio, averaged over 60 frames.

## How it runs at 60 fps on a 67 MHz ARM9
* Top screen, BG3: an 8bpp 512x256 wrapping bitmap holds the ground and scenery and is scrolled by hardware.
  Content is rendered ahead of the camera in five small chunks spread over five frames. Each chunk is a
  strip clipped to its rectangle, rendered into a RAM shadow and copied to VRAM, so no frame redraws the
  scene. A full redraw only happens on a new game or a scene change.
* Ground: integer span maths in iso (u = x+2y, v = x-2y) space, with no floats.
* Cars, bullets, heli, explosions and smoke are hardware OAM sprites (8bpp), pre-rendered from the original
  box-sprite code at each scene change.
* Overlay text (title, banner, game over) lives on BG0 tiles and is re-tiled only when it changes.
* HUD (sub screen, 16bpp bitmap): text fields are redrawn only when their value changes, the radar at 15 Hz,
  using 32-bit fills.
* Canvas primitives: colours are converted once per primitive, triangles use 16.16 fixed-point edges, and
  spans are drawn with memset.
* Audio: the same original tunes and SFX on hardware channels. Lead is a PSG 25% square, bass a looped
  32-sample triangle PCM, drums use noise/PSG channels, and SFX use PSG + noise with swept timers. The ARM9
  only runs a step sequencer and envelopes; there is no per-sample synthesis.
* Exactly one `swiWaitForVBlank` per frame.

## Build
`make` with DEVKITPRO/DEVKITARM set.
* `make EXTRA_CFLAGS=-DAR_DEMO` builds a self-playing demo.
* Add `-DAR_START_SCENE=n` (0-6) to start in a given scene.
