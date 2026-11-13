/*
 * win_http.c —— Windows 版"网络发送器"
 *
 * 功能：业务层说"把这段 JSON 发到某个地址"，这个文件负责真正把它通过 HTTPS 发出去，
 *       再把服务器的回复原样带回来。它不懂预约、登录这些业务，只管收发。
 *
 * 一次请求的完整过程（对应下面的模块 3～7）：
 *   准备参数 → 打开网络会话 → 连接服务器 → 创建请求 → 拼请求头 → 发送 →
 *   读状态码 → 分块读回复 → 清理所有资源 → 把回复交给业务层
 *
 * 用到的是 Windows 自带的 WinHTTP 库，不需要安装第三方网络库。
 */

/* ===================== 模块 1：编译配置与头文件 =====================
 * 功能：让 Windows API 使用"宽字符"（UTF-16）版本，并引入需要的库。
 */
#ifndef UNICODE        // 如果编译命令里没有定义 UNICODE
#define UNICODE        // 就在这里定义，让 Windows 头文件选用 W 结尾的宽字符函数
#endif
#ifndef _UNICODE       // C 运行库用的同类开关，和上面保持一致
#define _UNICODE
#endif
#include <windows.h>   // Windows 基础类型和函数，例如 MultiByteToWideChar、GetLastError
#include "win_http.h"  // 本文件要实现的函数声明
#include <stdio.h>     // snprintf：拼请求头、写错误信息
#include <stdlib.h>    // calloc / realloc / free：申请和释放内存
#include <string.h>    // strlen：计算字符串长度
#include <winhttp.h>   // WinHTTP 网络库：WinHttpOpen、WinHttpSendRequest 等

/* ===================== 模块 2：文字编码转换 =====================
 * 功能：业务层的文字都是 UTF-8（char），WinHTTP 只接受 UTF-16（wchar_t），发请求前要转换。
 * 返回新申请的内存，调用者负责 free；转换失败返回 NULL。
 */
static wchar_t *wide(const char *s) {
    // 步骤 1：第一次调用只"问"需要多少个宽字符（最后一个参数传 0）。
    //         MB_ERR_INVALID_CHARS：遇到不合法的 UTF-8 直接失败，返回 0
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    // 步骤 2：按需要的长度申请内存；calloc 会把内存清零。n 为 0 说明转换失败，不申请
    wchar_t *p = n ? calloc((size_t)n, sizeof(wchar_t)) : NULL;
    // 步骤 3：申请成功后，第二次调用才真正转换，结果写进 p
    if (p)
        MultiByteToWideChar(CP_UTF8, 0, s, -1, p, n);
    return p;
}

/* ===================== 模块 3：准备阶段 =====================
 * 功能：lab_win_http 是对外唯一的函数。这一段先声明所有要用的资源并全部设为空，
 *       再把地址、方法、路径转成宽字符。
 * 为什么一开始全设成 NULL：函数中途任何一步失败都会跳到末尾统一清理，
 *       清理代码靠"是不是 NULL"判断哪些资源已经申请过、需要释放。
 */
