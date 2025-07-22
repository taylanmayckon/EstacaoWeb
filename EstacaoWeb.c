#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include "hardware/pwm.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ssd1306.h"
#include "font.h"
#include "led_matrix.h"
#include "aht20.h"
#include "bmp280.h"
#include "payload.h"
#include "alerts.h"

// Nome e senha da rede wi-fi
#define WIFI_SSID "Infornet_Taylan"
#define WIFI_PASS "suta3021"

// GPIO utilizada
#define LED_RED 13
#define LED_GREEN 11
#define LED_BLUE 12 
#define BUTTON_A 5
#define BUTTON_B 6
#define JOYSTICK_BUTTON 22
#define LED_MATRIX_PIN 7
#define BUZZER_A 21
#define BUZZER_B 10

// Configurações da I2C do display
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C
bool cor = true;
ssd1306_t ssd;

// Configurações da I2C dos sensores
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define SEA_LEVEL_PRESSURE 101325.0 // Pressão ao nível do mar em Pa

// Para a matriz de leds
#define IS_RGBW false

// Estrutura para armazenar os dados dos sensores
// Dados individuais
AHT20_data_t AHT20_data;
BMP280_data_t BMP280_data;
int32_t raw_temp_bmp;
int32_t raw_pressure;

// Para o payload e gráficos
AHT20_buffer_t AHT20_buffer;
BMP280_buffer_t BMP280_buffer;
Payload_sizes_t payload_sizes;
Sensor_alerts_t sensor_alerts = {false, false, false, false};
char json_payload[1024]; 
ConfigParams_t config_params;

// Configurações para o PWM
uint wrap = 2000;
uint clkdiv = 25;

// Variáveis da PIO declaradas no escopo global
PIO pio;
uint sm;

