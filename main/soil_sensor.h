#ifndef SOIL_SENSOR_H
#define SOIL_SENSOR_H

#include <stdbool.h>


// Function Declarations
void controlWaterPump(bool turnOn);
int map(int x, int in_min, int in_max, int out_min, int out_max);
int getSoilMoisture(void);
void soilTask(void *pvParameter);

#endif  // SOIL_SENSOR_H
