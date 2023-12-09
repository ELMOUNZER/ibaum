#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_http_server.h"
#include "driver/gpio.h"
#include "driver/adc.h"

// Pin Definitions
#define RELAY_GPIO_PIN 25
#define LED_GPIO_PIN 2

// Soil Moisture Sensor Configuration
#define SOIL_SENSOR_ADC_CHANNEL ADC1_CHANNEL_7
#define PUMP_DURATION_SECONDS 5
#define BLINK_INTERVAL_MS 100
#define WET_LEVEL 2300
#define DRY_LEVEL 300


const char *ssid = "O2";
const char *pass = "T7YUCVBACHPA2YA6";

// Function Declarations
void controlWaterPump(bool turnOn);
void controlLed(bool turnOn);
void blinkLed(int blinkCount);
int map(int x, int in_min, int in_max, int out_min, int out_max);
int getSoilMoisture(void);
void soilTask(void *pvParameter);
int retry_num = 0;
httpd_handle_t start_webserver(void);
void stop_webserver(httpd_handle_t server);
void web_server_task(void *pvParameter);
void wifi_connection(void);

// Global Variables
SemaphoreHandle_t xButtonSemaphore;
int humidity = 0;
bool manualPumpStart = false;
bool pumpActive = false;
int moisture = 0;


static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data) {
    if (event_id == WIFI_EVENT_STA_START) {
        printf("WIFI CONNECTING....\n");
    } else if (event_id == WIFI_EVENT_STA_CONNECTED) {
        printf("WiFi CONNECTED\n");
    } else if (event_id == WIFI_EVENT_STA_DISCONNECTED) {
        printf("WiFi lost connection\n");
        if (retry_num < 5) {
            esp_wifi_connect();
            retry_num++;
            printf("Retrying to Connect...\n");
        }
    } else if (event_id == IP_EVENT_STA_GOT_IP) {
        printf("Wifi got IP...\n\n");
    }
}

esp_err_t custom_handler(httpd_req_t *req) {
    // Ensure that the moisture variable has a valid value before using it in the snprintf
    // For example, you can replace this with the actual value you want to display.
    int moisture = getSoilMoisture();

    // The HTML content is constructed in parts to make it more readable
    const char *html_head = "<html><body><button onclick=\"handleButtonPress()\">Press me</button>"
                            "<button onclick=\"stopPump()\">Stop Pump</button>";

    const char *html_moisture_script = "<p id=\"moisture\">Soil Moisture: <span id=\"moistureValue\"></span>%</p>"
                                       "<script>"
                                       "function handleButtonPress() {"
                                       "  fetch('/custom-button-press').then(response => response.text()).then(data => console.log(data));"
                                       "}"
                                       "function stopPump() {"
                                       "  fetch('/stop-pump').then(response => response.text()).then(data => console.log(data));"
                                       "}"
                                       "function updateMoisture() {"
                                       "  fetch('/get-moisture').then(response => response.text()).then(data => {"
                                       "    document.getElementById('moistureValue').innerText = data;"
                                       "  });"
                                       "}"
                                       "setInterval(updateMoisture, 1000);"
                                       "</script>";

    const char *html_end = "</body></html>";

    // Calculate the required buffer size for the HTML content
    size_t content_size = strlen(html_head) + strlen(html_moisture_script) + strlen(html_end) + 20; // 20 for extra characters and moisture value

    char *content = (char *)malloc(content_size);
    if (content == NULL) {
        ESP_LOGE("main", "Failed to allocate memory for HTML content");
        httpd_resp_send_500(req);
        return ESP_FAIL;
    }

    // Generate the HTML content
    snprintf(content, content_size, "%s%s%d%s", html_head, html_moisture_script, moisture, html_end);

    // Send the HTML content as the response
    httpd_resp_send(req, content, HTTPD_RESP_USE_STRLEN);

    // Free the allocated memory
    free(content);

    return ESP_OK;
}