char *lab_win_http(void *ctx, const char *host, const char *method, const char *path, const char *key,
                   const char *token, const char *body, int *status, char *error, size_t cap) {
    HINTERNET session = NULL, connection = NULL, request = NULL; // 三层网络句柄：会话 → 连接 → 请求
    wchar_t *h = NULL, *m = NULL, *p = NULL, *headers = NULL;    // 转成宽字符后的地址、方法、路径、请求头
    char *result = NULL;      // 服务器回复的内容，最后返回给业务层
    size_t used = 0;          // result 里已经存了多少字节
    // n：本次可读字节数；got：实际读到字节数；code：HTTP 状态码；redirect：禁止自动跳转的选项值
    DWORD n, got, code = 0, codeSize = sizeof(code), redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    char *utf8 = NULL;        // 先用 UTF-8 拼好的请求头，之后再转宽字符
    (void)ctx;                // Windows 版用不到上下文参数；这样写是告诉编译器"故意不用"，避免警告
    *status = 0;              // 先清空输出参数，失败时调用方看到的是 0 而不是上次的旧值
    error[0] = 0;             // 错误信息先设为空字符串
    h = wide(host);           // 服务器地址转宽字符
    m = wide(method);         // "GET" / "POST" 转宽字符
    p = wide(path);           // 请求路径转宽字符
    if (!h || !m || !p)       // 任何一个转换失败（内存不足或编码不合法）
        goto fail;            // 跳到统一失败出口

    /* ===================== 模块 4：建立 HTTPS 连接 =====================
     * 功能：按 WinHTTP 的固定三步打开网络通道：会话 → 连接服务器 → 创建一个请求。
     */
    // 步骤 1：打开会话。第一个参数是程序的"名片"（User-Agent），两个相邻字符串会被编译器自动拼成一个；
    //         AUTOMATIC_PROXY：自动使用系统代理设置，校园网有代理时也能连上
    session =
        WinHttpOpen(L"LabReservationNativeC/"
                    L"3.0.0",
                    WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
        goto fail;
    // 步骤 2：设置超时（毫秒）：域名解析 10 秒、连接 10 秒、发送 15 秒、接收 20 秒。
    //         网络断开时不会一直卡住，到时间就报错
    WinHttpSetTimeouts(session, 10000, 10000, 15000, 20000);
    // 步骤 3：连接服务器的 443 端口（HTTPS 默认端口）
    connection = WinHttpConnect(session, h, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection)
        goto fail;
    // 步骤 4：创建请求。WINHTTP_FLAG_SECURE 表示走加密的 HTTPS，令牌和密码不会被明文截获
    request = WinHttpOpenRequest(connection, m, p, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 WINHTTP_FLAG_SECURE);
    if (!request)
        goto fail;
    // 步骤 5：禁止自动跳转。如果服务器回复"请去另一个地址"，不跟过去，
    //         防止带着用户令牌的请求被转发到别的网站
    if (!WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)))
        goto fail;

    /* ===================== 模块 5：拼请求头并发送 =====================
     * 功能：告诉服务器"我发的是 JSON、我是哪个项目、我是哪个用户"，然后把请求体发出去。
     */
    // 步骤 1：按实际长度申请请求头缓冲区。160 字节留给固定文字（Content-Type 等），
    //         再加上 key 和 token 的长度，保证一定放得下
    utf8 = calloc(strlen(key) + strlen(token) + 160, 1);
    if (!utf8)
        goto fail;
    // 步骤 2：拼出请求头，每行用 \r\n 结尾（HTTP 协议规定）。
    //         ? : 的作用：key 或 token 为空字符串时，对应那一行整行不写
    snprintf(utf8, strlen(key) + strlen(token) + 160,
             "Content-Type: application/json\r\nAccept: application/json\r\n%s%s%s%s%s%s",
             *key ? "apikey: " : "", key, *key ? "\r\n" : "",                       // 第 3 行：apikey: 项目标识
             *token ? "Authorization: Bearer " : "", token,                         // 第 4 行：Authorization: Bearer 用户令牌
             *token ? "\r\n" : "");
    // 步骤 3：请求头转成宽字符，WinHTTP 只接受这种格式
    headers = wide(utf8);
    if (!headers)
        goto fail;
    // 步骤 4：发送请求并等待服务器回复。
    //         (DWORD)-1 表示请求头以 \0 结尾、让 WinHTTP 自己算长度；
    //         请求体为空（GET 请求）时传 WINHTTP_NO_REQUEST_DATA
    if (!WinHttpSendRequest(request, headers, (DWORD)-1, *body ? (void *)body : WINHTTP_NO_REQUEST_DATA,
                            (DWORD)strlen(body), (DWORD)strlen(body), 0) ||
        !WinHttpReceiveResponse(request, NULL))   // 两步任一失败都算网络失败
        goto fail;
    // 步骤 5：读出 HTTP 状态码（200 成功、401 令牌过期、404 不存在……），
    //         QUERY_FLAG_NUMBER 让它直接给数字而不是文字
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &code, &codeSize, WINHTTP_NO_HEADER_INDEX))
        goto fail;
    *status = (int)code;      // 通过输出参数把状态码交给调用方，由业务层决定怎么处理

    /* ===================== 模块 6：分块读取回复 =====================
     * 功能：服务器的回复可能很长，而且是分批到达的，所以要循环读，边读边把缓冲区变大。
     */
    result = calloc(1, 1);    // 先申请 1 字节并清零，相当于一个空字符串；回复为空时也能正常返回 ""
    if (!result)
        goto fail;
    do {
        char *next;
        n = 0;
        // 步骤 1：问 WinHTTP 现在有多少字节可以读
        if (!WinHttpQueryDataAvailable(request, &n))
            goto fail;
        if (!n)               // 0 字节说明已经读完
            break;
        // 步骤 2：总量超过 16 MB 就放弃，防止服务器异常时把内存耗尽
        if (used + n > 16 * 1024 * 1024) {
            SetLastError(ERROR_FILE_TOO_LARGE);   // 设置错误码，失败提示里会显示它
            goto fail;
        }
        // 步骤 3：把缓冲区扩大到"已有 + 新到 + 1"，+1 留给字符串结尾的 \0。
        //         先存进临时变量 next：如果 realloc 失败返回 NULL，原来的 result 地址还在，
        //         失败出口仍能 free 它；直接写 result = realloc(...) 会丢掉旧地址造成内存泄漏
        next = realloc(result, used + n + 1);
        if (!next)
            goto fail;
        result = next;
        // 步骤 4：把新数据读到缓冲区末尾（result + used 处），got 是实际读到的字节数
        if (!WinHttpReadData(request, result + used, n, &got))
            goto fail;
        used += got;          // 更新已读总量
        result[used] = 0;     // 末尾补 \0，保证 result 始终是一个合法的 C 字符串
        if (!got)             // 一个字节都没读到，也说明结束了
            break;
    } while (n);
    goto done;                // 一切顺利：跳过失败处理，直接去清理

    /* ===================== 模块 7：统一出口（失败处理 + 资源清理） =====================
     * 功能：不管成功还是失败，最后都从这里离开，保证每一个申请过的资源都被释放。
     * 为什么用 goto：C 没有 try/finally，也没有析构函数。前面任何一步失败都跳到 fail，
     *       不用在每个失败点重复写一遍释放代码，也就不容易漏掉。这是 C 项目里很常见的写法。
     */