const char HTML_BODY[] =
    "<!DOCTYPE html><html lang='pt-br'><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>DogAtmos - EmbarcaTech</title>"
    "<style>"
    "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;margin:0;padding:20px;background-color:#f0f2f5;color:#333}"
    "h1{text-align:center;color:#1e3a5f;margin-bottom:30px}"
    "h2{margin-top:0;color:#333;border-bottom:2px solid #e0e0e0;padding-bottom:5px;margin-bottom:15px}"
    ".main-content{max-width:1200px;margin:0 auto;display:flex;flex-direction:column;gap:20px}"
    ".sensor-row{display:flex;flex-wrap:wrap;align-items:center;justify-content:center;gap:20px;padding:20px;background-color:#ffffff;border-radius:8px;box-shadow:0 4px 6px rgba(0,0,0,0.05)}"
    ".chart-container{flex:2;min-width:300px;max-width:700px}"
    ".chart-container canvas{width:100%!important;height:300px!important}"
    ".sensor-info{flex:1;min-width:250px;display:flex;flex-direction:column;gap:15px}"
    ".info-item,.input-group,.alert-status-container{background-color:#f8f9fa;padding:10px 15px;border-radius:6px;border:1px solid #e9ecef}"
    ".info-item p{margin:0;font-size:1.2em;font-weight:500}"
    ".info-item span{font-weight:400;color:#007bff}"
    ".input-group label,.alert-status-container label{display:block;margin-bottom:5px;font-weight:500}"
    ".input-group input{width:100%;padding:8px;border:1px solid #ced4da;border-radius:4px;box-sizing:border-box}"
    ".alert-indicator{padding:10px;border-radius:8px;text-align:center;font-weight:bold;color:white;background-color:#4CAF50;transition:background-color .3s ease,opacity .3s ease}"
    ".alert-indicator.triggered{background-color:#f44336;animation:blink 1.2s infinite ease-in-out}"
    "@keyframes blink{50%{opacity:.7}}"
    "</style>"
    "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script>"
    "</head><body>"
    "<h1>DogAtmos - EmbarcaTech</h1>"
    "<div id='main-content' class='main-content'></div>"
    "<div style='text-align:center;max-width:1200px;margin:40px auto 20px;'>"
    "<hr style='border:0;height:1px;background-color:#ddd;'>"
    "<h3>Desenvolvido por: Taylan Mayckon</h3>"
    "<p>Atividade da Fase 2 do EmbarcaTech, envolvendo uso dos sensores BMP280 e AHT20 para criar uma estação meteorológica com interface WEB.</p>"
    "</div>"
    "<script>"
    "const sensorConfig=[{id:'AHT20_temperature',sensorName:'AHT20',label:'Temperatura',unit:'°C',color:[255,99,132],defaultMin:0,defaultMax:40,defaultOffset:0},{id:'AHT20_humidity',sensorName:'AHT20',label:'Umidade',unit:'%',color:[54,162,235],defaultMin:30,defaultMax:80,defaultOffset:0},{id:'BMP280_pressure',sensorName:'BMP280',label:'Pressão',unit:'kPa',color:[75,192,192],defaultMin:90,defaultMax:110,defaultOffset:0},{id:'BMP280_temperature',sensorName:'BMP280',label:'Temperatura',unit:'°C',color:[255,159,64],defaultMin:0,defaultMax:40,defaultOffset:0}];"
    "const charts={};const fetchInterval=2000;"
    "function initializeDashboard(){const t=document.getElementById('main-content');sensorConfig.forEach(e=>{const n=\"<div class='sensor-row'><div class='chart-container'><h2>\"+e.sensorName+\" - \"+e.label+\"</h2><canvas id='\"+e.id+\"_chart'></canvas></div><div class='sensor-info'><div class='info-item'><p>Valor Atual: <span id='current_\"+e.id+\"'>--</span> \"+e.unit+\"</p></div><div class='input-group'><label for='min_\"+e.id+\"'>Limite Mínimo:</label><input type='number' id='min_\"+e.id+\"' value='\"+e.defaultMin+\"'></div><div class='input-group'><label for='max_\"+e.id+\"'>Limite Máximo:</label><input type='number' id='max_\"+e.id+\"' value='\"+e.defaultMax+\"'></div><div class='input-group'><label for='offset_\"+e.id+\"'>Offset de Calibração:</label><input type='number' id='offset_\"+e.id+\"' value='\"+e.defaultOffset+\"'></div><div class='alert-status-container'><label>Status do Alerta:</label><div id='alert_\"+e.id+\"' class='alert-indicator'>NORMAL</div></div></div></div>\";t.insertAdjacentHTML('beforeend',n);['min','max','offset'].forEach(t=>{document.getElementById(t+'_'+e.id).addEventListener('change',()=>handleConfigChange(e.id))});const[o,a,s]=e.color;charts[e.id]=new Chart(document.getElementById(e.id+'_chart').getContext('2d'),{type:'line',data:{labels:[],datasets:[{label:e.label,data:[],borderColor:'rgba('+o+','+a+','+s+',1)',backgroundColor:'rgba('+o+','+a+','+s+',0.2)',borderWidth:2,fill:!0,tension:.4}]},options:{responsive:!0,maintainAspectRatio:!1,scales:{x:{title:{display:!0,text:'Tempo'}},y:{title:{display:!0,text:e.label+' ['+e.unit+']'},beginAtZero:!1}},animation:{duration:500}}})})}"
    "function handleConfigChange(e){const t=sensorConfig.find(t=>t.id===e),n=document.getElementById(`min_${e}`),o=document.getElementById(`max_${e}`),a=document.getElementById(`offset_${e}`);''===n.value&&(n.value=t.defaultMin),''===o.value&&(o.value=t.defaultMax),''===a.value&&(a.value=t.defaultOffset);const s={min:parseFloat(n.value),max:parseFloat(o.value),offset:parseFloat(a.value)};fetch(`/config/${e}`,{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(s)})}"
    "function loadConfig(){fetch('/config').then(e=>e.json()).then(t=>{sensorConfig.forEach(e=>{const n=t[e.id]||{};document.getElementById(`min_${e.id}`).value=n.min??e.defaultMin,document.getElementById(`max_${e.id}`).value=n.max??e.defaultMax,document.getElementById(`offset_${e.id}`).value=n.offset??e.defaultOffset})})}"
    "function updateData(){fetch('/data').then(e=>e.json()).then(t=>{const n=new Date;sensorConfig.forEach(e=>{const o=t[e.id];if(!o)return;const a=t[`alert_${e.id}`];updateCharts(charts[e.id],n,o),document.getElementById(`current_${e.id}`).textContent=parseFloat(o.at(-1)).toFixed(2),checkAlerts(e.id,a)})})}"
    "function checkAlerts(e,t){const n=document.getElementById(`alert_${e}`);let o='on'===t;n.classList.toggle('triggered',o),n.textContent=o?'ALERTA!':'NORMAL'}"
    "function updateCharts(e,t,n){const o=[],a=n.length;for(let s=0;s<a;s++){const c=new Date(t.getTime()-(a-1-s)*fetchInterval);o.push(c.toLocaleTimeString('pt-BR'))}e.data.labels=o,e.data.datasets[0].data=n,e.update('none')}"
    "initializeDashboard();loadConfig();updateData();setInterval(updateData,2000);"
    "</script>"
    "</body></html>";




