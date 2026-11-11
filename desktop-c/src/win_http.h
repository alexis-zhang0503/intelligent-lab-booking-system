//【答辩注释】Windows HTTPS 适配器声明；参数顺序与 LabTransport 一致。返回堆内存字符串，由共享请求层负责 free。
//【讲解 L1】按 LAB_WIN_HTTP_H 是否已定义选择编译分支，避免重复定义或适配平台。
#ifndef LAB_WIN_HTTP_H
//【讲解 L2】定义 LAB_WIN_HTTP_H，用于编译配置或头文件保护。
#define LAB_WIN_HTTP_H
//【讲解 L3】引入 client.h 的类型和函数声明。
#include "client.h"
//【讲解 L4】函数 lab_win_http：Windows 传输回调；负责 HTTPS、状态码、响应缓冲及资源清理，返回堆内存给共享层。
char *lab_win_http(void *, const char *, const char *, const char *, const char *, const char *, const char *,
                   //【讲解 L5】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
                   int *, char *, size_t);
//【讲解 L6】结束当前条件编译区域。
#endif
