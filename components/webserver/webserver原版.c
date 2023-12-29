#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
//webserver.c
#include "lwip/dns.h"

#include "webserver.h"
#include "Change_HTML.h"



#define  BUFF_SIZE 1024
extern const uint8_t index_html_start[] asm("_binary_index_html_start"); // 声明一个外部变量，这个变量是HTML文件的开始位置
extern const uint8_t index_html_end[]   asm("_binary_index_html_end"); // 声明一个外部变量，这个变量是HTML文件的结束位置

static const char *TAG = "WEB_SERVER"; // 定义一个标签，用于日志输出

// 定义HTTP 200响应的头部信息
static const char *HTTP_200 = "HTTP/1.1 200 OK\r\n"
                              "Server: lwIP/1.4.0\r\n"
                              "Content-Type: text/html\r\n"
                              "Connection: Keep-Alive\r\n"
                              "Content-Length: %d \r\n\r\n";

// 定义HTTP 400响应的头部信息
static const char *HTTP_400 = "HTTP/1.0 400 BadRequest\r\n"
                              "Content-Length: 0\r\n"
                              "Connection: Close\r\n"
                              "Server: lwIP/1.4.0\r\n\n";

// 自定义的写函数，用于向socket写入数据
int my_write(int fd, void *buffer, int length) {
    int bytes_left; // 剩余未写入的字节数
    int written_bytes; // 已写入的字节数
    char *ptr; // 指向待写入数据的指针

    ptr = buffer; // 初始化指针
    bytes_left = length; // 初始化剩余字节数
    while (bytes_left > 0) // 当还有未写入的数据时，继续写入
    {
        written_bytes = send(fd, ptr, bytes_left, 0); // 尝试写入数据
        if (written_bytes <= 0) // 如果写入失败
        {
            if (errno == EINTR) // 如果是因为被中断导致的失败，那么不改变已写入的字节数，继续尝试写入
                written_bytes = 0;
            else // 如果是因为其他原因导致的失败，那么返回错误
                return (-1);
        }
        bytes_left -= written_bytes; // 更新剩余未写入的字节数
        ptr += written_bytes; // 更新指针位置
        vTaskDelay(10); // 延迟一段时间，避免CPU占用过高
    }
    return (0); // 所有数据都已成功写入，返回成功
}

// 处理HTTP请求的函数
void handle_http_request(void *pvParameters) {
    char buff[BUFF_SIZE] = {0};  // 数据缓冲器
    int length = 0; // 用于保存生成的HTTP响应头的长度

    int fd = *(int *) pvParameters; // 从参数中获取socket文件描述符
    int bytes_recvd = 0; // 用于保存接收到的字节数
    char *uri = NULL; // 用于保存解析出的URI

    ESP_LOGI(TAG, "Http Sub Task Run with socket: %d", fd); // 输出日志，显示任务开始运行

    vTaskDelay(30); // 延迟一段时间，避免CPU占用过高

    // 读取HTTP请求头
    bytes_recvd = recv(fd, buff, BUFF_SIZE - 1, 0); // 尝试接收数据

    if (bytes_recvd <= 0)  // 如果接收失败
    {
        ESP_LOGE(TAG, "Recv requst header error!"); // 输出错误日志
        goto requst_error; // 跳转到错误处理代码
    }

    // 解析请求类型及请求URI
    uri = strstr(buff, "HTTP"); // 在请求头中查找"HTTP"
    if (uri == NULL) // 如果没有找到，说明请求头格式有误
    {
        ESP_LOGE(TAG, "Parase requst header error!"); // 输出错误日志
        goto requst_error; // 跳转到错误处理代码
    }
    uri[0] = 0;
    uri = NULL; // 将找到的位置设为字符串结束符，然后清空uri指针

    uri = strstr(buff, " "); // 在请求头中查找第一个空格，空格前面的是请求类型，空格后面的是请求的URI
    if (uri == NULL) // 如果没有找到，说明请求头格式有误
    {
        ESP_LOGE(TAG, "Parase requst uri error!"); // 输出错误日志
        goto requst_error; // 跳转到错误处理代码
    }
    uri[0] = 0;
    uri++; // 将找到的位置设为字符串结束符，然后将uri指针指向URI的开始位置

    ESP_LOGI(TAG, "the reqqust type is %s, uri is: %s", buff, uri); // 输出日志，显示解析出的请求类型和URI

    if (strcmp(buff, "GET") == 0) // 如果是GET请求
    {
        length = sprintf(buff, HTTP_200, index_html_end - index_html_start); // 生成HTTP 200响应头，同时计算出响应头的长度
        //my_write(fd, buff, length); // 将响应头写入socket
        //my_write(fd, index_html_start, index_html_end - index_html_start); // 将HTML文件的内容写入socket

        int original_length = index_html_end - index_html_start;
        char *html_content = malloc(original_length + 1); // 分配足够的内存
        memcpy(html_content, index_html_start, original_length); // 复制内容
        html_content[original_length] = '\0'; // 确保字符串以null结束

        char wifi_list[512]; // 假设这个数组足够大
// 填充wifi_list数组，例如：


        wifi_scan_config_t scan_config = {
                .ssid = 0,
                .bssid = 0,
                .channel = 0,
                .show_hidden = false
        };
        ESP_ERROR_CHECK(esp_wifi_scan_start(&scan_config, true));

        // 获取扫描结果
        uint16_t ap_count = 0;
        esp_wifi_scan_get_ap_num(&ap_count);
        wifi_ap_record_t ap_records[10];
        ap_count = ap_count > 10 ? 10 : ap_count;
        ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_records));
        // 生成HTML列表
        char temp_buf[128];
        strcpy(wifi_list, "<ul>");
        for (int i = 0; i < ap_count; i++) {
            snprintf(temp_buf, sizeof(temp_buf), "<li>%s (%d)</li>", ap_records[i].ssid, ap_records[i].rssi);
            strcat(wifi_list, temp_buf);
        }
        strcat(wifi_list, "</ul>");


