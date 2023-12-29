#include <stdio.h>
#include "ConnectWIFI.h"

// #include "D:/esp-code/auto-config-wifi/components/connect_wifi/ConnectWIFI.h"
/* The examples use WiFi configuration that you can set via project configuration menu.
   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID "zhangqi_wifi2"
#define EXAMPLE_ESP_WIFI_PASS "88888888"
#define EXAMPLE_ESP_WIFI_CHANNEL (10)
#define EXAMPLE_MAX_STA_CONN (2)

// #define EXAMPLE_ESP_WIFI_SSID      "espConnect"
// #define EXAMPLE_ESP_WIFI_PASS      "12345678"
#define EXAMPLE_ESP_MAXIMUM_RETRY 10

#define CONFIG_ESP_WIFI_AUTH_OPEN 1

#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

static const char *TAG = "wifi ap";

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    // if (event_id == WIFI_EVENT_AP_STACONNECTED) {
    //     wifi_event_ap_staconnected_t* event = (wifi_event_ap_staconnected_t*) event_data;
    //     ESP_LOGI("ESP32", "station "MACSTR" join, AID=%d",
    //              MAC2STR(event->mac), event->aid);
    // } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
    //     wifi_event_ap_stadisconnected_t* event = (wifi_event_ap_stadisconnected_t*) event_data;
    //     ESP_LOGI("ESP32", "station "MACSTR" leave, AID=%d",
    //              MAC2STR(event->mac), event->aid);
    // }
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_START)
    {
        ESP_LOGI(TAG, "WiFi AP Started"); // WiFi AP����
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED)
    {
        ESP_LOGI(TAG, "A device connected to the AP"); // ���豸���ӵ�AP
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED)
    {
        ESP_LOGI(TAG, "A device disconnected from the AP"); // ���豸�Ͽ���AP������
    }
}

void WIFI_AP_Init(void)
{
    ESP_LOGI("ESP32", "WIFI Start Init");

    ESP_ERROR_CHECK(esp_netif_init());                /*��ʼ���ײ�TCP/IP��ջ*/
    ESP_ERROR_CHECK(esp_event_loop_create_default()); /*����Ĭ���¼�ѭ��*/
    esp_netif_create_default_wifi_ap();               /*����Ĭ��WIFI AP,��������κγ�ʼ������,��API����ֹ*/

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(
        esp_wifi_init(&cfg)); /*��ʼ��WiFiΪWiFi�������������Դ����WiFi���ƽṹ��RX/TX��������WiFi NVS�ṹ�ȡ���WiFi������WiFi����*/

    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        NULL));

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .ssid_len = strlen(EXAMPLE_ESP_WIFI_SSID),
            .channel = EXAMPLE_ESP_WIFI_CHANNEL,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .max_connection = EXAMPLE_MAX_STA_CONN,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK
            //.authmode = WIFI_AUTH_OPEN
        },
    };
    if (strlen(EXAMPLE_ESP_WIFI_PASS) == 0)
    {
        wifi_config.ap.authmode = WIFI_AUTH_OPEN;
    }

    // ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI("ESP32", "wifi_init_softap finished. SSID:%s password:%s channel:%d",
             EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS, EXAMPLE_ESP_WIFI_CHANNEL);
}

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

// static const char *TAG = "wifi station";

static int s_retry_num = 0;

