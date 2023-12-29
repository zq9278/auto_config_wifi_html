#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "lwip/dns.h"

#include "esp_http_server.h"

#include "webserver.h"
#include "ConnectWIFI.h"

// 引入嵌入式资源
extern const uint8_t index_html_start[] asm("_binary_index_html_start");  // HTML起始指针
extern const uint8_t index_html_end[]   asm("_binary_index_html_end");    // HTML结束指针

static const char *TAG = "WEB_SERVER";  // 日志标签

#define MIN( x, y ) ( ( x ) < ( y ) ? ( x ) : ( y ) )  // 定义一个求最小值的宏


static esp_err_t http_SendText_html(httpd_req_t *req)
{
    const size_t upload_script_size = (index_html_end - index_html_start);  // 计算HTML文件的大小
    const char TxBuffer[] = "<h1> SSID1 other WIFI</h1>";  // 定义要发送的HTML内容
    httpd_resp_send_chunk(req, (const char *)index_html_start, upload_script_size);  // 发送HTML文件的内容
    httpd_resp_send_chunk(req, (const char *)TxBuffer, sizeof(TxBuffer));  // 发送额外的HTML内容

    return ESP_OK;  // 返回操作成功
}

// 将字符转换为数字
unsigned char CharToNum(unsigned char Data)
{
    if(Data >= '0' && Data <= '9')
    {
        return Data - '0';  // 如果是数字字符，则直接转换为对应的数字
    }
    else if(Data >= 'a' && Data <= 'f')
    {
        switch (Data)  // 如果是小写的十六进制字符，则转换为对应的数字
        {
            case 'a':return 10;
            case 'b':return 11;
            case 'c':return 12;
            case 'd':return 13;
            case 'e':return 14;
            case 'f':return 15;
            default:
                break;
        }
    }
    else if(Data >= 'A' && Data <= 'F')
    {
        switch (Data)  // 如果是大写的十六进制字符，则转换为对应的数字
        {
            case 'A':return 10;
            case 'B':return 11;
            case 'C':return 12;
            case 'D':return 13;
            case 'E':return 14;
            case 'F':return 15;
            default:
                break;
        }
    }
    return 0;  // 如果不是上述字符，则返回0
}

// 处理门户页面第一次GET请求
static esp_err_t HTTP_FirstGet_handler(httpd_req_t *req)
{
    http_SendText_html(req);  // 调用函数发送HTML内容

    return ESP_OK;  // 返回操作成功
}


// 处理门户页面POST请求，获取Wi-Fi名和密码
static esp_err_t WIFI_Config_POST_handler(httpd_req_t *req)
{
    char buf[100];  // 定义缓冲区，用于存储接收到的数据
    int ret, remaining = req->content_len;  // 定义剩余的请求内容长度

    while (remaining > 0) {  // 当剩余的请求内容长度大于0时，继续循环
        // 读取请求数据
        if ((ret = httpd_req_recv(req, buf, MIN(remaining, sizeof(buf)))) <= 0) {
            if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
                continue;  // 如果接收数据超时，则继续下一次循环
            }
            return ESP_FAIL;  // 如果接收数据失败，则返回失败
        }
        // 发送接收到的数据
        char WIFI_ConfigBackInformation[100] = "The WIFI To Connect :";
        strcat(WIFI_ConfigBackInformation, buf);  // 将接收到的数据拼接到响应字符串后面
        httpd_resp_send_chunk(req, WIFI_ConfigBackInformation, sizeof(WIFI_ConfigBackInformation));  // 发送响应
        remaining -= ret;  // 更新剩余的请求内容长度

        // 处理Wi-Fi名称和密码
        char wifi_name[50];
        char wifi_password[50];
        char wifi_passwordTransformation[50] = {0};
        esp_err_t e = httpd_query_key_value(buf, "ssid", wifi_name, sizeof(wifi_name));  // 从请求数据中解析出Wi-Fi名称
        if (e == ESP_OK) {
            printf("SSID = %s\r\n", wifi_name);  // 打印Wi-Fi名称
        } else {
            printf("error = %d\r\n", e);  // 打印错误信息
        }

        e = httpd_query_key_value(buf, "passWord", wifi_password, sizeof(wifi_password));  // 从请求数据中解析出Wi-Fi密码
        if (e == ESP_OK) {
            // 处理密码转换
            unsigned char Len = strlen(wifi_password);
            char tempBuffer[2];
            char *temp;
            unsigned char Cnt = 0;
            temp = wifi_password;
            for (int i = 0; i < Len;) {
                if (*temp == '%') {
                    tempBuffer[0] = CharToNum(temp[1]);
                    tempBuffer[1] = CharToNum(temp[2]);
                    *temp = tempBuffer[0] * 16 + tempBuffer[1];
                    wifi_passwordTransformation[Cnt] = *temp;
                    temp += 3;
                    i += 3;
                    Cnt++;
                } else {
                    wifi_passwordTransformation[Cnt] = *temp;
                    temp++;
                    i++;
                    Cnt++;
                }
            }
            temp -= Len;
            printf("Len = %d\r\n", Len);
            printf("wifi_password = %s\r\n", wifi_password);
            printf("pswd = %s\r\n", wifi_passwordTransformation);
        } else {
            printf("error = %d\r\n", e);  // 打印错误信息
        }



        // 记录接收到的数据
        ESP_LOGI(TAG, "=========== RECEIVED DATA ==========");
        ESP_LOGI(TAG, "%.*s", ret, buf);
        ESP_LOGI(TAG, "====================================");

        // 写入Wi-Fi配置到NVS并重启
        NvsWriteDataToFlash("WIFI Config Is OK!", wifi_name, wifi_passwordTransformation);
        esp_restart();  // 重启设备
    }

    return ESP_OK;  // 返回成功
}


// 启动Web服务器
void web_server_start(void)
{
    httpd_handle_t server = NULL;  // 定义一个HTTP服务器句柄
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();  // 获取默认的HTTP服务器配置
    config.uri_match_fn = httpd_uri_match_wildcard;  // 设置URI匹配函数为通配符匹配

    ESP_LOGI(TAG, "Starting HTTP Server on port: '%d'", config.server_port);  // 打印日志，输出正在启动的HTTP服务器的端口号
    if (httpd_start(&server, &config) != ESP_OK) {  // 如果启动HTTP服务器失败
        ESP_LOGE(TAG, "Failed to start file server!");  // 打印日志，输出启动文件服务器失败
        return;  // 返回
    }

    // 注册用于处理GET请求的URI处理器
    httpd_uri_t file_download = {
            .uri       = "/*",  // 匹配所有URI
            .method    = HTTP_GET,  // 请求方法为GET
            .handler   = HTTP_FirstGet_handler,  // 处理函数为HTTP_FirstGet_handler
            .user_ctx  = NULL,  // 用户上下文为NULL
    };
    httpd_register_uri_handler(server, &file_download);  // 在服务器上注册URI处理器

    // 注册用于处理POST请求的URI处理器
    httpd_uri_t file_upload = {
            .uri       = "/configwifi",  // 匹配/configwifi URI
            .method    = HTTP_POST,  // 请求方法为POST
            .handler   = WIFI_Config_POST_handler,  // 处理函数为WIFI_Config_POST_handler
            .user_ctx  = NULL,  // 用户上下文为NULL
    };
    httpd_register_uri_handler(server, &file_upload);  // 在服务器上注册URI处理器
}
