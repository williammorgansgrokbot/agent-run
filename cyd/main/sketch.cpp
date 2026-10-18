// AGENT RUN - isometric spy-car road game for the 4" ESP32 CYD (ST7796 480x320)
// Input: Bluetooth gamepad via Bluepad32. Rendering: TFT_eSPI, 480x32 strips, DMA ping-pong.
#include "sdkconfig.h"
#include <Arduino.h>
#include <Bluepad32.h>
#include <Preferences.h>
#include <TFT_eSPI.h>
#include <math.h>
#include "game_gfx.h"
#include "audio.h"

// Onboard RGB LED (common anode -> active LOW), LCDWiki E32R40T
static constexpr int LED_R = 22, LED_G = 16, LED_B = 17;
static float ledT[3] = {0, 0, 0};
static void ledFlash(int c, float sec) { if (ledT[c] < sec) ledT[c] = sec; }
static void ledUpdate(float dt) {
  const int pins[3] = {LED_R, LED_G, LED_B};
  for (int i = 0; i < 3; i++) { if (ledT[i] > 0) ledT[i] -= dt; digitalWrite(pins[i], ledT[i] > 0 ? LOW : HIGH); }
}
static uint8_t volSetting = 6; static bool musicSetting = true;

#define DEBUG_INPUT 1   // print raw controller state changes on Serial (115200)

TFT_eSPI tft;
TFT_eSprite sprA(&tft), sprB(&tft);
Canvas* G = nullptr;
int YO = 0;
Preferences prefs;

#include "game_core.h"

InState in;

// ------------------------------------------------------------ input (Bluepad32)
ControllerPtr pads[BP32_MAX_GAMEPADS];
static String padName = "";

void onConnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) if (!pads[i]) {
    pads[i] = ctl;
    ControllerProperties p = ctl->getProperties();
    Serial.printf("[BP32] connected idx=%d model=\"%s\" VID=0x%04x PID=0x%04x type=%d gamepad=%d\n", i,
                  ctl->getModelName().c_str(), p.vendor_id, p.product_id, p.type, ctl->isGamepad());
    Serial.printf("[BP32] btaddr=%02X:%02X:%02X:%02X:%02X:%02X flags=0x%08lx\n", p.btaddr[0], p.btaddr[1], p.btaddr[2],
                  p.btaddr[3], p.btaddr[4], p.btaddr[5], (unsigned long)p.flags);
    Serial.printf("[BP32] raw on connect: dpad=0x%02x buttons=0x%04x misc=0x%02x axL=%d,%d\n", ctl->dpad(), ctl->buttons(),
                  ctl->miscButtons(), ctl->axisX(), ctl->axisY());
    padName = ctl->getModelName();
    ctl->playDualRumble(0, 200, 0x80, 0x80);
    ctl->setPlayerLEDs(1);
    return;
  }
  Serial.println("[BP32] connected but no free slot");
}
void onDisconnectedController(ControllerPtr ctl) {
  for (int i = 0; i < BP32_MAX_GAMEPADS; i++) if (pads[i] == ctl) { pads[i] = nullptr; Serial.printf("[BP32] disconnected idx=%d\n", i); }
  padName = "";
  if (state == ST_PLAY) state = ST_PAUSE;
}
static ControllerPtr activePad() {
  for (auto p : pads) if (p && p->isConnected() && p->isGamepad()) return p;
  return nullptr;
}
static void rumble(uint8_t strong, uint16_t ms) { if (auto p = activePad()) p->playDualRumble(0, ms, strong, strong / 2); }

