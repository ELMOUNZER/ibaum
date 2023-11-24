#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "driver/gpio.h"
#include "freertos/semphr.h"
#include "DHT.h"

#define DHT_GPIO_PIN 4

void dht_task(void *pvParameter) {
    while (1) {
        float temperature = 0, humidity = 0;
        int ret = readDHT();
        
        if (ret == DHT_OK) {
            temperature = getTemperature();
            humidity = getHumidity();
            printf("Temperature: %.2f°C, Humidity: %.2f%%\n", temperature, humidity);
        } else {
            errorHandler(ret);
        }

        vTaskDelay(2000 / portTICK_PERIOD_MS);
    }
}

void app_main() {
    setDHTgpio(DHT_GPIO_PIN);
    xTaskCreate(&dht_task, "dht_task", 2048, NULL, 5, NULL);
}
