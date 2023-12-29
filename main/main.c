#include <string.h>

#include "freertos/FreeRTOS.h"       // 引入FreeRTOS API
#include "freertos/task.h"           // 引入任务API
#include "freertos/event_groups.h"   // 引入事件组API

#include "esp_wifi.h"                // 引入ESP32 WiFi API
#include "esp_log.h"                 // 引入日志API
#include "esp_netif.h"               // 引入网络接口API
#include "esp_event.h"               // 引入事件循环API
#include "nvs_flash.h"               // 引入非易失性存储API

#include <netdb.h>                   // 引入网络数据库操作API
#include <sys/socket.h>              // 引入套接字API


#include "dns_server.h"           // 引入自定义DNS服务器
#include "webserver.h"               // 引入Web服务器
#include "ConnectWIFI.h"
#include "driver/uart.h"             // 引入UART驱动API

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;   // FreeRTOS事件组，用于WiFi连接管理
const int CONNECTED_BIT = BIT0;               // 连接位标志

static const char *TAG = "Main";              // 日志标签

// WiFi事件处理函数
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "WiFi AP Started");      // WiFi AP启动
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "A device connected to the AP"); // 有设备连接到AP
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "A device disconnected from the AP"); // 有设备断开与AP的连接
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );  // 初始化非易失性存储（NVS），用于存储和检索配置
    //NvsWriteDataToFlash("","","");       // （被注释掉的）向NVS写入数据的函数调用

    char WIFI_Name[50] = { 0 };          // 用于存储Wi-Fi名称的字符数组
    char WIFI_PassWord[50] = { 0 };      // 用于存储Wi-Fi密码的字符数组

    /*读取保存的WIFI信息*/
    if(NvsReadDataFromFlash("WIFI Config Is OK!", WIFI_Name, WIFI_PassWord) == 0x00)
    {
        printf("WIFI SSID     :%s\r\n", WIFI_Name);        // 打印Wi-Fi名称
        printf("WIFI PASSWORD :%s\r\n", WIFI_PassWord);   // 打印Wi-Fi密码
        printf("开始初始化WIFI Station 模式\r\n");          // 提示开始初始化Wi-Fi Station模式
        wifi_init_sta(WIFI_Name, WIFI_PassWord);          // 按照读取的信息初始化Wi-Fi Station模式
    }
    else
    {
        printf("未读取到WIFI配置信息\r\n");                 // 打印未读取到Wi-Fi配置信息的提示
        printf("开始初始化WIFI AP 模式\r\n");               // 提示开始初始化Wi-Fi AP模式

        WIFI_AP_Init();      // 上电后配置Wi-Fi为AP模式
        //vTaskDelay(1000 / portTICK_PERIOD_MS);           // （被注释掉的）延迟函数调用
        
        //dns_server_start();  // 开启DNS服务
        web_server_start();  // 开启HTTP服务
        start_dns_server();
        
    }
}
