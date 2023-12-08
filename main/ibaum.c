#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "driver/adc.h"

#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_7  // GPIO 36 corresponds to ADC1_CH0
#define RELAY_GPIO_PIN 25       // Replace with the actual GPIO pin connected to the relay
#define LED_GPIO_PIN 2         // Replace with the actual GPIO pin connected to the LED
#define MOISTURE_THRESHOLD 55   // Adjust this value as needed
#define PUMP_DURATION_SECONDS 5
#define BLINK_INTERVAL_MS 100  // Blink interval for LED
#define WET_LEVEL 2300
#define DRY_LEVEL 300

void controlWaterPump(bool turnOn) {
    gpio_set_level(RELAY_GPIO_PIN, turnOn ? 1 : 0);
}

void control_Led(bool turnOn) {
    gpio_set_level(LED_GPIO_PIN, turnOn ? 1 : 0);
}

void blinkLed(int blinkCount) {
    for (int i = 0; i < blinkCount; i++) {
        control_Led(i % 2 == 0);  // Toggle the LED state
        vTaskDelay(BLINK_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

// Map the input value from one range to another range (similar to Arduino's map function) 
// 
int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int getSoilMoisture() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(SOIL_SENSOR_ADC_CHANNEL, ADC_ATTEN_DB_11);

    // Introduce a delay to allow the sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));

    int adc_value = adc1_get_raw(SOIL_SENSOR_ADC_CHANNEL);
    printf("Raw ADC Value: %d\n", adc_value);

    // Assuming the sensor's output range is linear between wet and dry values
    int moisture_percentage = map(adc_value, WET_LEVEL, DRY_LEVEL, 0, 100);
    //printf("Soil Moisture: %d%%\n", moisture_percentage);

    return moisture_percentage;
}

void soilTask(void *pvParameter) {
    bool pumpActive = false;

    // Blink the LED rapidly to indicate startup
    blinkLed(10);  // You can adjust the blink count for a more noticeable startup blink

    // Turn on the LED initially
    control_Led(true);

    while (1) {
        // Placeholder for reading soil moisture
        int moisture = getSoilMoisture();
        printf("Soil Moisture: %d%%\n", moisture);

        if (moisture < MOISTURE_THRESHOLD && !pumpActive) {
            // Turn on the water pump
            controlWaterPump(true);

            // Set the flag to indicate the pump is active
            pumpActive = true;

            // Print a message indicating water pumping
            printf("The soil is dry. Pumping water...\n");

            // Turn off the LED when the pump is active
            double blinkCount = PUMP_DURATION_SECONDS * 1000 / BLINK_INTERVAL_MS;
            blinkLed(blinkCount);

            // Wait for the specified duration
            vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION_SECONDS * 1000));

            // Turn off the water pump after the specified duration
            controlWaterPump(false);

            // Print a message indicating pump is off
            printf("Water pump is off.\n");

            // Turn on the LED after the pump is turned off
            control_Led(true);
        } else if (moisture >= MOISTURE_THRESHOLD) {
            // Reset the flag when moisture is above the threshold
            pumpActive = false;
        }

        // Wait for the next iteration
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main() {
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

    // Create the soil task
    xTaskCreate(&soilTask, "soil_task", 2048, NULL, 5, NULL);
}