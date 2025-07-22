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

typedef struct{
    float min;
    float max;
    float offset;
} ConfigParams_t;

typedef struct{
    ConfigParams_t AHT20_temperature;
    ConfigParams_t AHT20_humidity;
    ConfigParams_t BMP280_pressure;
    ConfigParams_t BMP280_temperature;
} AlertParams_t;

#endif