//        snprintf(wifi_list, sizeof(wifi_list),
//                 "<ul><li>WiFi 1</li><li>WiFi 2</li></ul>");

        char *placeholder = strstr(html_content, "<!-- WIFI_LIST -->");
        if (placeholder) {
            // 创建新的HTML字符串，包含wifi_list
            char *new_html = malloc(original_length + strlen(wifi_list) + 1);
            // 复制placeholder之前的部分
            memcpy(new_html, html_content, placeholder - html_content);
            new_html[placeholder - html_content] = '\0';
            // 连接wifi_list和placeholder之后的部分
            strcat(new_html, wifi_list);
            strcat(new_html, placeholder + strlen("<!-- WIFI_LIST -->"));

            // 使用 new_html...
            printf("%s",new_html);
            my_write(fd, buff, length); // 将响应头写入socket
            my_write(fd, new_html, strlen(new_html));


            free(new_html); // 不要忘记释放内存
        }
        free(html_content); // 释放原始HTML内容的内存




    } else if (strcmp(buff, "POST") == 0) {

    }else // 如果是其他类型的请求
    {
        my_write(fd, HTTP_400, strlen(HTTP_400)); // 将HTTP 400响应头写入socket
    }

    vTaskDelay(30); // 延迟一段时间，避免CPU占用过高

    requst_error: // 错误处理代码
    ESP_LOGI(TAG, "close socket %d", fd); // 输出日志，显示即将关闭的socket
    close(fd); // 关闭socket
    vTaskDelete(NULL); // 删除当前任务
}


// web服务器的主函数
void webserver(void *pvParameters) {
    int sockfd, new_fd; // socket句柄和建立连接后的句柄
    struct sockaddr_in my_addr; // 本方地址信息结构体
    struct sockaddr_in their_addr; // 对方地址信息
    socklen_t sin_size;

    struct timeval tv; // 发送接收超时时间
    tv.tv_sec = 10;
    tv.tv_usec = 0;

    sin_size = sizeof(struct sockaddr_in);
    sockfd = socket(AF_INET, SOCK_STREAM, 0); // 建立socket用于监听指定端口上的入站连接请求。是TCP服务器的第一步。
    /*AF_INET: 地址族（Address Family）。AF_INET 表示使用IPv4网络协议。指定套接字可以通信的网络类型。
SOCK_STREAM: 套接字类型。SOCK_STREAM 表示套接字是面向连接的流式套接字。它用于提供顺序化、可靠、双向、基于连接的字节流。与 SOCK_DGRAM（数据报套接字，用于UDP协议）相对。
0: 协议类型。表示选择默认协议，默认协议是TCP*/
    if (sockfd == -1) // 如果建立失败
    {
        ESP_LOGE(TAG, "socket failed:%d", errno); // 输出错误日志
        goto web_err; // 跳转到错误处理代码
    }
    my_addr.sin_family = AF_INET; // 该属性表示接收本机或其他机器传输
    my_addr.sin_port = htons(80); // 端口号
    my_addr.sin_addr.s_addr = htonl(INADDR_ANY); // IP，括号内容表示本机IP
    bzero(&(my_addr.sin_zero), 8); // 将其他属性置0

    if (bind(sockfd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr)) < 0) // 绑定地址结构体和socket
        /*将socket与特定的IP地址和端口号关联。对于服务器程序来说，这通常意味着将套接字绑定到一个公共地址和端口上，以便客户端可以找到并连接到它。*/
    {
        ESP_LOGE(TAG, "bind error"); // 输出错误日志
        goto web_err; // 跳转到错误处理代码
    }

    listen(sockfd, 8); // 开启监听 ，第二个参数是最大监听数
    ESP_LOGI(TAG, "webserver start..."); // 输出日志，显示服务器开始运行
    while (1) // 服务器主循环
    {
        new_fd = accept(sockfd, (struct sockaddr *) &their_addr, &sin_size); // 在这里阻塞直到接收到消息，参数分别是socket句柄，接收到的地址信息以及大小
        if (new_fd == -1) // 如果接收失败
        {
            ESP_LOGI(TAG, "accept failed"); // 输出错误日志
        } else // 如果接收成功
        {
            ESP_LOGI(TAG, "Accept new socket: %d", new_fd); // 输出日志，显示接收到的新socket

            setsockopt(new_fd, SOL_SOCKET, SO_RCVTIMEO, (const char *) &tv, sizeof(tv)); // 设置接收超时时间
            setsockopt(new_fd, SOL_SOCKET, SO_SNDTIMEO, (const char *) &tv, sizeof(tv)); // 设置发送超时时间

            int *para_fd = malloc(sizeof(int)); // 分配内存，用于保存socket文件描述符
            *para_fd = new_fd; // 将socket文件描述符保存到分配的内存中
            xTaskCreate(&handle_http_request, "socket_task", 1024 * 6, para_fd, 6, NULL); // 创建一个新任务，用于处理HTTP
        }
        vTaskDelay(10);
    }

    web_err:
    vTaskDelete(NULL);
}

void web_server_start(void) {
    xTaskCreate(&webserver, "webserver_task", 2048, NULL, 5, NULL);
}
