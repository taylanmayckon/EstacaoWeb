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

static const char HTML_INDEX[] = 
    "";

static const char HTML_BMP280[] = 
    "";

static const char HTML_AHT20[] = 
    "";

static const char HTML_BODY[] =
    "<!DOCTYPE html>\n"
    "<html lang=\"pt-br\">\n"
    "<head>\n"
    "<meta charset=\"UTF-8\">\n"
    "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
    "<title>DogAtmos</title>\n"
    "<style>\n"
    "body{font-family:sans-serif;background-color:#f0f2f5;text-align:center;padding:15px;}\n"
    "h1{color:#1e3a5f;margin-top:0;} h2{font-size:1.2em;margin-bottom:5px;border-bottom:1px solid #ddd;padding-bottom:5px;}\n"
    ".container{max-width:400px;margin:0 auto;}\n"
    ".sensor-block{background-color:#fff;padding:15px;margin-bottom:15px;border-radius:8px;box-shadow:0 2px 4px rgba(0,0,0,.1);}\n"
    ".data-p{font-size:1.4em;margin:10px 0;} .data-p span{color:#007bff;font-weight:bold;}\n"
    ".alert-span{font-weight:bold;} .alert-ok{color:green;} .alert-triggered{color:red;}\n"
    "input{width:90%;padding:8px;margin-top:5px;margin-bottom:10px;border:1px solid #ccc;border-radius:4px;}\n"
    "label{font-size:0.9em;}\n"
    ".chart-container{width:100%;margin-top:10px;margin-bottom:10px;}\n"
    "</style>\n"
    "<script src=\"https://cdn.jsdelivr.net/npm/chart.js\"></script>\n"
    "</head>\n"
    "<body>\n"
    "<div class=\"container\">\n"
    "<h1>DogAtmos - EmbarcaTech</h1>\n"
    "<div class=\"sensor-block\">\n"
    "<h2>AHT20 - Temperatura</h2>\n"
    "<p class=\"data-p\">Atual: <span class=\"data-value\" id=\"current_AHT20_temperature\">--</span> &deg;C</p>\n"
    "<div class=\"chart-container\"><canvas id=\"AHT20_temperature_chart\"></canvas></div>\n"
    "<label>Limites:</label> <input type=\"number\" id=\"min_AHT20_temperature\" placeholder=\"Min\"> <input type=\"number\" id=\"max_AHT20_temperature\" placeholder=\"Max\"><br>\n"
    "Status: <span class=\"alert-span alert-ok\" id=\"alert_AHT20_temperature\">NORMAL</span>\n"
    "</div>\n"
    "<div class=\"sensor-block\">\n"
    "<h2>AHT20 - Umidade</h2>\n"
    "<p class=\"data-p\">Atual: <span class=\"data-value\" id=\"current_AHT20_humidity\">--</span> %</p>\n"
    "<div class=\"chart-container\"><canvas id=\"AHT20_humidity_chart\"></canvas></div>\n"
    "<label>Limites:</label> <input type=\"number\" id=\"min_AHT20_humidity\" placeholder=\"Min\"> <input type=\"number\" id=\"max_AHT20_humidity\" placeholder=\"Max\"><br>\n"
    "Status: <span class=\"alert-span alert-ok\" id=\"alert_AHT20_humidity\">NORMAL</span>\n"
    "</div>\n"
    "<div class=\"sensor-block\">\n"
    "<h2>BMP280 - Pressão</h2>\n"
    "<p class=\"data-p\">Atual: <span class=\"data-value\" id=\"current_BMP280_pressure\">--</span> hPa</p>\n"
    "<div class=\"chart-container\"><canvas id=\"BMP280_pressure_chart\"></canvas></div>\n"
    "<label>Limites:</label> <input type=\"number\" id=\"min_BMP280_pressure\" placeholder=\"Min\"> <input type=\"number\" id=\"max_BMP280_pressure\" placeholder=\"Max\"><br>\n"
    "Status: <span class=\"alert-span alert-ok\" id=\"alert_BMP280_pressure\">NORMAL</span>\n"
    "</div>\n"
    "<div class=\"sensor-block\">\n"
    "<h2>BMP280 - Temperatura</h2>\n"
    "<p class=\"data-p\">Atual: <span class=\"data-value\" id=\"current_BMP280_temperature\">--</span> &deg;C</p>\n"
    "<div class=\"chart-container\"><canvas id=\"BMP280_temperature_chart\"></canvas></div>\n"
    "<label>Limites:</label> <input type=\"number\" id=\"min_BMP280_temperature\" placeholder=\"Min\"> <input type=\"number\" id=\"max_BMP280_temperature\" placeholder=\"Max\"><br>\n"
    "Status: <span class=\"alert-span alert-ok\" id=\"alert_BMP280_temperature\">NORMAL</span>\n"
    "</div>\n"
    "</div>\n"
    "<script>\n"
    "function createChartConfig(l,u,r,g,b){return{type:'line',data:{labels:[],datasets:[{label:l,data:[],borderColor:`rgba(${r},${g},${b},1)`,backgroundColor:`rgba(${r},${g},${b},.2)`,borderWidth:2,fill:true,tension:.4}]},options:{responsive:true,maintainAspectRatio:false,scales:{x:{title:{display:false}},y:{title:{display:false}}},plugins:{legend:{display:false}},animation:{duration:500}}}}\n"
    "const charts={AHT20_temperature:new Chart(document.getElementById('AHT20_temperature_chart').getContext('2d'),createChartConfig('Temp','°C',255,99,132)),AHT20_humidity:new Chart(document.getElementById('AHT20_humidity_chart').getContext('2d'),createChartConfig('Umid','%',54,162,235)),BMP280_pressure:new Chart(document.getElementById('BMP280_pressure_chart').getContext('2d'),createChartConfig('Press','hPa',75,192,192)),BMP280_temperature:new Chart(document.getElementById('BMP280_temperature_chart').getContext('2d'),createChartConfig('Temp','°C',255,159,64))};\n"
    "const MAX_DATA_POINTS=15;\n"
    "function updateData(){fetch('/dados.json').then(r=>r.json()).then(d=>{const t=new Date().toLocaleTimeString('pt-BR');for(const s in d){const v=d[s];document.getElementById(`current_${s}`).textContent=v.toFixed(2);updateChart(charts[s],t,v);checkAlerts(s,v)}}).catch(e=>console.error('Erro:',e));}\n"
    "function updateChart(c,l,d){c.data.labels.push(l);c.data.datasets.forEach(s=>{s.data.push(d)});if(c.data.labels.length>MAX_DATA_POINTS){c.data.labels.shift();c.data.datasets.forEach(s=>{s.data.shift()})}c.update('none');}\n"
    "function checkAlerts(i,v){const min=document.getElementById(`min_${i}`).value,max=document.getElementById(`max_${i}`).value,a=document.getElementById(`alert_${i}`);let t=!1;(max&&v>parseFloat(max))&&(t=!0);(min&&v<parseFloat(min))&&(t=!0);a.textContent=t?'ALERTA!':'NORMAL';a.className=`alert-span ${t?'alert-triggered':'alert-ok}`};"
    "async function sendConfig(baseId){const minVal=document.getElementById(`min_${baseId}`).value;const maxVal=document.getElementById(`max_${baseId}`).value;const config={sensor:baseId,min:parseFloat(minVal),max:parseFloat(maxVal)};await fetch('/config',{method:'POST',headers:{'Content-Type':'application/json'},body:JSON.stringify(config)});}\n"
    "document.addEventListener('DOMContentLoaded',()=>{updateData();setInterval(updateData,2500);document.querySelectorAll('input').forEach(i=>{i.addEventListener('change',e=>{const baseId=e.target.id.split('_').slice(1).join('_');sendConfig(baseId)})})});\n"
    "</script>\n"
    "</body>\n"
    "</html>";

struct http_state{
    char response[4096*2];
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
    else{
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