struct http_state{
    char response[10000];
    size_t len;
    size_t sent;
};


static err_t http_sent(void *arg, struct tcp_pcb *tpcb, u16_t len){
    struct http_state *hs = (struct http_state *)arg;
    hs->sent += len;
    if (hs->sent >= hs->len)
    {
        tcp_close(tpcb);
        free(hs);
    }
    return ERR_OK;
}

// Estrutura para guardar o estado do nosso servidor entre chamadas
typedef enum {
    STATE_NORMAL,
    STATE_WAITING_FOR_BODY
} server_mode_t;

typedef struct {
    server_mode_t mode;
    char sensor_id[32];
} server_state_t;

server_state_t server_state = { .mode = STATE_NORMAL };

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err){
    if (!p){
        tcp_close(tpcb);
        server_state.mode = STATE_NORMAL; 
        return ERR_OK;
    }

    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs){
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    hs->sent = 0;

    char request_buffer[1500];
    pbuf_copy_partial(p, request_buffer, p->tot_len, 0);
    request_buffer[p->tot_len] = '\0';


    // INÍCIO DA MÁQUINA DE ESTADOS 
    if (server_state.mode == STATE_NORMAL) {
        // MODO NORMAL: O servidor não está esperando por nada 

        if (strstr(request_buffer, "GET /data")){
            payload_sizes.json_size = sizeof(json_payload);
            payload_sizes.aht20_humi_size = sizeof(AHT20_buffer.humidity);
            payload_sizes.aht20_temp_size = sizeof(AHT20_buffer.temperature);
            payload_sizes.bmp280_press_size = sizeof(BMP280_buffer.pressure);
            payload_sizes.bmp280_temp_size = sizeof(BMP280_buffer.temperature);

            int json_len = payload_generate_json(json_payload, sensor_alerts, &AHT20_buffer, &BMP280_buffer, payload_sizes);
            hs->len = snprintf(hs->response, sizeof(hs->response),
                               "HTTP/1.1 200 OK\r\nContent-Type: application/json\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
                               json_len, json_payload);
        }
        else if (strncmp(request_buffer, "POST /config/", 13) == 0) {
            // Primeira parte de um POST: Apenas os cabeçalhos.
            sscanf(request_buffer, "POST /config/%s", server_state.sensor_id);
            printf("[DEBUG] POST Headers recebidos para: %s. Aguardando o corpo...\n", server_state.sensor_id);
            
            // Mudando o estado para esperar a próxima chamada
            server_state.mode = STATE_WAITING_FOR_BODY;

            free(hs); // Liberando o 'hs' pois não vai usar agora
            pbuf_free(p);
            return ERR_OK; 
        }
        else { 
            hs->len = snprintf(hs->response, sizeof(hs->response),
                               "HTTP/1.1 200 OK\r\nContent-Type: text/html\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
                               (int)strlen(HTML_BODY), HTML_BODY);
        }

    } else if (server_state.mode == STATE_WAITING_FOR_BODY) {
        // MODO DE ESPERA: Esse buffer é o corpo JSON do POST anterior
        printf("[DEBUG] Corpo do POST recebido para %s: %s\n", server_state.sensor_id, request_buffer);
        
        float min, max, offset;
        int parsed_count = sscanf(request_buffer, "{\"min\":%f,\"max\":%f,\"offset\":%f}", &min, &max, &offset);

        if (parsed_count == 3) {
            // Atualiza os parâmetros do sensor correto 
            if (strcmp(server_state.sensor_id, "AHT20_temperature") == 0) {
                config_params.AHT20_temperature.min = min;
                config_params.AHT20_temperature.max = max;
                config_params.AHT20_temperature.offset = offset;
            } else if (strcmp(server_state.sensor_id, "AHT20_humidity") == 0) {
                config_params.AHT20_humidity.min = min;
                config_params.AHT20_humidity.max = max;
                config_params.AHT20_humidity.offset = offset;
            } else if (strcmp(server_state.sensor_id, "BMP280_pressure") == 0) {
                config_params.BMP280_pressure.min = min;
                config_params.BMP280_pressure.max = max;
                config_params.BMP280_pressure.offset = offset;
            } else if (strcmp(server_state.sensor_id, "BMP280_temperature") == 0) {
                config_params.BMP280_temperature.min = min;
                config_params.BMP280_temperature.max = max;
                config_params.BMP280_temperature.offset = offset;
            }
            printf("[DEBUG] Parametros para %s atualizados!\n", server_state.sensor_id);

            // Enviando a resposta de sucesso
            const char *ok_resp = "OK";
            hs->len = snprintf(hs->response, sizeof(hs->response),
                                   "HTTP/1.1 200 OK\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
                                   strlen(ok_resp), ok_resp);
        } else {
             const char *err_resp = "Bad Request";
             hs->len = snprintf(hs->response, sizeof(hs->response),
                                   "HTTP/1.1 400 Bad Request\r\nContent-Length: %d\r\nConnection: close\r\n\r\n%s",
                                   strlen(err_resp), err_resp);
        }
        
        // Prepara para a próxima requisição
        server_state.mode = STATE_NORMAL;
    }
    
    tcp_arg(tpcb, hs);
    tcp_sent(tpcb, http_sent);
    tcp_write(tpcb, hs->response, hs->len, TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb);

    pbuf_free(p);
    return ERR_OK;
}