static void readInput() {
  BP32.update();
  InState n{};
  ControllerPtr c = activePad();
  if (c) {
    int ax = c->axisX(), ay = c->axisY();
    uint8_t d = c->dpad();
    n.sx = abs(ax) > 90 ? clampf(ax / 450.f, -1, 1) : 0;
    n.sy = abs(ay) > 90 ? clampf(-ay / 450.f, -1, 1) : 0;   // up = faster
    if (d & DPAD_LEFT) n.sx = -1; if (d & DPAD_RIGHT) n.sx = 1;
    if (d & DPAD_UP) n.sy = 1;    if (d & DPAD_DOWN) n.sy = -1;
    n.fire = c->a() || c->throttle() > 300;   // A / Cross (or right trigger)
    n.special = c->b() || c->x();             // B / Circle (X/Square also works)
    n.start = c->miscStart() || c->miscSystem();
    n.sel = c->miscSelect(); n.l1 = c->l1(); n.r1 = c->r1();
#if DEBUG_INPUT
    static uint16_t lb = 0; static uint8_t ld = 0, lm = 0;
    if (c->buttons() != lb || d != ld || c->miscButtons() != lm) {
      lb = c->buttons(); ld = d; lm = c->miscButtons();
      Serial.printf("[PAD] dpad=0x%02x buttons=0x%04x misc=0x%02x axL=%4d,%4d axR=%4d,%4d brake=%d thr=%d\n", ld, lb, lm,
                    ax, ay, c->axisRX(), c->axisRY(), c->brake(), c->throttle());
    }
#endif
  }
  n.fireEdge = n.fire && !in.fire; n.specialEdge = n.special && !in.special; n.startEdge = n.start && !in.start;
  n.selEdge = n.sel && !in.sel; n.l1Edge = n.l1 && !in.l1; n.r1Edge = n.r1 && !in.r1;
  in = n;
}


// ---- platform hooks for game_core.h
void plat_rumble(uint8_t strong, uint16_t ms) { rumble(strong, ms); }
void plat_led(int c, float sec) { ledFlash(c, sec); }
void plat_saveHi(uint32_t hi) { prefs.putUInt("hi", hi); }
void plat_log(const char* m) { Serial.println(m); }

static void drawHUD() {
  const Scene& s = SC();
  int y = PLAY_H;
  rect(0, y, SCR_W, HUD_H, s.hudBg); rect(0, y, SCR_W, 2, s.hudAcc);
  char buf[16];
  txt("SCORE", 6, y + 9, s.hudAcc, 1); snprintf(buf, sizeof(buf), "%06lu", (unsigned long)score);
  txt(buf, 40, y + 6, noScoreT > 0 ? RGB(248,80,80) : s.hudFg, 2);
  txt("LIVES", 128, y + 9, s.hudAcc, 1);
  for (int i = 0; i < min(lives, 5); i++) { int x = 162 + i * 14; rect(x, y + 11, 11, 8, TFT_WHITE); rect(x + 3, y + 8, 5, 3, RGB(80,120,200)); }
  txt("WPN", 236, y + 9, s.hudAcc, 1);
  if (pl.sp == SP_NONE) txt("MG", 258, y + 6, s.hudFg, 2);
  else { snprintf(buf, sizeof(buf), "%s x%d", SPNAME[pl.sp], pl.ammo); txt(buf, 258, y + 6, s.hudFg, 2); }
  txt("FUEL", 394, y + 9, s.hudAcc, 1);
  G->drawRect(420, y + 7 - YO, 54, 13, s.hudFg);
  int w = (int)(fuel * 50 / 100); rect(422, y + 9, w, 9, fuel < 25 && ((millis() / 250) & 1) ? RGB(248,40,40) : s.hudAcc);
}

