/* Captive Portal Example

    This example code is in the Public Domain (or CC0 licensed, at your option.)

    Unless required by applicable law or agreed to in writing, this
    software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
    CONDITIONS OF ANY KIND, either express or implied.
*/

#include <sys/param.h>
#include <inttypes.h>

#include "esp_log.h"
#include "esp_system.h"
#include "esp_netif.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#define DNS_PORT (53)
#define DNS_MAX_LEN (256)

#define OPCODE_MASK (0x7800)
#define QR_FLAG (1 << 7)
#define QD_TYPE_A (0x0001)
#define ANS_TTL_SEC (300)

static const char *TAG = "example_dns_redirect_server";

// DNS Header Packet
typedef struct __attribute__((__packed__))
{
    uint16_t id;
    uint16_t flags;
    uint16_t qd_count;
    uint16_t an_count;
    uint16_t ns_count;
    uint16_t ar_count;
} dns_header_t;

// DNS Question Packet
typedef struct
{
    uint16_t type;
    uint16_t class;
} dns_question_t;

// DNS Answer Packet
typedef struct __attribute__((__packed__))
{
    uint16_t ptr_offset;
    uint16_t type;
    uint16_t class;
    uint32_t ttl;
    uint16_t addr_len;
    uint32_t ip_addr;
} dns_answer_t;

/*
    Parse the name from the packet from the DNS name format to a regular .-seperated name
    returns the pointer to the next part of the packet
*/
static char *parse_dns_name(char *raw_name, char *parsed_name, size_t parsed_name_max_len)
{
    // 定义原始名称的指针
    char *label = raw_name;
    // 定义解析后名称的迭代器
    char *name_itr = parsed_name;
    // 初始化解析后名称的长度
    int name_len = 0;

    do
    {
        // 获取子名称的长度
        int sub_name_len = *label;
        // 更新总长度（包括一个点号）
        name_len += (sub_name_len + 1);
        // 如果超过最大长度，则返回NULL
        if (name_len > parsed_name_max_len)
        {
            return NULL;
        }

        // 将子名称复制到解析后的名称中
        memcpy(name_itr, label + 1, sub_name_len);
        // 在子名称后添加点号
        name_itr[sub_name_len] = '.';
        // 更新迭代器位置
        name_itr += (sub_name_len + 1);
        // 更新标签位置
        label += sub_name_len + 1;
    } while (*label != 0); // 继续循环直到标签结束

    // 将解析后名称的最后一个点号替换为字符串结束符
    parsed_name[name_len - 1] = '\0';
    // 返回解析后的名称之后的第一个字符的指针
    return label + 1;
}

// 解析DNS请求并准备带有softAP IP地址的DNS响应
static int parse_dns_request(char *req, size_t req_len, char *dns_reply, size_t dns_reply_max_len)
{
    // 检查请求长度是否超过最大回复长度
    if (req_len > dns_reply_max_len)
    {
        return -1;
    }

    // 准备回复，清零并复制请求内容
    memset(dns_reply, 0, dns_reply_max_len);
    memcpy(dns_reply, req, req_len);

    // 转换网络数据包的字节序与芯片的字节序
    dns_header_t *header = (dns_header_t *)dns_reply;
    ESP_LOGD(TAG, "DNS query with header id: 0x%X, flags: 0x%X, qd_count: %d",
             ntohs(header->id), ntohs(header->flags), ntohs(header->qd_count));

    // 如果不是标准查询，则返回0
    if ((header->flags & OPCODE_MASK) != 0)
    {
        return 0;
    }

    // 设置问题响应标志
    header->flags |= QR_FLAG;

    // 获取问题计数并更新回答计数
    uint16_t qd_count = ntohs(header->qd_count);
    header->an_count = htons(qd_count);

    // 计算回复长度
    int reply_len = qd_count * sizeof(dns_answer_t) + req_len;
    // 如果回复长度超过最大长度，则返回-1
    if (reply_len > dns_reply_max_len)
    {
        return -1;
    }

    // 设置当前答案和问题的指针
    char *cur_ans_ptr = dns_reply + req_len;
    char *cur_qd_ptr = dns_reply + sizeof(dns_header_t);
    char name[128];

    // 针对每个问题用ESP32的IP地址回答
    for (int i = 0; i < qd_count; i++)
    {
        // 解析问题中的名称
        char *name_end_ptr = parse_dns_name(cur_qd_ptr, name, sizeof(name));
        // 如果解析失败，则返回-1
        if (name_end_ptr == NULL)
        {
            ESP_LOGE(TAG, "Failed to parse DNS question: %s", cur_qd_ptr);
            return -1;
        }

        // 获取问题类型和类
        dns_question_t *question = (dns_question_t *)(name_end_ptr);
        uint16_t qd_type = ntohs(question->type);
        uint16_t qd_class = ntohs(question->class);

        ESP_LOGD(TAG, "Received type: %d | Class: %d | Question for: %s", qd_type, qd_class, name);

        // 如果问题类型是A（IPv4地址），则准备回答
        if (qd_type == QD_TYPE_A)
        {
            dns_answer_t *answer = (dns_answer_t *)cur_ans_ptr;

            // 设置回答字段
            answer->ptr_offset = htons(0xC000 | (cur_qd_ptr - dns_reply));
            answer->type = htons(qd_type);
            answer->class = htons(qd_class);
            answer->ttl = htonl(ANS_TTL_SEC);

            // 获取ESP32的IP信息
            esp_netif_ip_info_t ip_info;
            esp_netif_get_ip_info(esp_netif_get_handle_from_ifkey("WIFI_AP_DEF"), &ip_info);
            ESP_LOGD(TAG, "Answer with PTR offset: 0x%" PRIX16 " and IP 0x%" PRIX32, ntohs(answer->ptr_offset), ip_info.ip.addr);

            // 设置回答的地址长度和IP地址
            answer->addr_len = htons(sizeof(ip_info.ip.addr));
            answer->ip_addr = ip_info.ip.addr;
        }
    }
    // 返回回复长度
    return reply_len;
}

