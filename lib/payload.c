#include "payload.h"
#include "aht20.h"
#include "bmp280.h"

// Inicializa os buffers com valores zerados
void payload_buffers_init(AHT20_buffer_t *AHT20_buffer, BMP280_buffer_t *BMP280_buffer){
    for(int i=0; i<BUFFER_SIZE; i++){
        AHT20_buffer->humidity[i] = 0.0f;
        AHT20_buffer->temperature[i] = 0.0f;
        BMP280_buffer->pressure[i] = 0.0f;
        BMP280_buffer->temperature[i] = 0.0f;
    }
}

// Atualiza os buffers com o valor mais recente
void payload_buffers_update(AHT20_data_t AHT20_data, BMP280_data_t BMP280_data, AHT20_buffer_t *AHT20_buffer, BMP280_buffer_t *BMP280_buffer){
    for(int i=0; i<BUFFER_SIZE-1; i++){
        AHT20_buffer->humidity[i] = AHT20_buffer->humidity[i+1];
        AHT20_buffer->temperature[i] = AHT20_buffer->temperature[i+1];
        BMP280_buffer->pressure[i] = BMP280_buffer->pressure[i+1];
        BMP280_buffer->temperature[i] = BMP280_buffer->temperature[i+1];
    }

    AHT20_buffer->humidity[BUFFER_SIZE-1] = AHT20_data.humidity;
    AHT20_buffer->temperature[BUFFER_SIZE-1] = AHT20_data.temperature;
    BMP280_buffer->pressure[BUFFER_SIZE-1] = BMP280_data.pressure;
    BMP280_buffer->temperature[BUFFER_SIZE-1] = BMP280_data.temperature;
}