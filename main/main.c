#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_mac.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"
#include "esp_netif.h"

#define SSID "ESP_WiFi" //SSID
#define Password "12345678" //Password，不填寫將設為開放網路
#define ap_channel 9 //Channel，設為0將自動設定
#define max_con 10 //最大連線數
#define ap_ip "192.168.100.1" //IP
#define ap_netmask "255.255.255.0" //Netmask

static const char *TAG = "[ESP32 AP]";

/***** ap_event_handler *****/
static void ap_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data)
{
    if (event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
        ESP_LOGI(TAG, "裝置 "MACSTR" 已連上AP, AID=%d",
                 MAC2STR(event->mac), event->aid);
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
        ESP_LOGI(TAG, "裝置 "MACSTR" 已從AP中斷連線, AID=%d",
                 MAC2STR(event->mac), event->aid);
    }
}

/***** AP設定、初始化 *****/
void wifi_ap(void)
{
    // const char *ip = "192.168.100.1";

    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    // esp_netif_create_default_wifi_ap();
    esp_netif_t *ap_netif = esp_netif_create_default_wifi_ap(); // 取得 AP 的 netif

    ESP_ERROR_CHECK(esp_netif_dhcps_stop(ap_netif)); // 停止 DHCP

    // 設定 IP
    esp_netif_ip_info_t ip_info;
    ip_info.ip.addr = esp_ip4addr_aton(ap_ip); // ip
    ip_info.gw.addr = esp_ip4addr_aton(ap_ip); // gw
    ip_info.netmask.addr = esp_ip4addr_aton(ap_netmask); // netmask
    ESP_ERROR_CHECK(esp_netif_set_ip_info(ap_netif, &ip_info));

    ESP_ERROR_CHECK(esp_netif_dhcps_start(ap_netif)); // 啟動 DHCP

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &ap_event_handler, NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = SSID,
            .ssid_len = strlen(SSID),
            .password = Password,
            .max_connection = max_con,
            .channel = ap_channel,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
        },
    };

    // char log_pwd = wifi_config.ap.password;
    if (strlen(Password) == 0) {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
        // log_pwd = "開放網路"; // 開放網路
    }

    // if (ap_channel != 0) {
    //     wifi_config.ap.channel = ap_channel; // 使用指定頻道
    // }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_netif_ip_info_t current_ip_info; // 取得 IP
    ESP_ERROR_CHECK(esp_netif_get_ip_info(ap_netif, &current_ip_info));
    ESP_LOGI(TAG, "AP已啟動！");
    ESP_LOGI(TAG, "SSID: %s, Password: %s", wifi_config.ap.ssid, wifi_config.ap.password);
    // ESP_LOGI(TAG, "SSID: %s, Password: %s", wifi_config.ap.ssid, log_pwd);
    ESP_LOGI(TAG, "IP: " IPSTR, IP2STR(&current_ip_info.ip));
    ESP_LOGI(TAG, "Channel: %d", wifi_config.ap.channel);
    ESP_LOGI(TAG, "最大連線數: %d", wifi_config.ap.max_connection);
}

/***** 主程式 *****/
void app_main(void)
{
    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    ESP_LOGI(TAG, "AP模式設定中...");
    wifi_ap();
}