static err_t connection_callback(void *arg, struct tcp_pcb *newpcb, err_t err){
    tcp_recv(newpcb, http_recv);
    return ERR_OK;
}


static void start_http_server(void){
    struct tcp_pcb *pcb = tcp_new();
    if (!pcb){
        printf("Erro ao criar PCB TCP\n");
        return;
    }
    if (tcp_bind(pcb, IP_ADDR_ANY, 80) != ERR_OK){
        printf("Erro ao ligar o servidor na porta 80\n");
        return;
    }
    pcb = tcp_listen(pcb);
    tcp_accept(pcb, connection_callback);
    printf("Servidor HTTP rodando na porta 80...\n");
}

// -> Funções Auxiliares =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Função para configurar o PWM e iniciar com 0% de DC
void set_pwm(uint gpio, uint wrap){
    gpio_set_function(gpio, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(gpio);
    pwm_set_clkdiv(slice_num, clkdiv);
    pwm_set_wrap(slice_num, wrap);
    pwm_set_enabled(slice_num, true); 
    pwm_set_gpio_level(gpio, 0);
}

// Função para imprimir uma exclamação nos alertas do display
void make_alert_display(bool alert_flag, int x, int y){
    if(alert_flag){
        ssd1306_rect(&ssd, y, x, 26, 8, cor, !cor);
        ssd1306_draw_string(&ssd, "!", x+8, y, !cor);
    }
    else{
        ssd1306_draw_string(&ssd, "NORMAL", x, y, !cor);
    }
}

// -> ISR dos Botões =-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=
// Tratamento de interrupções 
int display_page = 1;
int num_pages = 5;
uint32_t last_isr_time = 0;
void gpio_irq_handler(uint gpio, uint32_t events){
    uint32_t current_isr_time = to_us_since_boot(get_absolute_time());
    if(current_isr_time-last_isr_time > 200000){ // Debounce
        last_isr_time = current_isr_time;
        
        if(gpio==BUTTON_A) {
            display_page--;
        }
        else{
            display_page++;
        }
        

        if(display_page>num_pages) display_page=num_pages;
        if(display_page<1) display_page=1;
    }
}


uint32_t last_sensor_read = 0;
bool buzzer_active = false;

int main(){
    stdio_init_all();
    sleep_ms(2000);

    // Iniciando o display
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);

    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Iniciando Wi-Fi", 0, 0, false);
    ssd1306_draw_string(&ssd, "Aguarde...", 0, 30, false);    
    ssd1306_send_data(&ssd);

    // Iniciando o I2C dos sensores
    i2c_init(I2C_PORT, 400 * 1000);
    gpio_set_function(I2C_SDA, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA);
    gpio_pull_up(I2C_SCL);

    // Inicializando o BMP280
    bmp280_init(I2C_PORT);
    struct bmp280_calib_param params;
    bmp280_get_calib_params(I2C_PORT, &params);

    // Inicializando o AHT20
    aht20_reset(I2C_PORT);
    aht20_init(I2C_PORT);

    // Iniciando a matriz de leds
    pio = pio0;
    sm = 0;
    uint offset = pio_add_program(pio, &ws2812_program);
    ws2812_program_init(pio, sm, offset, LED_MATRIX_PIN, 800000, IS_RGBW);

    // Iniciando Wi-Fi
    if (cyw43_arch_init()){
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "WiFi -> FALHA", 0, 0, false);
        ssd1306_send_data(&ssd);
        return 1;
    }

    cyw43_arch_enable_sta_mode();
    if (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASS, CYW43_AUTH_WPA2_AES_PSK, 10000)){
        ssd1306_fill(&ssd, false);
        ssd1306_draw_string(&ssd, "WiFi -> ERRO", 0, 0, false);
        ssd1306_send_data(&ssd);
        return 1;
    }

    uint8_t *ip = (uint8_t *)&(cyw43_state.netif[0].ip_addr.addr);
    char ip_str[24];
    snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);

    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "WiFi -> OK", 0, 0, false);
    ssd1306_draw_string(&ssd, ip_str, 0, 10, false);
    ssd1306_send_data(&ssd);

    // Iniciando os buffers dos payloads
    payload_buffers_init(&AHT20_buffer, &BMP280_buffer);

    // Criando a configuração inicial dos alertas
    config_params.AHT20_humidity.max = 80.0;
    config_params.AHT20_humidity.min = 30.0;
    config_params.AHT20_humidity.offset = 0.0;

    config_params.AHT20_temperature.max = 40.0;
    config_params.AHT20_temperature.min = 0.0;
    config_params.AHT20_temperature.offset = 0.0;

    config_params.BMP280_pressure.max = 110.0;
    config_params.BMP280_pressure.min = 90.0;
    config_params.BMP280_pressure.offset = 0.0;

    config_params.BMP280_temperature.max = 40.0;
    config_params.BMP280_temperature.min = 0.0;
    config_params.BMP280_temperature.offset = 0.0;

    // Iniciando o LED RGB
    set_pwm(LED_RED, wrap);
    set_pwm(LED_GREEN, wrap);
    set_pwm(LED_BLUE, wrap);
    // Iniciando os buzzers
    set_pwm(BUZZER_A, wrap);
    set_pwm(BUZZER_B, wrap);
    
    // Iniciando os botões
    gpio_init(BUTTON_A);
    gpio_set_dir(BUTTON_A, GPIO_IN);
    gpio_pull_up(BUTTON_A);
    gpio_set_irq_enabled_with_callback(BUTTON_A, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);

    gpio_init(BUTTON_B);
    gpio_set_dir(BUTTON_B, GPIO_IN);
    gpio_pull_up(BUTTON_B);
    gpio_set_irq_enabled_with_callback(BUTTON_B, GPIO_IRQ_EDGE_FALL, true, &gpio_irq_handler);


    start_http_server();

    while (true){
        cyw43_arch_poll();

        uint32_t current_sensor_read = to_us_since_boot(get_absolute_time());
        // Temporização para leitura dos sensores
        if(current_sensor_read-last_sensor_read > 2000000){
            last_sensor_read = current_sensor_read;
            // Leitura do BMP280
            bmp280_read_raw(I2C_PORT, &raw_temp_bmp, &raw_pressure);
            int32_t temperature = bmp280_convert_temp(raw_temp_bmp, &params);
            int32_t pressure = bmp280_convert_pressure(raw_pressure, raw_temp_bmp, &params);

            printf("[DEBUG] Leitura de sensores\n");
            printf("BMP280.pressure = %.3f kPa\n", BMP280_data.pressure);
            printf("BMP280.temperature = %.2f C\n", BMP280_data.temperature);

            // Leitura do AHT20
            if (aht20_read(I2C_PORT, &AHT20_data)){
                printf("AHT20_data.temperature = %.2f C\n", AHT20_data.temperature);
                printf("AHT20_data.humidity = %.2f %%\n", AHT20_data.humidity);
            }
            else{
                printf("Erro na leitura do AHT10!\n");
            }
            printf("\n\n");

            // Aplicando os offsets de calibração do HTML
            BMP280_data.pressure = config_params.BMP280_pressure.offset + pressure/1000.0f;
            BMP280_data.temperature = config_params.BMP280_temperature.offset + temperature/100.0f;
            AHT20_data.humidity = config_params.AHT20_humidity.offset + AHT20_data.humidity;
            AHT20_data.temperature= config_params.AHT20_temperature.offset + AHT20_data.temperature;

            // Verifica se deve acionar alerta
            alerts_handle(&sensor_alerts, config_params, BMP280_data, AHT20_data);

            // Atualizando os dados dos buffers
            payload_buffers_update(AHT20_data, BMP280_data, &AHT20_buffer, &BMP280_buffer);
        }

        // Strings com os valores
        char str_tmp_aht[5];
        char str_humi_aht[5];
        char str_press_bmp[5];
        char str_temp_bmp[5];

        char str_offset_tmp_aht[5];
        char str_offset_humi_aht[5];
        char str_offset_press_bmp[5];
        char str_offset_temp_bmp[5];

        // Atualizando as strings
        sprintf(str_tmp_aht, "%.1f C", AHT20_data.temperature);
        sprintf(str_humi_aht, "%.1f %%", AHT20_data.humidity);
        sprintf(str_press_bmp, "%.1f kPa", BMP280_data.pressure);
        sprintf(str_temp_bmp, "%.1f C", BMP280_data.temperature);

        sprintf(str_offset_tmp_aht, "%.1f C", config_params.AHT20_temperature.offset);
        sprintf(str_offset_humi_aht, "%.1f %%", config_params.AHT20_humidity.offset);
        sprintf(str_offset_press_bmp, "%.1f kPa", config_params.BMP280_pressure.offset);
        sprintf(str_offset_temp_bmp, "%.1f C", config_params.BMP280_temperature.offset);

        // Checa ativação dos alertas
        if(sensor_alerts.aht20_humidity || sensor_alerts.aht20_temperature || sensor_alerts.bmp280_pressure || sensor_alerts.bmp280_temperature){
            pwm_set_gpio_level(BUZZER_A, wrap*0.02);
            pwm_set_gpio_level(BUZZER_B, wrap*0.02);
            pwm_set_gpio_level(LED_RED, wrap*0.05);
            matrix_alert(0.05);
            sleep_ms(50);
            pwm_set_gpio_level(BUZZER_A, 0);
            pwm_set_gpio_level(BUZZER_B, 0);
            pwm_set_gpio_level(LED_RED, 0);
            matrix_alert(0.0);
        }
        else{
            pwm_set_gpio_level(BUZZER_A, 0);
            pwm_set_gpio_level(BUZZER_B, 0);
            pwm_set_gpio_level(LED_RED, 0);
            matrix_alert(0.0);
        }

        // Atualiza o Display LCD
        // Frame que será reutilizado
        ssd1306_fill(&ssd, false);
        ssd1306_rect(&ssd, 0, 0, 128, 64, cor, !cor);
        // Mensagem superior (Nome do projeto e vagas ocupadas/totais)
        ssd1306_rect(&ssd, 0, 0, 128, 12, cor, cor); // Fundo preenchido
        ssd1306_draw_string(&ssd, "DogAtmos", 4, 3, true);

        switch(display_page){
            // AHT20 - Temperatura
            case 1:
                ssd1306_draw_string(&ssd, "1/5", 95, 3, true);
                ssd1306_draw_string(&ssd, "ATUAL: ", 4, 18, false);
                ssd1306_draw_string(&ssd, str_tmp_aht, 12+7*8, 18, false);
                ssd1306_draw_string(&ssd, "STATUS: ", 4, 28, false);
                // Simbolo de alerta
                make_alert_display(sensor_alerts.aht20_temperature, 4 + 8*8, 28);
                // Offset atual
                ssd1306_draw_string(&ssd, "OFFSET: ", 4, 38, false);
                ssd1306_draw_string(&ssd, str_tmp_aht, 4 + 8*8, 38, false);
                // Indicação inferior
                ssd1306_rect(&ssd, 51, 0, 128, 12, cor, cor); // Fundo preenchido
                ssd1306_draw_string(&ssd, "AHT-TEMPERATURA", 4, 53, true);
                break;

            // AHT20 - Umidade
            case 2:
                ssd1306_draw_string(&ssd, "2/5", 95, 3, true);
                ssd1306_draw_string(&ssd, "ATUAL: ", 4, 18, false);
                ssd1306_draw_string(&ssd, str_humi_aht, 12+7*8, 18, false);
                ssd1306_draw_string(&ssd, "STATUS: ", 4, 28, false);
                // Simbolo de alerta
                make_alert_display(sensor_alerts.aht20_humidity, 4 + 8*8, 28);
                // Offset atual
                ssd1306_draw_string(&ssd, "OFFSET: ", 4, 38, false);
                ssd1306_draw_string(&ssd, str_offset_humi_aht, 4 + 8*8, 38, false);
                // Indicação inferior
                ssd1306_rect(&ssd, 51, 0, 128, 12, cor, cor); // Fundo preenchido
                ssd1306_draw_string(&ssd, "AHT - UMIDADE", 4, 53, true);
                break;

            // BMP - Pressão
            case 3:
                ssd1306_draw_string(&ssd, "3/5", 95, 3, true);
                ssd1306_draw_string(&ssd, "ATUAL: ", 4, 18, false);
                ssd1306_draw_string(&ssd, str_press_bmp, 12+7*8, 18, false);
                ssd1306_draw_string(&ssd, "STATUS: ", 4, 28, false);
                // Simbolo de alerta
                make_alert_display(sensor_alerts.bmp280_pressure, 4 + 8*8, 28);
                // Offset atual
                ssd1306_draw_string(&ssd, "OFFSET: ", 4, 38, false);
                ssd1306_draw_string(&ssd, str_offset_press_bmp, 4 + 8*8, 38, false);
                // Indicação inferior
                ssd1306_rect(&ssd, 51, 0, 128, 12, cor, cor); // Fundo preenchido
                ssd1306_draw_string(&ssd, "BMP - PRESSAO", 4, 53, true);
                break;

            // BMP - Temperatura
            case 4:
                ssd1306_draw_string(&ssd, "4/5", 95, 3, true);
                ssd1306_draw_string(&ssd, "ATUAL: ", 4, 18, false);
                ssd1306_draw_string(&ssd, str_temp_bmp, 12+7*8, 18, false);
                ssd1306_draw_string(&ssd, "STATUS: ", 4, 28, false);
                // Simbolo de alerta
                make_alert_display(sensor_alerts.bmp280_temperature, 4 + 8*8, 28);
                // Offset atual
                ssd1306_draw_string(&ssd, "OFFSET: ", 4, 38, false);
                ssd1306_draw_string(&ssd, str_offset_temp_bmp, 4 + 8*8, 38, false);
                // Indicação inferior
                ssd1306_rect(&ssd, 51, 0, 128, 12, cor, cor); // Fundo preenchido
                ssd1306_draw_string(&ssd, "BMP-TEMPERATURA", 4, 53, true);
                break;

            // Tela de IP
            case 5:
                ssd1306_draw_string(&ssd, "5/5", 95, 3, true);
                ssd1306_draw_string(&ssd, "IP do servidor", 4, 24, false);
                ssd1306_draw_string(&ssd, ip_str, 4, 34, false);
                break;
        }

        

        ssd1306_send_data(&ssd); // Envia para o display

        sleep_ms(50);
    }

    cyw43_arch_deinit();
    return 0;
}