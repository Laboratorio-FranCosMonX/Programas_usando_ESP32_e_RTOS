#include <stdio.h> 
#include <string.h>
#include "freertos/FreeRTOS.h" 
#include "esp_system.h" 
#include "esp_wifi.h" 
#include "esp_log.h" 
#include "esp_event.h" 
#include "nvs_flash.h" 
#include "lwip/err.h" 
#include "lwip/sys.h" 

#include "dht.h"
#define DHT_PIN 23

#define TAG "AULA 10"
#define RETRY_NUM 5
#define SSID "Gato Preto"
#define PASSWORD "89324312"

int retry_num = 0;

static void wifi_event_handler(void *event_handler_arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{

    if (event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "Conectando ...");
        esp_wifi_connect();
    }

    else if (event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "Conectado a rede local");
    }

    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "Desconectado a rede local");
        if (retry_num < RETRY_NUM)
        {
            esp_wifi_connect();
            retry_num++;
            ESP_LOGI(TAG, "Reconectando ...");
        }
    }

    else if (event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;

        ESP_LOGI(TAG, "IP: " IPSTR,IP2STR(&event->ip_info.ip));
        retry_num = 0;
    }
}

void wifi_connection()
{
    ESP_ERROR_CHECK(esp_netif_init());//inicialização do tcp/ip

    ESP_ERROR_CHECK(esp_event_loop_create_default());//cria o grupo de eventos ... sinalizador de eventos. É ele quem gerencia problemas na criação
    esp_netif_create_default_wifi_sta(); //utilizar a stack tcpip para utilizar no modo station
    //esp_netif_create_default_wifi_ap();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,//qqr evento
                                                        &wifi_event_handler,//funcao de callback
                                                        NULL,
                                                        &instance_any_id));//handle a ser intanciado para ser usado posteriormente
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));
    wifi_config_t wifi_config = 
    {
        .sta = 
        {
            .ssid = SSID,
            .password = PASSWORD,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	esp_wifi_set_storage(WIFI_STORAGE_RAM);
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "SSID:%s  password:%s",SSID,PASSWORD);
    
}

void dth_task(void *args){

    int16_t temp = 0;
    int16_t hum = 0;
    while(1){
        if(dht_read_data(DHT_TYPE_AM2301, DHT_PIN, &hum, &temp) == ESP_OK){
            ESP_LOGI(TAG, "temperatura %d C| umidade %d %%", temp/10, hum/10);
        }else{
            ESP_LOGI(TAG, "Não foi possivel fazer leitura");
        }
        vTaskDelay(pdMS_TO_TICKS(2000));
    }
}

void app_main(void)
{
    nvs_flash_init();
    //wifi_connection();
    xTaskCreate(dth_task, "tarefa do dth", 1023*4, NULL, 2, NULL);
}