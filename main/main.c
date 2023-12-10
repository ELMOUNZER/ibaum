#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "freertos/semphr.h"  // Include the semaphore header
#include "soil_sensor.h"
#include "blink.h"

// Pin Definitions
#define RELAY_GPIO_PIN 27
#define LED_GPIO_PIN 2

SemaphoreHandle_t xButtonSemaphore;  // Declare the semaphore

void app_main() {

    vTaskDelay(pdMS_TO_TICKS(1000));

    gpio_config_t led_io_conf = {
            .pin_bit_mask = (1ULL << LED_GPIO_PIN),
            .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config_t relay_io_conf = {
            .pin_bit_mask = (1ULL << RELAY_GPIO_PIN),
            .mode = GPIO_MODE_OUTPUT,
    };
     gpio_config_t io_conf = {
        .pin_bit_mask = (1ULL << RELAY_GPIO_PIN),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io_conf);
    gpio_config(&led_io_conf);
    gpio_config(&relay_io_conf);

    // Initialize NVS flash
    esp_err_t nvs_init_result = nvs_flash_init();
    if (nvs_init_result != ESP_OK) {
        printf("Error initializing NVS Flash\n");
        // Handle error as needed
    }

    // Create a binary semaphore
    xButtonSemaphore = xSemaphoreCreateBinary();
    if (xButtonSemaphore == NULL) {
        printf("Error creating semaphore\n");
        // Handle error as needed
    }

    // Call the WiFi connection function
    // wifi_connection();  // Commented out as it's not defined


    // Create tasks
    // xTaskCreate(&web_server_task, "web_server_task", 4096, NULL, 5, NULL);  // Commented out as it's not defined
    xTaskCreate(&soilTask, "soil_task", 2048, NULL, 5, NULL);

}
