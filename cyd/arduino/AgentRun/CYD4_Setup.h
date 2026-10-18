// TFT_eSPI setup for the 4.0" ESP32 "Cheap Yellow Display" (ESP32-32E / E32R40T / E32N40T, ST7796 480x320)
// Pins per LCDWiki "4.0inch ESP32-32E Display" docs. Verify against your board's silkscreen/schematic.
#pragma once
#define USER_SETUP_ID 4040
#define ST7796_DRIVER
#define TFT_WIDTH  320
#define TFT_HEIGHT 480
#define TFT_RGB_ORDER TFT_BGR   // swap to TFT_RGB if red/blue are reversed
#define USE_HSPI_PORT
#define TFT_MISO 12
#define TFT_MOSI 13
#define TFT_SCLK 14
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST  -1   // tied to EN
#define TFT_BL   27
#define TFT_BACKLIGHT_ON HIGH
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define SMOOTH_FONT
#define SPI_FREQUENCY      40000000   // try 80000000 if stable
#define SPI_READ_FREQUENCY 16000000
