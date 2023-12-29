//
// Created by zq on 2023/12/26.
//
#include <stdlib.h>
#include <string.h>
#include "Change_HTML.h"



/**
 * ���������ַ������뵽HTML���ݵ�ָ��ռλ��λ�á�
 *
 * @param html_content ԭʼHTML���ݡ�
 * @param placeholder ռλ���ı���
 * @param insert_content Ҫ����������ݡ�
 * @return ָ����HTML���ݵ�ָ�롣ʹ����Ϻ�Ӧ����free()�ͷš�
 */
char *insert_into_html(const char *html_content, const char *placeholder, const char *insert_content) {
    // ����ռλ��
    char *placeholder_pos = strstr(html_content, placeholder);
    if (!placeholder_pos) {
        return NULL; // ռλ��δ�ҵ�
    }

    // ������HTML�ĳ��Ȳ������ڴ�
    size_t new_html_length = strlen(html_content) - strlen(placeholder) + strlen(insert_content) + 1;
    char *new_html = (char *)malloc(new_html_length);
    if (!new_html) {
        return NULL; // �ڴ����ʧ��
    }

    // ����ռλ��֮ǰ�Ĳ���
    size_t pre_placeholder_length = placeholder_pos - html_content;
    memcpy(new_html, html_content, pre_placeholder_length);

    // ����������
    strcpy(new_html + pre_placeholder_length, insert_content);

    // ����ռλ��֮��Ĳ���
    strcpy(new_html + pre_placeholder_length + strlen(insert_content), placeholder_pos + strlen(placeholder));

    return new_html;
}