static void drawOverlay() {
  const Scene& s = SC();
  if (state != ST_OVER && banner > 0 && ((int)(banner * 4) & 1 || banner > 1)) txt(bannerText, SCR_W / 2, 10, RGB(248,232,120), 2, TC_DATUM, RGB(120,40,80));
  if (state == ST_PAUSE) { txt("PAUSED", SCR_W / 2, 110, TFT_WHITE, 4, TC_DATUM, TFT_BLACK);
    txt(activePad() ? "PRESS START TO RESUME" : "CONTROLLER LOST - RECONNECT", SCR_W / 2, 160, s.hudAcc, 2, TC_DATUM, TFT_BLACK); }
  if (state == ST_TITLE) {
    txt("AGENT RUN", SCR_W / 2, 30, RGB(248,220,40), 6, TC_DATUM, RGB(200,0,120));
    txt("ROAD SPY 2026", SCR_W / 2, 84, RGB(0,248,248), 2, TC_DATUM);
    txt("STICK / DPAD: STEER + SPEED", SCR_W / 2, 186, TFT_WHITE, 2, TC_DATUM, TFT_BLACK);
    txt("A: FIRE    B: SPECIAL    START: PAUSE", SCR_W / 2, 206, TFT_WHITE, 1, TC_DATUM, TFT_BLACK);
    txt("SELECT: MUSIC ON/OFF    L1/R1: VOLUME", SCR_W / 2, 218, RGB(180,180,200), 1, TC_DATUM, TFT_BLACK);
    ControllerPtr c = activePad();
    if (c) { if ((millis() / 400) & 1) txt("> PRESS A TO START <", SCR_W / 2, 232, RGB(248,0,200), 2, TC_DATUM, TFT_BLACK);
             txt(("PAD: " + padName).c_str(), SCR_W / 2, 258, RGB(120,248,120), 1, TC_DATUM); }
    else { txt("PAIR A BLUETOOTH CONTROLLER", SCR_W / 2, 232, RGB(248,140,64), 2, TC_DATUM, TFT_BLACK);
           txt("(PUT IT IN PAIRING MODE - SEE README)", SCR_W / 2, 258, RGB(200,200,200), 1, TC_DATUM); }
    char hb[24]; snprintf(hb, sizeof(hb), "HI %06lu", (unsigned long)hiScore); txt(hb, SCR_W / 2, 276, RGB(160,160,200), 1, TC_DATUM);
  }
  if (state == ST_OVER) {
    txt("GAME OVER", SCR_W / 2, 70, RGB(248,64,64), 5, TC_DATUM, TFT_BLACK);
    char b[32]; snprintf(b, sizeof(b), "SCORE %06lu", (unsigned long)score); txt(b, SCR_W / 2, 130, TFT_WHITE, 3, TC_DATUM, TFT_BLACK);
    snprintf(b, sizeof(b), "HI %06lu", (unsigned long)hiScore); txt(b, SCR_W / 2, 164, RGB(160,160,200), 2, TC_DATUM, TFT_BLACK);
    if (newHi && ((millis() / 300) & 1)) txt("NEW HIGH SCORE!", SCR_W / 2, 192, RGB(248,220,40), 2, TC_DATUM, TFT_BLACK);
    txt("PRESS A", SCR_W / 2, 230, RGB(248,0,200), 2, TC_DATUM, TFT_BLACK);
  }
}

static void renderFrame() {
  buildDrawList();
  tft.startWrite();
  for (int s = 0; s < NSTRIPS; s++) {
    G = (s & 1) ? &sprB : &sprA;
    YO = s * STRIP_H;
    int y1 = YO + STRIP_H;
    if (YO < PLAY_H) {
      drawGround(YO, min(y1, PLAY_H));
      for (int i = 0; i < ndl; i++) if (dl[i].ymax >= YO && dl[i].ymin < y1) drawItem(dl[i]);
    }
    if (y1 > PLAY_H) drawHUD();
    drawOverlay();
    tft.pushImageDMA(0, YO, SCR_W, STRIP_H, (uint16_t*)G->getPointer());
  }
  tft.dmaWait();
  tft.endWrite();
}

