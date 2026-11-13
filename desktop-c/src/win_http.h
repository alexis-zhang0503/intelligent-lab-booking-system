/*
 * win_http.h —— Windows 版"网络发送器"的声明
 *
 * 功能：告诉 main.c 有一个函数 lab_win_http 可以在 Windows 上发 HTTPS 请求。
 * 用法：程序启动时执行 client.transport = lab_win_http; 之后业务层所有网络请求都由它发出。
 * 它的参数顺序必须和 client.h 里的 LabTransport 完全一致，否则不能赋给 transport。
 */
#ifndef LAB_WIN_HTTP_H   // 头文件保护：防止被重复包含
#define LAB_WIN_HTTP_H
#include "client.h"      // 需要 client.h 里的类型（参数说明见 client.h 模块 3）
// 参数：(上下文, 服务器, 方法, 路径, apikey, 访问令牌, 请求体, 写回状态码, 写回错误, 错误缓冲区大小)
// 返回：响应内容（堆内存，调用者 free）；网络失败返回 NULL
char *lab_win_http(void *, const char *, const char *, const char *, const char *, const char *, const char *,
                   int *, char *, size_t);
#endif
