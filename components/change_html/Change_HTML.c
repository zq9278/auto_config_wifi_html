//
// Created by zq on 2023/12/26.
//
#include <stdlib.h>
#include <string.h>
#include "Change_HTML.h"



/**
 * 将给定的字符串插入到HTML内容的指定占位符位置。
 *
 * @param html_content 原始HTML内容。
 * @param placeholder 占位符文本。
 * @param insert_content 要插入的新内容。
 * @return 指向新HTML内容的指针。使用完毕后应该用free()释放。
 */
char *insert_into_html(const char *html_content, const char *placeholder, const char *insert_content) {
    // 查找占位符
    char *placeholder_pos = strstr(html_content, placeholder);
    if (!placeholder_pos) {
        return NULL; // 占位符未找到
    }

    // 计算新HTML的长度并分配内存
    size_t new_html_length = strlen(html_content) - strlen(placeholder) + strlen(insert_content) + 1;
    char *new_html = (char *)malloc(new_html_length);
    if (!new_html) {
        return NULL; // 内存分配失败
    }

    // 拷贝占位符之前的部分
    size_t pre_placeholder_length = placeholder_pos - html_content;
    memcpy(new_html, html_content, pre_placeholder_length);

    // 插入新内容
    strcpy(new_html + pre_placeholder_length, insert_content);

    // 拷贝占位符之后的部分
    strcpy(new_html + pre_placeholder_length + strlen(insert_content), placeholder_pos + strlen(placeholder));

    return new_html;
}