// ------------------------------------------------------------ Arduino entry points
// Called from app_main() BEFORE Bluetooth starts (see main.c), so a BT failure still leaves a visible screen.
static bool displayUp = false;
extern "C" void agentrun_early_display(void) {
  displayUp = true;
  pinMode(TFT_BL, OUTPUT); digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  tft.init();
  tft.setRotation(1);   // landscape 480x320 (use 3 to flip 180 deg)
  tft.fillScreen(TFT_NAVY);
  tft.setTextColor(TFT_YELLOW, TFT_NAVY); tft.setTextDatum(MC_DATUM);
  tft.drawString("AGENT RUN", 240, 130, 4);
  tft.setTextColor(TFT_WHITE, TFT_NAVY);
  tft.drawString("Starting Bluetooth...", 240, 180, 2);
  tft.drawString("(stuck here = Bluetooth init problem, check serial 115200)", 240, 205, 1);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n[AGENT RUN] setup() reached - Bluetooth stack is up");
  if (!displayUp) agentrun_early_display();   // Arduino IDE build: no main.c, so init here
  digitalWrite(TFT_BL, TFT_BACKLIGHT_ON);
  initScenes();
  sprA.setColorDepth(16); sprB.setColorDepth(16);
  if (!sprA.createSprite(SCR_W, STRIP_H) || !sprB.createSprite(SCR_W, STRIP_H)) {
    Serial.println("[AGENT RUN] sprite alloc failed!"); tft.drawString("SPRITE ALLOC FAILED", 10, 10, 2); while (1) delay(1000);
  }
  tft.initDMA();
  prefs.begin("agentrun", false);
  hiScore = prefs.getUInt("hi", 0);
  volSetting = prefs.getUChar("vol", 6); musicSetting = prefs.getBool("music", true);
  for (int p : {LED_R, LED_G, LED_B}) { pinMode(p, OUTPUT); digitalWrite(p, HIGH); }
  audioBegin(volSetting, musicSetting);
  audioSetSong(SONG_TITLE);
  rngState ^= esp_random();

  BP32.setup(&onConnectedController, &onDisconnectedController);
  BP32.enableBLEService(true);              // BLE gamepads (Xbox Series, 8BitDo BLE, some generic HID)
  BP32.enableNewBluetoothConnections(true);
  // BP32.forgetBluetoothKeys();            // uncomment once if a controller refuses to re-pair
  Serial.printf("[AGENT RUN] free heap %u, largest block %u\n", (unsigned)ESP.getFreeHeap(), (unsigned)ESP.getMaxAllocHeap());
  memset(ents, 0, sizeof(ents)); pl = {}; pl.speed = 4; sceneIdx = 1;
}

void loop() {
  static uint32_t last = millis(), fpsT = 0; static int frames = 0;
  uint32_t now = millis();
  float dt = clampf((now - last) / 1000.f, 0.001f, 0.05f); last = now;
  readInput();
  if (banner > 0) banner -= dt;
  ledUpdate(dt);
  if (in.selEdge) { musicSetting = !musicSetting; audioSetMusic(musicSetting); prefs.putBool("music", musicSetting); setBanner(musicSetting ? "MUSIC ON" : "MUSIC OFF", 1.2f); }
  if (in.l1Edge || in.r1Edge) {
    int v = (int)volSetting + (in.r1Edge ? 1 : -1); volSetting = (uint8_t)(v < 0 ? 0 : (v > 10 ? 10 : v));
    audioSetVolume(volSetting); prefs.putUChar("vol", volSetting);
    char b[20]; snprintf(b, sizeof(b), "VOLUME %d", volSetting); setBanner(b, 1.2f); audioSfx(SFX_GUN);
  }
  switch (state) {
    case ST_TITLE:
      // attract mode: cruise through the night scene with some traffic
      pl.a += 4 * dt; camA = pl.a + 2; tGame += dt; spawnT -= dt;
      if (spawnT <= 0) { spawnT = 1.5f; if (Ent* e = spawn(E_CIV, camA + 21, laneB(rnd() % 4))) { e->va = 2; e->col = RGB(248,40,120); } }
      for (auto& e : ents) if (e.t) { e.a += e.va * dt; if (e.a - camA < -22) e.t = E_NONE; }
      if (in.fireEdge || in.startEdge) newGame();
      break;
    case ST_PLAY: update(dt); if (in.startEdge && state == ST_PLAY) state = ST_PAUSE; break;
    case ST_PAUSE: if (in.startEdge && activePad()) state = ST_PLAY; break;
    case ST_OVER: tGame += dt; if (in.fireEdge || in.startEdge) { state = ST_TITLE; audioSetSong(SONG_TITLE); memset(ents, 0, sizeof(ents)); sceneIdx = 1; pl.dead = 0; } break;
  }
  renderFrame();
  frames++;
  if (now - fpsT > 5000) { Serial.printf("[AGENT RUN] fps %.1f heap %u\n", frames * 1000.f / (now - fpsT), (unsigned)ESP.getFreeHeap()); frames = 0; fpsT = now; }
  // frame cap ~30 fps
  uint32_t spent = millis() - now; if (spent < 33) delay(33 - spent); else delay(1);
}