static void event_handler(void *arg, esp_event_base_t event_base,
                          int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num < EXAMPLE_ESP_MAXIMUM_RETRY)
        {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        else
        {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
        ESP_LOGI(TAG, "connect to the AP fail");
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(char *WIFI_Name, char *WIFI_PassWord)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            /* Setting a password implies station will connect to all security modes including WEP/WPA.
             * However these modes are deprecated and not advisable to be used. Incase your Access point
             * doesn't support WPA2, these mode can be enabled by commenting below line */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
        },
    };

    strcpy((char *)&wifi_config.sta.ssid, WIFI_Name);
    strcpy((char *)&wifi_config.sta.password, WIFI_PassWord);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
                                           WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
                                           pdFALSE,
                                           pdFALSE,
                                           portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT)
    {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 WIFI_Name, WIFI_PassWord);
        // xTaskCreate(tcp_client_task, "tcp_client", 1024 * 10, NULL, 5, NULL);/*TCP_client ����TCP*/
    }
    else if (bits & WIFI_FAIL_BIT)
    {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 WIFI_Name, WIFI_PassWord);
        NvsWriteDataToFlash("", "", ""); /*������������������˳����ӣ���������������Ϣ������*/
        esp_restart();
    }
    else
    {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void NvsWriteDataToFlash(char *ConfirmString, char *WIFI_Name, char *WIFI_PassWord)
{
    nvs_handle handle;
    // д��һ���������ݣ�һ���ַ�����WIFI��Ϣ�Լ��汾��Ϣ
    static const char *NVS_CUSTOMER = "customer data";
    static const char *DATA2 = "String";
    static const char *DATA3 = "blob_wifi";
    // static const char *DATA4 = "blob_version";

    // // Ҫд����ַ���
    // char str_for_store[50] = "WIFI Config Is OK!";
    // Ҫд���WIFI��Ϣ
    wifi_config_t wifi_config_to_store;
    //  wifi_config_t wifi_config_to_store = {
    //     .sta = {
    //         .ssid = "store_ssid:hello_kitty",
    //         .password = "store_password:1234567890",
    //     },
    // };
    strcpy((char *)&wifi_config_to_store.sta.ssid, WIFI_Name);
    strcpy((char *)&wifi_config_to_store.sta.password, WIFI_PassWord);
    // // Ҫд��İ汾��
    // uint8_t version_for_store[4] = {0x01, 0x01, 0x01, 0x00};

    printf("set size:%u\r\n", sizeof(wifi_config_to_store));
    ESP_ERROR_CHECK(nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle));
    ESP_ERROR_CHECK(nvs_set_str(handle, DATA2, ConfirmString));
    ESP_ERROR_CHECK(nvs_set_blob(handle, DATA3, &wifi_config_to_store, sizeof(wifi_config_to_store)));
    // ESP_ERROR_CHECK( nvs_set_blob( handle, DATA4, version_for_store, 4) );

    ESP_ERROR_CHECK(nvs_commit(handle));
    nvs_close(handle);
}

unsigned char NvsReadDataFromFlash(char *ConfirmString, char *WIFI_Name, char *WIFI_PassWord)
{
    nvs_handle handle;                                 // NVS���
    static const char *NVS_CUSTOMER = "customer data"; // ����NVS�����ռ�
    static const char *DATA2 = "String";               // ��������ȷ�ϵļ���
    static const char *DATA3 = "blob_wifi";            // ����洢Wi-Fi���õļ���
    // static const char *DATA4 = "blob_version";  // ����ע�͵��ģ�����洢�汾��Ϣ�ļ���

    uint32_t str_length = 50;         // �ַ������ȱ���
    char str_data[50] = {0};          // �洢��ȡ���ַ���
    wifi_config_t wifi_config_stored; // �洢��ȡ��Wi-Fi����
    // uint8_t version[4] = {0};  // ����ע�͵��ģ��洢�汾��Ϣ������
    // uint32_t version_len = 4;  // ����ע�͵��ģ��汾��Ϣ����

    memset(&wifi_config_stored, 0x0, sizeof(wifi_config_stored)); // ����wifi_config_stored�ṹ
    uint32_t wifi_len = sizeof(wifi_config_stored);               // ����wifi_config_stored�ĳ���

    ESP_ERROR_CHECK(nvs_open(NVS_CUSTOMER, NVS_READWRITE, &handle)); // ��NVS�洢

    // ���Զ�ȡDATA2����ֵ
    nvs_get_str(handle, DATA2, str_data, &str_length); // ��ȡ�ַ�������

    // ���Զ�ȡDATA3����ֵ��Wi-Fi���ã�
    nvs_get_blob(handle, DATA3, &wifi_config_stored, &wifi_len); // ��ȡWi-Fi��������

    // ��ӡ��ȡ��Wi-Fi������Ϣ
    printf("[data3]: ssid:%s passwd:%s\r\n", wifi_config_stored.sta.ssid, wifi_config_stored.sta.password);

    // ����ȡ��Wi-Fi������Ϣ���Ƶ�����Ĳ�����
    strcpy(WIFI_Name, (char *)&wifi_config_stored.sta.ssid);         // ����SSID
    strcpy(WIFI_PassWord, (char *)&wifi_config_stored.sta.password); // ��������

    nvs_close(handle); // �ر�NVS�洢

    // �Ƚ�ȷ���ַ������ж��Ƿ��ȡ�ɹ�
    if (strcmp(ConfirmString, str_data) == 0)
    {
        return 0x00; // ���ƥ�䣬����0x00
    }
    else
    {
        return 0xFF; // �����ƥ�䣬����0xFF
    }
}

