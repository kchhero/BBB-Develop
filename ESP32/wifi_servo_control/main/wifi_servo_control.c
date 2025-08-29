#include <stdio.h>
#include <string.h>
#include "esp_http_client.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_http_server.h"
#include "driver/uart.h"
#include "cJSON.h" // JSON 파싱을 위해 cJSON 라이브러리 추가

#define WIFI_SSID      "********"
#define WIFI_PASSWORD  "********"
#define UART_NUM       UART_NUM_2
#define UART_TXD_PIN   17 // 사용할 TX 핀 (보드에 맞게 변경)
#define UART_RXD_PIN   16 // 사용할 RX 핀 (보드에 맞게 변경)
#define BUF_SIZE       1024

static const char *TAG = "ESP32_WEB_SERVER";

// --- 웹 서버 핸들러 ---

// index.html 파일을 펌웨어에서 가져오기 위한 선언
extern const uint8_t index_html_start[] asm("_binary_index_html_start");
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");

// GET / 요청 핸들러: UI (index.html)를 전송
static esp_err_t root_get_handler(httpd_req_t *req)
{
    const uint32_t index_html_len = index_html_end - index_html_start;
    httpd_resp_set_type(req, "text/html");
    httpd_resp_send(req, (const char *)index_html_start, index_html_len);
    return ESP_OK;
}

// POST /api/servo 요청 핸들러: JSON 데이터를 받아 UART로 전송
static esp_err_t servo_post_handler(httpd_req_t *req)
{
    char content[128];
    int recv_len = httpd_req_recv(req, content, sizeof(content) - 1);
    if (recv_len <= 0) return ESP_FAIL;
    content[recv_len] = '\0';

    cJSON *root = cJSON_Parse(content);
    if (root == NULL) { /* ... 에러 처리 ... */ }

    cJSON *j_ch = cJSON_GetObjectItem(root, "channel");
    cJSON *j_on = cJSON_GetObjectItem(root, "on");
    cJSON *j_angle = cJSON_GetObjectItem(root, "angle");
    cJSON *j_speed = cJSON_GetObjectItem(root, "speed");

    if (cJSON_IsNumber(j_ch) && cJSON_IsNumber(j_on) && cJSON_IsNumber(j_angle) && cJSON_IsNumber(j_speed)) {
        char tx_buffer[64];
        // C:O:A:S 형식으로 시리얼 데이터 생성 (예: 8:1:-45:50)
        int len = snprintf(tx_buffer, sizeof(tx_buffer), "%d:%d:%d:%d\n", 
            j_ch->valueint, j_on->valueint, j_angle->valueint, j_speed->valueint);
        
        uart_write_bytes(UART_NUM, tx_buffer, len);
        ESP_LOGI(TAG, "Sent: %s", tx_buffer);
        httpd_resp_send(req, "OK", HTTPD_RESP_USE_STRLEN);
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid payload");
    }

    cJSON_Delete(root);
    return ESP_OK;
}

// 웹 서버 시작 함수 (URI 핸들러 등록)
static httpd_handle_t start_webserver(void)
{
     httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // =================================================================
        // 루트 경로("/")에 대한 GET 요청을 root_get_handler 함수가 처리하도록 등록합니다.
        httpd_uri_t root = {
            .uri      = "/",
            .method   = HTTP_METHOD_GET,
            .handler  = root_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root);
        // =================================================================
        httpd_uri_t root_post = {
            .uri      = "/",
            .method   = HTTP_METHOD_POST,
            .handler  = root_get_handler,
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &root_post);
        // =================================================================
        
        // API 경로 핸들러 등록
        httpd_uri_t servo_api = { 
            .uri      = "/api/servo", 
            .method   = HTTP_METHOD_POST, 
            .handler  = servo_post_handler, 
            .user_ctx = NULL 
        };
        httpd_register_uri_handler(server, &servo_api);
        
        // =================================================================
        httpd_uri_t servo_api_patch = {
            .uri      = "/api/servo",
            .method   = HTTP_METHOD_PATCH, // PATCH 요청을
            .handler  = servo_post_handler, // POST 핸들러와 동일한 함수로 처리
            .user_ctx = NULL
        };
        httpd_register_uri_handler(server, &servo_api_patch);

        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

/**
 * @brief Wi-Fi 이벤트 발생 시 호출되는 콜백 함수
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        ESP_LOGI(TAG, "Disconnected from Wi-Fi. Retrying to connect...");
        esp_wifi_connect();
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP address: " IPSTR, IP2STR(&event->ip_info.ip));
        start_webserver(); // IP를 할당받으면 웹 서버 시작
    }
}

/**
 * @brief Wi-Fi를 Station 모드로 초기화하고 접속합니다.
 */
void wifi_init_sta(void)
{
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL, &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL, &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}

/**
 * @brief UART 통신을 초기화합니다.
 */
static void uart_init(void) {
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // 2. 드라이버 설치
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    // 3. 파라미터 설정
    ESP_ERROR_CHECK(uart_param_config(UART_NUM, &uart_config));
    // 4. TX/RX 핀 지정
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM, UART_TXD_PIN, UART_RXD_PIN,
                                 UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
}

/**
 * @brief 애플리케이션 메인 함수
 */
void app_main(void)
{
    // NVS (Non-Volatile Storage) 초기화
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // UART 및 Wi-Fi 초기화
    uart_init();
    wifi_init_sta();
}
