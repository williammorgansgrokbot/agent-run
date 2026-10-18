// SPDX-License-Identifier: Apache-2.0
// Copyright 2019 Ricardo Quesada
// http://retro.moe/unijoysticle2

#include "sdkconfig.h"

#include <stddef.h>

// BTstack related
#include <btstack_port_esp32.h>
#include <btstack_run_loop.h>
#include <btstack_stdio_esp32.h>

// Bluepad32 related
#include <arduino_platform.h>
#include <uni.h>
#include "driver/gpio.h"
#include "esp_rom_sys.h"

// AGENT RUN: bring the display up before Bluetooth so the user always sees something.
extern void agentrun_early_display(void);

//
// Autostart
//
#if CONFIG_AUTOSTART_ARDUINO
void initBluepad32() {
#else
int app_main(void) {
#endif  // !CONFIG_AUTOSTART_ARDUINO
    // hci_dump_open(NULL, HCI_DUMP_STDOUT);

// Don't use BTstack buffered UART. It conflicts with the console.
    esp_rom_printf("\n[AGENT RUN] app_main: early boot, backlight GPIO27 -> HIGH\n");
    gpio_reset_pin(GPIO_NUM_27);
    gpio_set_direction(GPIO_NUM_27, GPIO_MODE_OUTPUT);
    gpio_set_level(GPIO_NUM_27, 1);
    agentrun_early_display();
    esp_rom_printf("[AGENT RUN] display init done, starting Bluetooth...\n");

    // AGENT RUN: btstack_stdio_init() removed. It installs its own UART0 driver/task, which conflicts with
    // Arduino Serial.begin() and asserts if any byte arrives on UART0 before the BTstack run loop exists.

    // Configure BTstack for ESP32 VHCI Controller
    btstack_init();

    // hci_dump_init(hci_dump_embedded_stdout_get_instance());

    // Must be called before uni_init()
    uni_platform_set_custom(get_arduino_platform());

    // Init Bluepad32.
    uni_init(0 /* argc */, NULL /* argv */);

    // Does not return.
    btstack_run_loop_execute();
#if !CONFIG_AUTOSTART_ARDUINO
    return 0;
#endif  // !CONFIG_AUTOSTART_ARDUINO
}
