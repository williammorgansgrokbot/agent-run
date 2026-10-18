set(srcs
        "main.c"
        "sketch.cpp"
        "audio_synth.cpp"
        "audio_esp32.cpp")

set(requires "bluepad32" "bluepad32_arduino" "arduino" "btstack" "TFT_eSPI" "esp_driver_dac" "esp_driver_gpio")

idf_component_register(SRCS "${srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES "${requires}")
