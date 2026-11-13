/*
 * other/client_common.c —— 基础工具（答辩不讲）
 *
 * 三个模块都会用到的小函数：安全地读取 JSON 字段、安全地复制字符串、清空登录身份、释放数据。
 */
#include "../client.h"
#include <limits.h>   // INT_MIN / INT_MAX：读整数前检查范围
#include <string.h>   // strlen / memcpy / memset

/* 功能：云端的数据可能缺字段、类型不对，字符串可能超长。这些小函数把这些意外挡在门外，
 *       让其他代码可以放心地读数据、复制字符串，不会因为一个空指针或越界就崩溃。
 */

// 读字符串字段：例如 lab_str(预约, "status") 得到 "confirmed"
const char *lab_str(const cJSON *j, const char *key) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(j, key);   // 按字段名查找（区分大小写）；j 为 NULL 时也安全返回 NULL
    // 确认找到了、而且确实是字符串，才返回它的内容；否则返回空字符串 ""。
    // 返回 "" 而不是 NULL 的好处：调用方可以直接 strcmp(lab_str(...), "xxx")，不用先判空
    return cJSON_IsString(v) && v->valuestring ? v->valuestring : "";
}

// 读整数字段：例如 lab_int(预约, "device_number") 得到 12
int lab_int(const cJSON *j, const char *key) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(j, key);
    // cJSON 里数字存成 double；先确认是数字、且在 int 能表示的范围内，再取整数值，否则返回 0
    return cJSON_IsNumber(v) && v->valuedouble >= INT_MIN && v->valuedouble <= INT_MAX ? v->valueint : 0;
}

// 安全复制字符串：放得下才复制，放不下就拒绝（返回 0），绝不写出 dst 的边界。
// 代替 strcpy：strcpy 不检查目标大小，是缓冲区溢出漏洞最常见的来源
int lab_copy_text(char *dst, size_t cap, const char *src) {
    if (!src || strlen(src) >= cap)    // 源为空，或长度 ≥ 容量（还要留 1 字节给结尾 \0）
        return 0;
    memcpy(dst, src, strlen(src) + 1); // +1 把结尾的 \0 一起复制过去
    return 1;
}
// 清空登录身份：退出登录、登录失败、账号被停用时调用，把令牌从内存里彻底抹掉
void lab_session_clear(LabSession *s) {
    // 把结构体当成一串字节逐个写 0。volatile 告诉编译器"每次写入都必须真的执行"：
    // 否则编译器可能发现这块内存之后没人再读，把整个清零循环当成无用代码删掉，令牌就残留在内存里
    volatile unsigned char *p = (volatile unsigned char *)s;
    size_t n = sizeof(*s);   // 整个结构体的字节数（约 12.5 KB）
    while (n--)              // 循环 n 次
        *p++ = 0;            // 当前字节写 0，指针后移一位
}

// 释放全部数据：刷新数据、退出登录、关闭程序时调用
void lab_model_free(LabModel *m) {
    cJSON_Delete(m->devices);    // 删除一个 cJSON 数组会连同它里面的每条记录一起释放
    cJSON_Delete(m->bookings);   // 指针是 NULL 时 cJSON_Delete 什么都不做，所以重复调用也安全
    cJSON_Delete(m->profiles);
    cJSON_Delete(m->issues);
    cJSON_Delete(m->usage);
    memset(m, 0, sizeof(*m));    // 5 个指针全部置为 NULL，防止之后误用已经释放的内存（悬空指针）
}
