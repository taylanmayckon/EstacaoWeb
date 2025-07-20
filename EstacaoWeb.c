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

// Nome e senha da rede wi-fi
#define WIFI_SSID ""
#define WIFI_PASS ""

// GPIO utilizada
#define LED_PIN 12
#define BOTAO_A 5
#define BOTAO_JOY 22
#define JOYSTICK_X 26
#define JOYSTICK_Y 27

// Configurações da I2C e sensores
#define I2C_PORT_DISP i2c1
#define I2C_SDA_DISP 14
#define I2C_SCL_DISP 15
#define endereco 0x3C

const char HTML_BODY[] =
    "<!DOCTYPE html><html lang='pt-br'><head><meta charset='UTF-8'>"
    "<meta name='viewport' content='width=device-width, initial-scale=1.0'>"
    "<title>DogAtmos - EmbarcaTech</title>"

    "<style>"
    "body{font-family:-apple-system,BlinkMacSystemFont,'Segoe UI',Roboto,Helvetica,Arial,sans-serif;margin:0;padding:20px;background-color:#f0f2f5;color:#333}"
    "h1{text-align:center;color:#1e3a5f;margin-bottom:30px}"
    "h2{margin-top:0;color:#333;border-bottom:2px solid #e0e0e0;padding-bottom:5px;margin-bottom:15px}"
    ".main-content{max-width:1200px;margin:0 auto;display:flex;flex-direction:column;gap:20px}"
    ".sensor-row{display:flex;flex-wrap:wrap;align-items:center;justify-content:center;gap:20px;padding:20px;background-color:#fff;border-radius:8px;box-shadow:0 4px 6px rgba(0,0,0,.05)}"
    ".chart-container{flex:2;min-width:300px;max-width:700px}"
    ".chart-container canvas{width:100%!important;height:300px!important}"
    ".sensor-info{flex:1;min-width:250px;display:flex;flex-direction:column;gap:15px}"
    ".info-item,.input-group,.alert-status-container{background-color:#f8f9fa;padding:10px 15px;border-radius:6px;border:1px solid #e9ecef}"
    ".info-item p{margin:0;font-size:1.2em;font-weight:500}"
    ".info-item span{font-weight:400;color:#007bff}"
    ".input-group label,.alert-status-container label{display:block;margin-bottom:5px;font-weight:500}"
    ".input-group input{width:100%;padding:8px;border:1px solid #ced4da;border-radius:4px;box-sizing:border-box}"
    ".alert-indicator{padding:10px;border-radius:8px;text-align:center;font-weight:bold;color:#fff;background-color:#4CAF50;transition:background-color .3s ease,opacity .3s ease}"
    ".alert-indicator.triggered{background-color:#f44336;animation:blink 1.2s infinite ease-in-out}"
    "@keyframes blink{50%{opacity:.7}}"
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
    "const charts={};const MAX_DATA_POINTS=30;"
    "function initializeDashboard(){const e=document.getElementById('main-content');sensorConfig.forEach(function(t){var a=\""
    "<div class='sensor-row'><div class='chart-container'><h2>\"+t.sensorName+\" - \"+t.label+\"</h2><canvas id='\"+t.id+\"_chart'></canvas></div>"
    "<div class='sensor-info'><div class='info-item'><p>Valor Atual: <span id='current_\"+t.id+\"'>--</span> \"+t.unit+\"</p></div>"
    "<div class='input-group'><label for='min_\"+t.id+\"'>Limite Mínimo:</label><input type='number' id='min_\"+t.id+\"' placeholder='\"+t.placeholderMin+\"'></div>"
    "<div class='input-group'><label for='max_\"+t.id+\"'>Limite Máximo:</label><input type='number' id='max_\"+t.id+\"' placeholder='\"+t.placeholderMax+\"'></div>"
    "<div class='input-group'><label for='offset_\"+t.id+\"'>Offset de Calibração:</label><input type='number' id='offset_\"+t.id+\"' value='0'></div>"
    "<div class='alert-status-container'><label>Status do Alerta:</label><div id='alert_\"+t.id+\"' class='alert-indicator'>NORMAL</div></div></div></div>"
    "\";e.insertAdjacentHTML('beforeend',a);var n=t.color[0],l=t.color[1],o=t.color[2];charts[t.id]=new Chart(document.getElementById(t.id+'_chart').getContext('2d'),{type:'line',data:{labels:[],datasets:[{label:t.label,data:[],borderColor:'rgba('+n+','+l+','+o+',1)',backgroundColor:'rgba('+n+','+l+','+o+',0.2)',borderWidth:2,fill:!0,tension:.4}]},options:{responsive:!0,maintainAspectRatio:!1,scales:{x:{title:{display:!0,text:'Tempo'}},y:{title:{display:!0,text:t.label+' ['+t.unit+']'},beginAtZero:!1}},animation:{duration:500}}})})}"
    "function updateData(){fetch('/data').then(e=>e.json()).then(e=>{const t=(new Date).toLocaleTimeString('pt-BR');sensorConfig.forEach(function(a){const n=e[a.id];if(void 0!==n){const e=document.getElementById('offset_'+a.id),l=parseFloat(e.value)||0,o=n+l;document.getElementById('current_'+a.id).textContent=o.toFixed(2),addDataToChart(charts[a.id],t,o),checkAlerts(a.id,o)}})}).catch(e=>console.error('Error fetching sensor data:',e))}"
    "function checkAlerts(e,t){const a=document.getElementById('min_'+e),n=document.getElementById('max_'+e),l=document.getElementById('alert_'+e),o=parseFloat(a.value),r=parseFloat(n.value);let c=!isNaN(o)&&t<o||!isNaN(r)&&t>r;l.classList.toggle('triggered',c),l.textContent=c?'ALERTA!':'NORMAL'}"
    "function addDataToChart(e,t,a){e.data.labels.push(t),e.data.datasets[0].data.push(a),e.data.labels.length>MAX_DATA_POINTS&&(e.data.labels.shift(),e.data.datasets[0].data.shift()),e.update('none')}"
    "initializeDashboard();setInterval(updateData,2000);"
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

    if (strstr(req, "GET /led/on")){
        gpio_put(LED_PIN, 1);
        const char *txt = "Ligado";
        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           (int)strlen(txt), txt);
    }
    else if (strstr(req, "GET /led/off")){
        gpio_put(LED_PIN, 0);
        const char *txt = "Desligado";
        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: text/plain\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           (int)strlen(txt), txt);
    }
    else if (strstr(req, "GET /estado")){
        adc_select_input(0);
        uint16_t x = adc_read();
        adc_select_input(1);
        uint16_t y = adc_read();
        int botao = !gpio_get(BOTAO_A);
        int joy = !gpio_get(BOTAO_JOY);

        char json_payload[96];
        int json_len = snprintf(json_payload, sizeof(json_payload),
                                "{\"led\":%d,\"x\":%d,\"y\":%d,\"botao\":%d,\"joy\":%d}\r\n",
                                gpio_get(LED_PIN), x, y, botao, joy);

        printf("[DEBUG] JSON: %s\n", json_payload);

        hs->len = snprintf(hs->response, sizeof(hs->response),
                           "HTTP/1.1 200 OK\r\n"
                           "Content-Type: application/json\r\n"
                           "Content-Length: %d\r\n"
                           "Connection: close\r\n"
                           "\r\n"
                           "%s",
                           json_len, json_payload);
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

    gpio_init(LED_PIN);
    gpio_set_dir(LED_PIN, GPIO_OUT);

    gpio_init(BOTAO_A);
    gpio_set_dir(BOTAO_A, GPIO_IN);
    gpio_pull_up(BOTAO_A);

    gpio_init(BOTAO_JOY);
    gpio_set_dir(BOTAO_JOY, GPIO_IN);
    gpio_pull_up(BOTAO_JOY);

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

    start_http_server();
    char str_x[5]; // Buffer para armazenar a string
    char str_y[5]; // Buffer para armazenar a string
    bool cor = true;
    while (true){
        cyw43_arch_poll();

        sleep_ms(300);
    }

    cyw43_arch_deinit();
    return 0;
}