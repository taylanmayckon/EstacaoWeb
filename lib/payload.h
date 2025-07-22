#ifndef PAYLOAD_H
#define PAYLOAD_H

#include "aht20.h"
#include "bmp280.h"

#define BUFFER_SIZE 50

typedef struct {
    float temperature[BUFFER_SIZE];
    float humidity[BUFFER_SIZE];
} AHT20_buffer_t;

typedef struct {
    float temperature[BUFFER_SIZE];
    float pressure[BUFFER_SIZE];
} BMP280_buffer_t;

void payload_buffers_init(AHT20_buffer_t *AHT20_buffer, BMP280_buffer_t *BMP280_buffer);
void payload_buffers_update(AHT20_data_t AHT20_data, BMP280_data_t BMP280_data, AHT20_buffer_t *AHT20_buffer, BMP280_buffer_t *BMP280_buffer);


#endif