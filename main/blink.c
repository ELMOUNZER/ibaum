#include "blink.h"

void blink_task(void *pvParameter) {
    while (1) {
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        printf("on\n");
        gpio_set_level(BLINK_GPIO, 0);
        printf("off\n");
        vTaskDelay(1000 / portTICK_PERIOD_MS);

    }
}
