#ifndef BLINK_H
#define BLINK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define BLINK_GPIO 2

void blink_task(void *pvParameter);

#endif