fail:                         // 只有失败时才会进入这里
    // 写一条用户能看懂的中文错误，附上 Windows 错误码方便排查；
    // 提醒"先刷新再重试"：请求可能已经到达服务器只是回复超时，盲目重试可能重复操作
    snprintf(error, cap,
             "网络请求失败（Windows 错误 %lu），请检查网络后刷新；若提交超时，先刷新确认结果，再重试。",
             (unsigned long)GetLastError());
    free(result);             // 读了一半的回复不能用，释放掉（free(NULL) 是安全的）
    result = NULL;            // 返回 NULL 告诉调用方"失败了"
done:                         // 成功和失败都会经过这里
    if (headers) {
        // 请求头里含有用户令牌：释放前先用 SecureZeroMemory 清零。
        // 普通 memset 可能被编译器当作"写了没人读"而优化掉，SecureZeroMemory 保证一定执行
        SecureZeroMemory(headers, wcslen(headers) * sizeof(wchar_t));
        free(headers);
    }
    if (utf8) {
        SecureZeroMemory(utf8, strlen(utf8));   // UTF-8 版本的请求头同样含令牌，同样清零
        free(utf8);
    }
    free(h);                  // 释放三个转换出来的宽字符串
    free(m);
    free(p);
    // 按"后打开先关闭"的顺序关闭网络句柄：请求 → 连接 → 会话
    if (request)
        WinHttpCloseHandle(request);
    if (connection)
        WinHttpCloseHandle(connection);
    if (session)
        WinHttpCloseHandle(session);
    return result;            // 成功：回复内容（调用者负责 free）；失败：NULL
}