/*
    设置套接字并监听DNS查询，
    对所有类型A的查询回复softAP的IP地址
*/
void dns_server_task(void *pvParameters)
{
    // 接收缓冲区
    char rx_buffer[128];
    // 地址字符串
    char addr_str[128];
    // 地址族
    int addr_family;
    // IP协议
    int ip_protocol;

    // 循环监听
    while (1)
    {

        // 设置目的地址为任意地址
        // 定义一个sockaddr_in结构体变量dest_addr，用于存储目的地址信息
        struct sockaddr_in dest_addr;
        // 将目的地址设置为任意地址。htonl函数确保地址格式符合网络字节顺序
        dest_addr.sin_addr.s_addr = htonl(INADDR_ANY);
        // 设置地址族为AF_INET，表示使用IPv4地址
        dest_addr.sin_family = AF_INET;
        // 将目的端口号设置为DNS_PORT。htons函数确保端口号格式符合网络字节顺序
        dest_addr.sin_port = htons(DNS_PORT);
        // 将addr_family变量设置为AF_INET，表示地址族为IPv4
        addr_family = AF_INET;
        // 将ip_protocol变量设置为IPPROTO_IP，表示使用IP协议
        ip_protocol = IPPROTO_IP;
        // 将IP地址转换为字符串形式
        inet_ntoa_r(dest_addr.sin_addr, addr_str, sizeof(addr_str) - 1);

        // 创建套接字
        int sock = socket(addr_family, SOCK_DGRAM, ip_protocol);
        // 如果创建失败，则退出循环
        if (sock < 0)
        {
            ESP_LOGE(TAG, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG, "Socket created");

        // 绑定套接字
        int err = bind(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        // 如果绑定失败，则退出循环
        if (err < 0)
        {
            ESP_LOGE(TAG, "Socket unable to bind: errno %d", errno);
        }
        ESP_LOGI(TAG, "Socket bound, port %d", DNS_PORT);

        // 循环接收数据
        while (1)
        {
            ESP_LOGI(TAG, "Waiting for data");
            // 创建源地址结构体，足够容纳IPv4和IPv6
            struct sockaddr_in6 source_addr;
            socklen_t socklen = sizeof(source_addr);
            // 从套接字接收数据
            int len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0, (struct sockaddr *)&source_addr, &socklen);
            // 接收失败时的处理
            if (len < 0)
            {
                ESP_LOGE(TAG, "recvfrom failed: errno %d", errno);
                close(sock);
                break;
            }
            // 接收到数据的处理
            else
            {
                // 将发送者的IP地址转换为字符串
                if (source_addr.sin6_family == PF_INET)
                {
                    inet_ntoa_r(((struct sockaddr_in *)&source_addr)->sin_addr.s_addr, addr_str, sizeof(addr_str) - 1);
                }
                else if (source_addr.sin6_family == PF_INET6)
                {
                    inet6_ntoa_r(source_addr.sin6_addr, addr_str, sizeof(addr_str) - 1);
                }

                // 将接收到的数据视为字符串处理
                rx_buffer[len] = 0;

                // 准备DNS回复
                char reply[DNS_MAX_LEN];
                int reply_len = parse_dns_request(rx_buffer, len, reply, DNS_MAX_LEN);

                ESP_LOGI(TAG, "Received %d bytes from %s | DNS reply with len: %d", len, addr_str, reply_len);
                // 准备回复失败时的处理
                if (reply_len <= 0)
                {
                    ESP_LOGE(TAG, "Failed to prepare a DNS reply");
                }
                else
                {
                    // 将回复发送给请求者
                    int err = sendto(sock, reply, reply_len, 0, (struct sockaddr *)&source_addr, sizeof(source_addr));
                    // 发送失败时的处理
                    if (err < 0)
                    {
                        ESP_LOGE(TAG, "Error occurred during sending: errno %d", errno);
                        break;
                    }
                }
            }
        }

        // 如果套接字有效，则关闭套接字
        if (sock != -1)
        {
            ESP_LOGE(TAG, "Shutting down socket");
            shutdown(sock, 0);
            close(sock);
        }
    }
    // 删除任务
    vTaskDelete(NULL);
}

void start_dns_server(void)
{
    // 创建并启动DNS服务器任务
    xTaskCreate(dns_server_task, "dns_server", 4096, NULL, 5, NULL);
}
