#include <string.h>

#include "freertos/FreeRTOS.h"       // ����FreeRTOS API
#include "freertos/task.h"           // ��������API
#include "freertos/event_groups.h"   // �����¼���API

#include "esp_wifi.h"                // ����ESP32 WiFi API
#include "esp_log.h"                 // ������־API
#include "esp_netif.h"               // ��������ӿ�API
#include "esp_event.h"               // �����¼�ѭ��API
#include "nvs_flash.h"               // �������ʧ�Դ洢API

#include <netdb.h>                   // �����������ݿ����API
#include <sys/socket.h>              // �����׽���API


#include "dns_server.h"           // �����Զ���DNS������
#include "webserver.h"               // ����Web������
#include "ConnectWIFI.h"
#include "driver/uart.h"             // ����UART����API

/* FreeRTOS event group to signal when we are connected & ready to make a request */
static EventGroupHandle_t wifi_event_group;   // FreeRTOS�¼��飬����WiFi���ӹ���
const int CONNECTED_BIT = BIT0;               // ����λ��־

static const char *TAG = "Main";              // ��־��ǩ

// WiFi�¼�������
static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START) {
        ESP_LOGI(TAG, "WiFi AP Started");      // WiFi AP����
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        ESP_LOGI(TAG, "A device connected to the AP"); // ���豸���ӵ�AP
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        ESP_LOGI(TAG, "A device disconnected from the AP"); // ���豸�Ͽ���AP������
    }
}

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );  // ��ʼ������ʧ�Դ洢��NVS�������ڴ洢�ͼ�������
    //NvsWriteDataToFlash("","","");       // ����ע�͵��ģ���NVSд�����ݵĺ�������

    char WIFI_Name[50] = { 0 };          // ���ڴ洢Wi-Fi���Ƶ��ַ�����
    char WIFI_PassWord[50] = { 0 };      // ���ڴ洢Wi-Fi������ַ�����

    /*��ȡ�����WIFI��Ϣ*/
    if(NvsReadDataFromFlash("WIFI Config Is OK!", WIFI_Name, WIFI_PassWord) == 0x00)
    {
        printf("WIFI SSID     :%s\r\n", WIFI_Name);        // ��ӡWi-Fi����
        printf("WIFI PASSWORD :%s\r\n", WIFI_PassWord);   // ��ӡWi-Fi����
        printf("��ʼ��ʼ��WIFI Station ģʽ\r\n");          // ��ʾ��ʼ��ʼ��Wi-Fi Stationģʽ
        wifi_init_sta(WIFI_Name, WIFI_PassWord);          // ���ն�ȡ����Ϣ��ʼ��Wi-Fi Stationģʽ
    }
    else
    {
        printf("δ��ȡ��WIFI������Ϣ\r\n");                 // ��ӡδ��ȡ��Wi-Fi������Ϣ����ʾ
        printf("��ʼ��ʼ��WIFI AP ģʽ\r\n");               // ��ʾ��ʼ��ʼ��Wi-Fi APģʽ

        WIFI_AP_Init();      // �ϵ������Wi-FiΪAPģʽ
        //vTaskDelay(1000 / portTICK_PERIOD_MS);           // ����ע�͵��ģ��ӳٺ�������
        
        //dns_server_start();  // ����DNS����
        web_server_start();  // ����HTTP����
        start_dns_server();
        
    }
}
