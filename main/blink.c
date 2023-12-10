#include "blink.h"


void blinkLed(int blinkCount) {
    for (int i = 0; i < blinkCount; i++) {
        control_Led(i % 2 == 0);  // Toggle the LED state
        vTaskDelay(BLINK_INTERVAL_MS / portTICK_PERIOD_MS);
    }

    // Ensure LED is turned off at the end
    control_Led(false);
}


void control_Led(bool turnOn) {
    gpio_set_level(LED_GPIO_PIN, turnOn ? 1 : 0);
}