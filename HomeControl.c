// ================================ BIBLIOTECAS ================================
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "pico/cyw43_arch.h"
#include "hardware/clocks.h"
#include "pico/bootrom.h"

#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"

// Bibliotecas personalizadas do projeto
#include "lib/ws2812b.h"
#include "lib/rgb.h"
#include "lib/oledgfx.h"
#include "lib/push_button.h"

// ================================ CONFIGURAÇÕES Wi-Fi ================================
// Credenciais WIFI - Tome cuidado se publicar no github!
#define WIFI_SSID "Nilson"
#define WIFI_PASSWORD "casaesnet"

// ================================ DEFINIÇÕES DE PINOS ================================
// Definição dos pinos dos LEDs
#define LED_PIN CYW43_WL_GPIO_LED_PIN   // GPIO do CI CYW43 (LED interno da Pico W)
#define LED_BLUE_PIN 12                 // GPIO12 - LED azul
#define LED_GREEN_PIN 11                // GPIO11 - LED verde
#define LED_RED_PIN 13                  // GPIO13 - LED vermelho

// Configuração do barramento I2C
#define I2C_PORT i2c1

// ================================ CONFIGURAÇÕES DO DISPLAY OLED ================================
/// @brief OLED display pin configuration
#define OLED_SDA 14      ///< Serial data pin (pino de dados serial)
#define OLED_SCL 15      ///< Serial clock pin (pino de clock serial)
#define OLED_ADDR 0x3C   ///< I2C address of OLED (endereço I2C do OLED)
#define OLED_BAUDRATE 400000  ///< I2C communication speed (velocidade de comunicação I2C)

// ================================ MACROS E FUNÇÕES UTILITÁRIAS ================================
// Macro para entrar em modo bootsel (reinicialização por USB)
#define set_bootsel_mode() reset_usb_boot(0, 0)

// ================================ VARIÁVEIS GLOBAIS ================================
// Padrão de bits para acender todas as luzes (5x5 matriz)
static uint8_t all_lights_glyph[] = { 1, 1, 1, 1, 1,
                               1, 1, 1, 1, 1,
                               1, 1, 1, 1, 1,
                               1, 1, 1, 1, 1,
                               1, 1, 1, 1, 1
                            };

// Ponteiros globais para os periféricos (para acesso nas callbacks)
static ws2812b_t *ws_global = NULL;   // Ponteiro global para WS2812B (fita LED)
static rgb_t *rgb_global = NULL;      // Ponteiro global para LEDs RGB
static ssd1306_t *ssd_global = NULL;  // Ponteiro global para display OLED

// ================================ DECLARAÇÕES DE FUNÇÕES ================================
// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);

// Função de callback para processar requisições HTTP
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);

// Leitura da temperatura interna do microcontrolador
float temp_read(void);

// Tratamento das requisições do usuário (controle dos dispositivos)
void user_request(char **request);

/**
 * @brief GPIO interrupt callback for button presses
 * @param gpio GPIO pin that triggered interrupt
 * @param event Type of interrupt event
 */
static void gpio_irq_callback(uint gpio, uint32_t event);

void vSystemMonitorTask(void *pvParameters);
void vSystemInit(void);
void vUpdateSystemStatusPage(void);
void init_web_server(void);
void liquid_level_control_task(void *pvParameters);
int read_water_level(void);
void vLDRTask(void *pvParameters);
void vPIRTask(void *pvParameters);

