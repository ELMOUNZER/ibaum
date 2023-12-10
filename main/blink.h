#ifndef BLINK_H
#define BLINK_H

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"

#define LED_GPIO_PIN 2
#define BLINK_INTERVAL_MS 100

void control_Led(bool turnOn);
void blinkLed(int blinkCount);

#endif