//static esp_err_t wifiscan(httpd_req_t *req)
//{
    // char wifi_list[512]; // ������������㹻��
    //                      // ���wifi_list���飬���磺

    // wifi_scan_config_t scan_config = {
    //     .ssid = 0,
    //     .bssid = 0,
    //     .channel = 0,
    //     .show_hidden = false};
    // ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

    // // ��ȡɨ����
    // uint16_t ap_count = 0;
    // esp_wifi_scan_get_ap_num(&ap_count);
    // wifi_ap_record_t ap_records[10];
    // ap_count = ap_count > 10 ? 10 : ap_count;
    // ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));
    // // ����HTML�б�
    // char temp_buf[128];
    // strcpy(wifi_list, "<ul>");
    // for (int i = 0; i < ap_count; i++)
    // {
    //     snprintf(temp_buf, sizeof(temp_buf), "<li onclick=\"selectWifi(this)\">%s (%d)</li>", ap_records[i].ssid, ap_records[i].rssi);
    //     //snprintf(temp_buf, sizeof(temp_buf), "<li onclick=\"selectWifi(this)\">%s</li>", ap_records[i].ssid);
    //     strcat(wifi_list, temp_buf);
    // }
    // strcat(wifi_list, "</ul>");
    // httpd_resp_send_chunk(req, (const char *)wifi_list, sizeof(wifi_list));
    // httpd_resp_send_chunk(req, NULL, 0);





    //printf("testaaaaaaaaaaaaaaaaaaaaaaaaaaaa"); // ��ӡWi-Fi����

    //        snprintf(wifi_list, sizeof(wifi_list),
    //                 "<ul><li>WiFi 1</li><li>WiFi 2</li></ul>");

    // char *placeholder = strstr(html_content, "<!-- WIFI_LIST -->");
    // if (placeholder)
    // {
    //     // �����µ�HTML�ַ���������wifi_list
    //     char *new_html = malloc(original_length + strlen(wifi_list) + 1);
    //     // ����placeholder֮ǰ�Ĳ���
    //     memcpy(new_html, html_content, placeholder - html_content);
    //     new_html[placeholder - html_content] = '\0';
    //     // ����wifi_list��placeholder֮��Ĳ���
    //     strcat(new_html, wifi_list);
    //     strcat(new_html, placeholder + strlen("<!-- WIFI_LIST -->"));

    //     // ʹ�� new_html...
    //     printf("%s", new_html);
    //     my_write(fd, buff, length); // ����Ӧͷд��socket
    //     my_write(fd, new_html, strlen(new_html));

    //     free(new_html); // ��Ҫ�����ͷ��ڴ�
    // return ESP_OK;
    // }

