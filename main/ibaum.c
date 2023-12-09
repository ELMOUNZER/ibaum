#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "driver/gpio.h"
#include "driver/adc.h"
#include "esp_log.h"

#include "ssd1306.h"
#include "font8x8_basic.h"

#define tag "SSD1306"

#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_7
#define RELAY_GPIO_PIN 25
#define LED_GPIO_PIN 2
#define MOISTURE_THRESHOLD 55
#define PUMP_DURATION_SECONDS 5
#define BLINK_INTERVAL_MS 100
#define WET_LEVEL 2300
#define DRY_LEVEL 300

// Declare semaphore for thread-safe communication
SemaphoreHandle_t xSemaphore;

// Global variable to store LED status
bool ledStatus = false;

void controlWaterPump(bool turnOn) {
    gpio_set_level(RELAY_GPIO_PIN, turnOn ? 1 : 0);
}

void control_Led(bool turnOn) {
    gpio_set_level(LED_GPIO_PIN, turnOn ? 1 : 0);
    // Update global LED status
    ledStatus = turnOn;
}

void blinkLed(int blinkCount) {
    for (int i = 0; i < blinkCount; i++) {
        control_Led(i % 2 == 0);
        vTaskDelay(BLINK_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int getSoilMoisture() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(SOIL_SENSOR_ADC_CHANNEL, ADC_ATTEN_DB_11);

    vTaskDelay(pdMS_TO_TICKS(1000));

    int adc_value = adc1_get_raw(SOIL_SENSOR_ADC_CHANNEL);
    //printf("Raw ADC Value: %d\n", adc_value);

    int moisture_percentage = map(adc_value, WET_LEVEL, DRY_LEVEL, 0, 100);
    return moisture_percentage;
}

void displayTask(SSD1306_t *dev) {
    int center = 3;

    while (1) {
        // Retrieve soil moisture value
        int moisture = getSoilMoisture();

        // Display soil moisture value with horizontal scrolling
        char moisture_text[16];
        sprintf(moisture_text, "Moisture: %d%%", moisture);

        // Clear the screen before displaying new content
        ssd1306_clear_screen(dev, false);

        // Display new text with horizontal scrolling
        for (int i = 0; i < strlen(moisture_text) * 8; i++) {
            int start = center - i / 8;
            ssd1306_display_text(dev, start, moisture_text, 5, false);
            vTaskDelay(pdMS_TO_TICKS(100));  // Adjust the delay as needed
            ssd1306_clear_screen(dev, false);
        }

        // Wait for a while to show the new content without scrolling
        vTaskDelay(pdMS_TO_TICKS(2000));  // Delay for 2 seconds
    }
}

void soilTask(void *pvParameter) {
    SSD1306_t *dev = (SSD1306_t *)pvParameter;
    bool pumpActive = false;

    // Turn on the LED initially
    control_Led(true);

    while (1) {
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

        // Thread-safe communication: Notify other tasks if needed
        xSemaphoreGive(xSemaphore);

        // Update OLED display based on LED status
        displayTask(dev);

        // Wait for the next iteration
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void communicationTask(void *pvParameter) {
    SSD1306_t *dev = (SSD1306_t *)pvParameter;

    while (1) {
        // Wait for notification from soilTask
        if (xSemaphoreTake(xSemaphore, portMAX_DELAY) == pdTRUE) {
            // Perform actions that need to be synchronized with soilTask
            printf("Communication task received notification.\n");

            // Update OLED display based on LED status
            displayTask(dev);
        }

        // Additional functionality can be added here if needed

        // Adjust priority based on application needs
        vTaskPrioritySet(NULL, configMAX_PRIORITIES / 2);

        // Wait for the next iteration
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void app_main(void) {
    SSD1306_t dev;
    int center;

    gpio_config_t relay_config = {
        .pin_bit_mask = (1ULL << RELAY_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0
    };

    gpio_config(&relay_config);

    gpio_config_t led_config = {
        .pin_bit_mask = (1ULL << LED_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
        .intr_type = GPIO_INTR_DISABLE,
        .pull_down_en = 0,
        .pull_up_en = 0
    };

    gpio_config(&led_config);

#if CONFIG_I2C_INTERFACE
    i2c_master_init(&dev, CONFIG_SDA_GPIO, CONFIG_SCL_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_I2C_INTERFACE

#if CONFIG_SPI_INTERFACE
    spi_master_init(&dev, CONFIG_MOSI_GPIO, CONFIG_SCLK_GPIO, CONFIG_CS_GPIO, CONFIG_DC_GPIO, CONFIG_RESET_GPIO);
#endif // CONFIG_SPI_INTERFACE

#if CONFIG_SSD1306_128x64
    ssd1306_init(&dev, 128, 64);
    center = 3;
#endif // CONFIG_SSD1306_128x64

#if CONFIG_SSD1306_128x32
    ssd1306_init(&dev, 128, 32);
    center = 1;
#endif // CONFIG_SSD1306_128x32

    ssd1306_clear_screen(&dev, false);
    ssd1306_contrast(&dev, 0xff);
    char moisture_text[16];
    sprintf(moisture_text, "M:%d%%", getSoilMoisture());
    ssd1306_display_text(&dev, center, moisture_text, 5, false);

    vTaskDelay(3000 / portTICK_PERIOD_MS);

    // Initialize semaphore for thread-safe communication
    xSemaphore = xSemaphoreCreateBinary();

    if (xSemaphore != NULL) {
        // Create the soil task on Core 1 (PRO CPU)
        xTaskCreatePinnedToCore(&soilTask, "soil_task", 4096, (void *)&dev, configMAX_PRIORITIES - 1, NULL, 1);

        // Create the communication task on Core 0 (APP CPU)
        xTaskCreatePinnedToCore(&communicationTask, "communication_task", 4096, NULL, configMAX_PRIORITIES / 2, NULL, 0);
    }
}
