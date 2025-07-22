#include "pico/cyw43_arch.h"
#include "pico/stdlib.h"
#include "lwip/tcp.h"
#include "hardware/i2c.h"
#include "hardware/adc.h"
#include "hardware/gpio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ssd1306.h"
#include "font.h"
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

// Configurações da I2C do display
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C
bool cor = true;

// Configurações da I2C dos sensores
#define I2C_PORT i2c0
#define I2C_SDA 0
#define I2C_SCL 1
#define SEA_LEVEL_PRESSURE 101325.0 // Pressão ao nível do mar em Pa

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


// Configurações para o PWM
uint wrap = 2000;
uint clkdiv = 25;

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
    ".alert-indicator{padding:10px;border-radius:8px;text-align:center;font-weight:bold;color:white;background-color:#4CAF50;transition:background-color 0.3s ease,opacity 0.3s ease}"
    ".alert-indicator.triggered{background-color:#f44336;animation:blink 1.2s infinite ease-in-out}"
    "@keyframes blink{50%{opacity:0.7}}"
    "</style>"

    "<script src='https://cdn.jsdelivr.net/npm/chart.js'></script></head>"

    "<body><h1>DogAtmos - EmbarcaTech</h1>"
    "<div id='main-content' class='main-content'></div>"
    "<div style='text-align:center;max-width:1200px;margin:40px auto 20px;'>"
    "<hr style='border:0;height:1px;background-color:#ddd;'>"
    "<h3>Desenvolvido por: Taylan Mayckon</h3>"
    "<p>Atividade da Fase 2 do EmbarcaTech, envolvendo uso dos sensores BMP280 e AHT20 para criar uma estação meteorológica com interface WEB.</p>"
    "</div>"

    "<script>"
    "const sensorConfig=[{id:'AHT20_temperature',sensorName:'AHT20',label:'Temperatura',unit:'°C',color:[255,99,132],placeholderMin:'Ex: 10',placeholderMax:'Ex: 30'},{id:'AHT20_humidity',sensorName:'AHT20',label:'Umidade',unit:'%',color:[54,162,235],placeholderMin:'Ex: 40',placeholderMax:'Ex: 70'},{id:'BMP280_pressure',sensorName:'BMP280',label:'Pressão',unit:'hPa',color:[75,192,192],placeholderMin:'Ex: 1000',placeholderMax:'Ex: 1020'},{id:'BMP280_temperature',sensorName:'BMP280',label:'Temperatura',unit:'°C',color:[255,159,64],placeholderMin:'Ex: 10',placeholderMax:'Ex: 30'}];"
    "const charts={};const fetchInterval=2000;"
    "function initializeDashboard(){const t=document.getElementById('main-content');sensorConfig.forEach(e=>{const n=\""
    "<div class='sensor-row'><div class='chart-container'><h2>\"+e.sensorName+\" - \"+e.label+\"</h2><canvas id='\"+e.id+\"_chart'></canvas></div>"
    "<div class='sensor-info'><div class='info-item'><p>Valor Atual: <span id='current_\"+e.id+\"'>--</span> \"+e.unit+\"</p></div>"
    "<div class='input-group'><label for='min_\"+e.id+\"'>Limite Mínimo:</label><input type='number' id='min_\"+e.id+\"' placeholder='\"+e.placeholderMin+\"'></div>"
    "<div class='input-group'><label for='max_\"+e.id+\"'>Limite Máximo:</label><input type='number' id='max_\"+e.id+\"' placeholder='\"+e.placeholderMax+\"'></div>"
    "<div class='input-group'><label for='offset_\"+e.id+\"'>Offset de Calibração:</label><input type='number' id='offset_\"+e.id+\"' value='0'></div>"
    "<div class='alert-status-container'><label>Status do Alerta:</label><div id='alert_\"+e.id+\"' class='alert-indicator'>NORMAL</div></div></div></div>"
    "\";t.insertAdjacentHTML('beforeend',n);const[o,a,r]=e.color;charts[e.id]=new Chart(document.getElementById(e.id+'_chart').getContext('2d'),{type:'line',data:{labels:[],datasets:[{label:e.label,data:[],borderColor:'rgba('+o+','+a+','+r+',1)',backgroundColor:'rgba('+o+','+a+','+r+',0.2)',borderWidth:2,fill:!0,tension:.4}]},options:{responsive:!0,maintainAspectRatio:!1,scales:{x:{title:{display:!0,text:'Tempo'}},y:{title:{display:!0,text:e.label+' ['+e.unit+']'},beginAtZero:!1}},animation:{duration:500}}})})}"
    "function sendInputs(){}"
    "function updateData(){fetch('/data').then(e=>e.json()).then(e=>{const t=new Date;sensorConfig.forEach(o=>{const n=e[o.id],a=e['alert_'+o.id];updateCharts(charts[o.id],t,n),document.getElementById('current_'+o.id).textContent=parseFloat(n.at(-1)).toFixed(2),checkAlerts(o.id,a)})})}"
    "function checkAlerts(e,t){const o=document.getElementById('alert_'+e);let n='on'===t;o.classList.toggle('triggered',n),o.textContent=n?'ALERTA!':'NORMAL'}"
    "function updateCharts(e,t,o){const n=[];const a=o.length;for(let r=0;r<a;r++){const s=new Date(t.getTime()-(a-1-r)*fetchInterval);n.push(s.toLocaleTimeString('pt-BR'))}e.data.labels=n,e.data.datasets[0].data=o,e.update('none')}"
    "initializeDashboard();updateData();setInterval(updateData,fetchInterval);"
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