// Função principal
int main()
{
    //Inicializa todos os tipos de bibliotecas stdio padrão presentes que estão ligados ao binário.
    stdio_init_all();

    //Inicializa a arquitetura do cyw43
    while (cyw43_arch_init())
    {
        printf("Falha ao inicializar Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }

    // GPIO do CI CYW43 em nível baixo
    cyw43_arch_gpio_put(LED_PIN, 0);

    // Ativa o Wi-Fi no modo Station, de modo a que possam ser feitas ligações a outros pontos de acesso Wi-Fi.
    cyw43_arch_enable_sta_mode();

    // Conectar à rede WiFI - fazer um loop até que esteja conectado
    printf("Conectando ao Wi-Fi...\n");
    while (cyw43_arch_wifi_connect_timeout_ms(WIFI_SSID, WIFI_PASSWORD, CYW43_AUTH_WPA2_AES_PSK, 20000))
    {
        printf("Falha ao conectar ao Wi-Fi\n");
        sleep_ms(100);
        return -1;
    }
    printf("Conectado ao Wi-Fi\n");

    // Caso seja a interface de rede padrão - imprimir o IP do dispositivo.
    if (netif_default)
    {
        printf("IP do dispositivo: %s\n", ipaddr_ntoa(&netif_default->ip_addr));
    }

    // Configura o servidor TCP - cria novos PCBs TCP. É o primeiro passo para estabelecer uma conexão TCP.
    struct tcp_pcb *server = tcp_new();
    if (!server)
    {
        printf("Falha ao criar servidor TCP\n");
        return -1;
    }

    //vincula um PCB (Protocol Control Block) TCP a um endereço IP e porta específicos.
    if (tcp_bind(server, IP_ADDR_ANY, 80) != ERR_OK)
    {
        printf("Falha ao associar servidor TCP à porta 80\n");
        return -1;
    }

    // Coloca um PCB (Protocol Control Block) TCP em modo de escuta, permitindo que ele aceite conexões de entrada.
    server = tcp_listen(server);

    // Define uma função de callback para aceitar conexões TCP de entrada. É um passo importante na configuração de servidores TCP.
    tcp_accept(server, tcp_server_accept);
    printf("Servidor ouvindo na porta 80\n");

    // Inicializa o conversor ADC
    adc_init();
    adc_set_temp_sensor_enabled(true);

    while (true)
    {
        /* 
        * Efetuar o processamento exigido pelo cyw43_driver ou pela stack TCP/IP.
        * Este método deve ser chamado periodicamente a partir do ciclo principal 
        * quando se utiliza um estilo de sondagem pico_cyw43_arch 
        */
        cyw43_arch_poll(); // Necessário para manter o Wi-Fi ativo
        sleep_ms(100);      // Reduz o uso da CPU
    }

    //Desligar a arquitetura CYW43.
    cyw43_arch_deinit();
    return 0;
}

// -------------------------------------- Funções ---------------------------------

// Função de callback ao aceitar conexões TCP
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
    tcp_recv(newpcb, tcp_server_recv);
    return ERR_OK;
}
void user_request(char **request){
    printf("Processando requisição: %s\n", *request);
    
    if (strstr(*request, "GET /ligar_arcondicionado") != NULL)
    {
        printf("Comando: Ligar Ar Condicionado\n");
        // TODO: Implementar controle do ar condicionado
    }
    else if (strstr(*request, "GET /ligar_luzes") != NULL)  // CORRIGIDO: era /ligar_lampadas
    {
        printf("Comando: Ligar Luzes\n");
        // TODO: Implementar controle das luzes
    }
    else if (strstr(*request, "GET /sleep_arcondicionado") != NULL)
    {
        printf("Comando: Sleep Ar Condicionado\n");
        // TODO: Implementar modo sleep do ar condicionado
    }
    else if (strstr(*request, "GET /desligar_lampadas") != NULL)
    {
        printf("Comando: Desligar Lâmpadas\n");
        // TODO: Implementar desligamento das lâmpadas
    }
    else if (strstr(*request, "GET /desligar_bomba_agua") != NULL)  // CORRIGIDO: era /desligar_bomba
    {
        printf("Comando: Desligar Bomba d'Água\n");
        // TODO: Implementar controle da bomba d'água
    }
};

/**
 * @brief Leitura da temperatura interna do microcontrolador
 * @return Temperatura em graus Celsius
 * 
 * Usa o sensor de temperatura interno do RP2040 via ADC
 */
float temp_read(void){
    adc_select_input(4);                                    // Seleciona canal 4 do ADC (sensor de temperatura)
    uint16_t raw_value = adc_read();                        // Lê valor bruto do ADC (0-4095)
    const float conversion_factor = 3.3f / (1 << 12);      // Fator de conversão (3.3V / 4096)
    // Fórmula de conversão específica do RP2040 para temperatura
    float temperature = 27.0f - ((raw_value * conversion_factor) - 0.706f) / 0.001721f;
    return temperature;
}

/**
 * @brief Função de callback para processar requisições HTTP
 * @param arg Argumento adicional (não usado)
 * @param tpcb PCB TCP da conexão
 * @param p Buffer de pacote recebido
 * @param err Código de erro
 * @return Código de erro
 * 
 * Esta é a função principal que processa as requisições HTTP e gera as respostas
 */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
    // ========== VERIFICAÇÃO DE CONEXÃO ==========
    // Se não há dados, fecha a conexão
    if (!p)
    {
        tcp_close(tpcb);        // Fecha a conexão TCP
        tcp_recv(tpcb, NULL);   // Remove o callback de recepção
        return ERR_OK;
    }

    // ========== PROCESSAMENTO DA REQUISIÇÃO ==========
    // Alocação do request na memória dinâmica
    char *request = (char *)malloc(p->len + 1);    // Aloca memória para a requisição
    memcpy(request, p->payload, p->len);           // Copia dados do buffer de rede
    request[p->len] = '\0';                        // Adiciona terminador de string

    printf("Request: %s\n", request);              // Debug: imprime a requisição

    // ========== CONTROLE DOS DISPOSITIVOS ==========
    // Tratamento de request - Controle dos LEDs e outros dispositivos
    user_request(&request);
    
    // ========== LEITURA DE SENSORES ==========
    // Leitura da temperatura interna para exibir no dashboard
    float temperature = temp_read();

    // ========== GERAÇÃO DA RESPOSTA HTML ==========
    // Cria a resposta HTML (buffer grande para a página completa)
    char html[8400];

    // ========== GERAÇÃO DA PÁGINA WEB ==========
    // Instruções HTML do webserver - página completa de automação residencial
        // Instruções html do webserver - CORRIGIDO
    snprintf(html, 4096, // Formatar uma string e armazená-la em um buffer de caracteres
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=UTF-8\r\n"
        "\r\n"
        "<!DOCTYPE html>\n"
        "<html lang=\"pt\">\n"
        "<head>\n"
        "<meta charset=\"UTF-8\">\n"
        "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\n"
        "<meta http-equiv=\"X-UA-Compatible\" content=\"ie=edge\">\n"
        "<title>HomeControl - Automação Residencial</title>\n"
        "<style>\n"
        "body {\n"
        "background-color: #000000;\n"
        "font-family: 'Arial', sans-serif;\n"
        "text-align: center;\n"
        "margin-top: 20px;\n"
        "color: #333;\n"
        "}\n"
        "h1 {\n"
        "font-size: 48px;\n"
        "color: #2b78e4;\n"
        "margin-bottom: 30px;\n"
        "font-weight: bold;\n"
        "}\n"
        "h2 {\n"
        "color: #2b78e4;\n"
        "}\n"
        "button {\n"
        "background-color: #4CAF50;\n"
        "font-size: 24px;\n"
        "margin: 10px;\n"
        "padding: 15px 30px;\n"
        "border-radius: 10px;\n"
        "color: white;\n"
        "border: none;\n"
        "cursor: pointer;\n"
        "transition: background-color 0.3s ease;\n"
        "}\n"
        "button:hover {\n"
        "background-color: #45a049;\n"
        "}\n"
        "button:active {\n"
        "background-color: #388e3c;\n"
        "}\n"
        ".temperature {\n"
        "font-size: 36px;\n"
        "margin-top: 30px;\n"
        "color: #FF5722;\n"
        "font-weight: bold;\n"
        "}\n"
        ".control-panel {\n"
        "display: grid;\n"
        "grid-template-columns: repeat(2, 1fr);\n"
        "gap: 15px;\n"
        "max-width: 600px;\n"
        "margin: 0 auto;\n"
        "}\n"
        ".form-container {\n"
        "padding: 20px;\n"
        "background-color: #fff;\n"
        "border-radius: 10px;\n"
        "box-shadow: 0 4px 8px rgba(0, 0, 0, 0.1);\n"
        "}\n"
        ".control-panel form {\n"
        "margin: 10px 0;\n"
        "}\n"
        "</style>\n"
        "</head>\n"
        "<body>\n"
        "<h1>HomeControl - Automação Residencial</h1>\n"
        "<div class=\"control-panel\">\n"
        "<div class=\"form-container\">\n"
        "<h2>Controle de Dispositivos</h2>\n"
        "<form action=\"./ligar_arcondicionado\">\n"
        "<button type=\"submit\">Ligar Ar Condicionado</button>\n"
        "</form>\n"
        "<form action=\"./ligar_luzes\">\n"
        "<button type=\"submit\">Ligar Luzes</button>\n"
        "</form>\n"
        "<form action=\"./sleep_arcondicionado\">\n"
        "<button type=\"submit\">Sleep para o Ar Condicionado</button>\n"
        "</form>\n"
        "<form action=\"./desligar_lampadas\">\n"
        "<button type=\"submit\">Desligamento Automático das Lâmpadas</button>\n"
        "</form>\n"
        "<form action=\"./desligar_bomba_agua\">\n"
        "<button type=\"submit\">Desligar a Bomba d'Água</button>\n"
        "</form>\n"
        "</div>\n"
        "<div class=\"form-container\">\n"
        "<h2>Monitoramento</h2>\n"
        "<p class=\"temperature\">Temperatura Interna: %.2f °C</p>\n"
        "<p class=\"temperature\">Nível de Água: 87 cm</p>\n"
        "</div>\n"
        "</div>\n"
        "</body>\n"
        "</html>\n", temperature);

    // ========== ENVIO DA RESPOSTA HTTP ==========
    // Escreve dados para envio (mas não os envia imediatamente)
    // TCP_WRITE_FLAG_COPY copia os dados para buffer interno do TCP
    tcp_write(tpcb, html, strlen(html), TCP_WRITE_FLAG_COPY);

    // Envia a mensagem efetivamente pela rede
    tcp_output(tpcb);

    // ========== LIMPEZA DE MEMÓRIA ==========
    // Libera memória alocada dinamicamente para a requisição
    free(request);
    
    // Libera um buffer de pacote (pbuf) que foi alocado anteriormente
    pbuf_free(p);

    return ERR_OK; // Retorna sucesso
}

void vSystemMonitorTask(void *pvParameters)
{
    // TODO
}

void vSystemInit(void)
{
    // TODO
}

void vUpdateSystemStatusPage(void)
{
    // TODO
}

void init_web_server(void)
{
    // TODO
}

void liquid_level_control_task(void *pvParameters)
{
    // TODO
}

int read_water_level(void)
{
    return 0; // TODO
}

void vLDRTask(void *pvParameters)
{
    // TODO
}

void vPIRTask(void *pvParameters)
{
    // TODO
}