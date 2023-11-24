#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "DHT.h"

#define DHT_GPIO_PIN 4
#define RELAY_GPIO_PIN 25  // Replace with the actual GPIO pin connected to the relay
#define LED_GPIO_PIN 26    // Replace with the actual GPIO pin connected to the LED
#define HUMIDITY_THRESHOLD 70.0
#define HYSTERESIS 5.0  // Adjust this value as needed
#define PUMP_DURATION_SECONDS 5
#define BLINK_INTERVAL_MS 200  // Blink interval for LED

void controlWaterPump(bool turnOn) {
    gpio_set_level(RELAY_GPIO_PIN, turnOn ? 1 : 0);
}

void controlLed(bool turnOn) {
    gpio_set_level(LED_GPIO_PIN, turnOn ? 1 : 0);
}

void blinkLed(int blinkCount) {
    for (int i = 0; i < blinkCount; i++) {
        controlLed(i % 2 == 0);  // Toggle the LED state
        vTaskDelay(BLINK_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}


void dht_task(void *pvParameter) {
    bool pumpActive = false;

    // Blink the LED rapidly to indicate startup
    blinkLed(10);  // You can adjust the blink count for a more noticeable startup blink

    // Turn on the LED initially
    controlLed(true);

    while (1) {
        int ret = readDHT();

        if (ret == DHT_OK) {
            float humidity = getHumidity();
            printf("Humidity: %.2f%%\n", humidity);

            if (humidity > HUMIDITY_THRESHOLD + HYSTERESIS && !pumpActive) {
                // Turn on the water pump
                controlWaterPump(true);

                // Set the flag to indicate the pump is active
                pumpActive = true;

                // Print a message indicating water pumping
                printf("Pumping water\n");

                // Turn off the LED when the pump is active
                double blinkcount = PUMP_DURATION_SECONDS * 1000 / BLINK_INTERVAL_MS;
                blinkLed(blinkcount);

                // Wait for the specified duration
                vTaskDelay(blinkcount);

                // Turn off the water pump after the specified duration
                controlWaterPump(false);

                // Print a message indicating pump is off
                printf("Pump off\n");

                // Turn on the LED after the pump is turned off
                controlLed(true);
            } else if (humidity <= HUMIDITY_THRESHOLD - HYSTERESIS) {
                // Reset the flag when humidity falls below the lower threshold
                pumpActive = false;
            }
        } else {
            errorHandler(ret);
        }

        // Wait for the next iteration
        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    setDHTgpio(DHT_GPIO_PIN);

    // Setup the LED pin as an output
    gpio_config_t led_io_conf = {
        .pin_bit_mask = (1ULL << LED_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&led_io_conf);

    // Setup the relay and LED pins as outputs
    gpio_config_t relay_io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&relay_io_conf);

    xTaskCreate(&dht_task, "dht_task", 2048, NULL, 5, NULL);
}