static err_t http_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err){
    if (!p){
        tcp_close(tpcb);
        return ERR_OK;
    }

    char *req = (char *)p->payload;
    struct http_state *hs = malloc(sizeof(struct http_state));
    if (!hs){
        pbuf_free(p);
        tcp_close(tpcb);
        return ERR_MEM;
    }
    hs->sent = 0;

    // Dados atuais dos dois sensores
    if (strstr(req, "GET /data")){
       // Atualizando o payload
        payload_sizes.json_size = sizeof(json_payload);
        payload_sizes.aht20_humi_size = sizeof(AHT20_buffer.humidity);
        payload_sizes.aht20_temp_size = sizeof(AHT20_buffer.temperature);
        payload_sizes.bmp280_press_size = sizeof(BMP280_buffer.pressure);
        payload_sizes.bmp280_temp_size = sizeof(BMP280_buffer.temperature);
        int json_len = payload_generate_json(json_payload, sensor_alerts, &AHT20_buffer, &BMP280_buffer, payload_sizes);

        // Debug 
        if (json_len > 0 && json_len < sizeof(json_payload)) {
            printf("JSON gerado com sucesso (%d bytes):\n", json_len);
            printf("%s\n", json_payload);
        } else {
            printf("Erro ao gerar o JSON: buffer pequeno demais!\n");
        }

        hs->len = snprintf(hs->response, sizeof(hs->response),
                            "HTTP/1.1 200 OK\r\n"
                            "Content-Type: application/json\r\n"
                            "Content-Length: %d\r\n"
                            "Connection: close\r\n"
                            "\r\n"
                            "%s",
                            json_len, json_payload);
    }
    else if(strstr(req, "GET /config")){

    }
    else { 
        hs->len = snprintf(hs->response, sizeof(hs->response),
                        "HTTP/1.1 200 OK\r\n"
                        "Content-Type: text/html\r\n"
                        "Content-Length: %d\r\n"
                        "Connection: close\r\n"
                        "\r\n"
                        "%s",
                        (int)strlen(HTML_BODY), HTML_BODY);
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


int main(){
    stdio_init_all();
    sleep_ms(2000);

    // Iniciando o I2C dos sensores
    i2c_init(I2C_PORT_DISP, 400 * 1000);
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

    // Iniciando o display
    i2c_init(I2C_PORT_DISP, 400 * 1000);
    gpio_set_function(I2C_SDA_DISP, GPIO_FUNC_I2C);
    gpio_set_function(I2C_SCL_DISP, GPIO_FUNC_I2C);
    gpio_pull_up(I2C_SDA_DISP);
    gpio_pull_up(I2C_SCL_DISP);

    ssd1306_t ssd;
    ssd1306_init(&ssd, WIDTH, HEIGHT, false, endereco, I2C_PORT_DISP);
    ssd1306_config(&ssd);
    ssd1306_fill(&ssd, false);
    ssd1306_draw_string(&ssd, "Iniciando Wi-Fi", 0, 0, false);
    ssd1306_draw_string(&ssd, "Aguarde...", 0, 30, false);    
    ssd1306_send_data(&ssd);

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

    // Iniciando os buffers do payload
    payload_buffers_init(&AHT20_buffer, &BMP280_buffer);

    start_http_server();

    while (true){
        cyw43_arch_poll();

        

        sleep_ms(300);
    }

    cyw43_arch_deinit();
    return 0;
}