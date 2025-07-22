#ifndef ALERTS_H
#define ALERTS_H

#include <stdbool.h>

// Struct para as flags de alerta
typedef struct {
    bool aht20_temperature;
    bool aht20_humidity;
    bool bmp280_pressure;
    bool bmp280_temperature;
} Sensor_alerts_t;

#endif