esp_err_t button_press_handler(httpd_req_t *req) {
    ESP_LOGI("WebServer", "Custom button pressed from web interface!");

    // Set the flag to start the pump manually
    manualPumpStart = true;

    httpd_resp_send(req, "Custom button pressed from web interface!", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

esp_err_t stop_pump_handler(httpd_req_t *req) {
    ESP_LOGI("WebServer", "Stop Pump button pressed from web interface!");

    // Set the flag to stop the pump manually
    controlWaterPump(false);

    httpd_resp_send(req, "Stop Pump button pressed from web interface!", HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t stop_pump_uri = {
        .uri = "/stop-pump",
        .method = HTTP_GET,
        .handler = stop_pump_handler,
        .user_ctx = NULL
};


esp_err_t get_humidity_handler(httpd_req_t *req) {
    char content[20];
    snprintf(content, sizeof(content), "%d", humidity);
    httpd_resp_send(req, content, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}

httpd_uri_t custom_uri = {
        .uri = "/",
        .method = HTTP_GET,
        .handler = custom_handler,
        .user_ctx = NULL
};

httpd_uri_t button_press_uri = {
        .uri = "/custom-button-press",
        .method = HTTP_GET,
        .handler = button_press_handler,
        .user_ctx = NULL
};

httpd_uri_t get_humidity_uri = {
        .uri = "/get-humidity",
        .method = HTTP_GET,
        .handler = get_humidity_handler,
        .user_ctx = NULL
};

void wifi_connection() {
    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t wifi_initiation = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wifi_initiation);
    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL);

    wifi_config_t wifi_configuration = {
            .sta = {
                    .ssid = "O2",
                    .password = "T7YUCVBACHPA2YA6",
            }
    };

    strcpy((char *)wifi_configuration.sta.ssid, ssid);
    strcpy((char *)wifi_configuration.sta.password, pass);
    esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_configuration);
    esp_wifi_start();
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_connect();
    printf("wifi_init_softap finished. SSID:%s  password:%s", ssid, pass);
}

void humidity_task(void *pvParameter) {
    while (1) {
        vTaskDelay(5000 / portTICK_PERIOD_MS);  // Delay 5 seconds
        humidity += 10;
    }
}

httpd_handle_t start_webserver(void) {
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.server_port = 8080;  // Ändere den Server-Port auf 8080 oder einen anderen verfügbaren Port

    if (httpd_start(&server, &config) == ESP_OK) {
        httpd_register_uri_handler(server, &custom_uri);
        httpd_register_uri_handler(server, &button_press_uri);
        httpd_register_uri_handler(server, &get_humidity_uri);
        httpd_register_uri_handler(server, &stop_pump_uri);

        return server;
    }

    ESP_LOGI("WebServer", "Fehler beim Starten des Servers!");
    return NULL;
}


void stop_webserver(httpd_handle_t server) {
    httpd_stop(server);
}

void web_server_task(void *pvParameter) {
    httpd_handle_t server = start_webserver();
    xTaskCreate(&humidity_task, "humidity_task", 2048, NULL, 5, NULL);

    while (1) {
        // Check if the button was pressed to start the pump manually
        if (manualPumpStart) {
            printf("Manually starting pump...\n");
            // Assuming controlWaterPump function is available
            // Add your pump control logic here
            controlWaterPump(true);

            // Reset the manual pump start flag
            manualPumpStart = false;
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    stop_webserver(server);
}

void controlWaterPump(bool turnOn) {
    if (turnOn) {
        // Add logic to turn on the water pump
        printf("Turning on the water pump...\n");
        // Example: GPIO high to turn on the pump
        gpio_set_level(RELAY_GPIO_PIN, 1);
    } else {
        // Add logic to turn off the water pump
        printf("Turning off the water pump...\n");
        // Example: GPIO low to turn off the pump
        gpio_set_level(RELAY_GPIO_PIN, 0);
    }
}


void control_Led(bool turnOn) {
    gpio_set_level(LED_GPIO_PIN, turnOn ? 1 : 0);
}

void blinkLed(int blinkCount) {
    for (int i = 0; i < blinkCount; i++) {
        control_Led(i % 2 == 0);  // Toggle the LED state
        vTaskDelay(BLINK_INTERVAL_MS / portTICK_PERIOD_MS);
    }
}

int map(int x, int in_min, int in_max, int out_min, int out_max) {
    return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}

int getSoilMoisture() {
    adc1_config_width(ADC_WIDTH_BIT_12);
    adc1_config_channel_atten(SOIL_SENSOR_ADC_CHANNEL, ADC_ATTEN_DB_11);

    // Introduce a delay to allow the sensor to stabilize
    vTaskDelay(pdMS_TO_TICKS(1000));

    int adc_value = adc1_get_raw(SOIL_SENSOR_ADC_CHANNEL);
    printf("Raw ADC Value: %d\n", adc_value);

    // Assuming the sensor's output range is linear between wet and dry values
    int moisture_percentage = map(adc_value, WET_LEVEL, DRY_LEVEL, 0, 100);

    return moisture_percentage;
}

void soilTask(void *pvParameter) {
    bool pumpActive = false;
    int MOISTURE_THRESHOLD = 55;

    // Blink the LED rapidly to indicate startup
    blinkLed(10);  // You can adjust the blink count for a more noticeable startup blink

    // Turn on the LED initially
    control_Led(true);

    while (1) {
        // Placeholder for reading soil moisture
        moisture = getSoilMoisture();
        printf("Soil Moisture: %d%%\n", moisture);

        // Check if the moisture is below the threshold or the button was pressed
        if ((moisture < MOISTURE_THRESHOLD && !pumpActive) || manualPumpStart) {
            // Turn on the water pump
            controlWaterPump(true);

            // Set the flag to indicate the pump is active
            pumpActive = true;

            // Print a message indicating water pumping
            printf("The soil is dry. Pumping water...\n");

            // Turn off the LED when the pump is active
            double blinkCount = PUMP_DURATION_SECONDS * 1000 / BLINK_INTERVAL_MS;
            blinkLed(blinkCount);

            // Wait for the specified duration
            vTaskDelay(pdMS_TO_TICKS(PUMP_DURATION_SECONDS * 1000));

            // Turn off the water pump after the specified duration
            controlWaterPump(false);

            // Print a message indicating pump is off
            printf("Water pump is off.\n");

            // Turn on the LED after the pump is turned off
            control_Led(true);

            // Reset the manual pump start flag
            manualPumpStart = false;
        } else if (moisture >= MOISTURE_THRESHOLD) {
            // Reset the flag when moisture is above the threshold
            pumpActive = false;
        }

        // Wait for the next iteration
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void) {
    // Setup GPIO configurations for LED and Relay
    gpio_config_t led_io_conf = {
            .pin_bit_mask = (1ULL << LED_GPIO_PIN),
            .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config_t relay_io_conf = {
            .pin_bit_mask = (1ULL << RELAY_GPIO_PIN),
            .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&led_io_conf);
    gpio_config(&relay_io_conf);

    // Create tasks and initialize components
    nvs_flash_init();
    xButtonSemaphore = xSemaphoreCreateBinary();
    wifi_connection();
    xTaskCreate(&web_server_task, "web_server_task", 4096, NULL, 5, NULL);
    xTaskCreate(&soilTask, "soil_task", 2048, NULL, 5, NULL);
}