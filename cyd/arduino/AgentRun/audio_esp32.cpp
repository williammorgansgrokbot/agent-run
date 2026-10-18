// AGENT RUN audio backend for the ESP32 CYD: continuous DAC (I2S0+DMA) on GPIO26, task on core 0.
#include "audio.h"
#include <Arduino.h>
#include "driver/dac_continuous.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static constexpr int SR = 22050;
static constexpr int AMP_EN = 4;     // FM8002E amp enable, active LOW (LCDWiki E32R40T)
static constexpr int BLOCK = 256;

static void audioTask(void*) {
  dac_continuous_handle_t h;
  dac_continuous_config_t cfg = {};
  cfg.chan_mask = DAC_CHANNEL_MASK_CH1;   // CH1 = GPIO26 on ESP32
  cfg.desc_num = 4; cfg.buf_size = 2048; cfg.freq_hz = SR; cfg.offset = 0;
  cfg.clk_src = DAC_DIGI_CLK_SRC_DEFAULT; cfg.chan_mode = DAC_CHANNEL_MODE_SIMUL;
  if (dac_continuous_new_channels(&cfg, &h) != ESP_OK || dac_continuous_enable(h) != ESP_OK) {
    Serial.println("[AUDIO] DAC init failed - sound disabled"); vTaskDelete(nullptr);
  }
  Serial.println("[AUDIO] DAC GPIO26 @22050 Hz running");
  static uint8_t buf[BLOCK];
  bool ampOn = false;
  for (;;) {
    audioSynthRender(buf, BLOCK);
    bool want = audioWantAmp();
    if (want != ampOn) { ampOn = want; gpio_set_level((gpio_num_t)AMP_EN, ampOn ? 0 : 1); }
    dac_continuous_write(h, buf, BLOCK, nullptr, -1);
  }
}

void audioBegin(uint8_t v, bool mOn) {
  audioSynthInit(SR);
  audioSynthSetup(v, mOn);
  gpio_reset_pin((gpio_num_t)AMP_EN); gpio_set_direction((gpio_num_t)AMP_EN, GPIO_MODE_OUTPUT); gpio_set_level((gpio_num_t)AMP_EN, 1);
  xTaskCreatePinnedToCore(audioTask, "audio", 4096, nullptr, 2, nullptr, 0);
}
