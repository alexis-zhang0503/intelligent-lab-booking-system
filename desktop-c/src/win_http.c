//【答辩注释】WinHTTP HTTPS 适配器：UTF-8 转宽字符、设置请求头、禁止自动重定向、分块读取响应并统一清理句柄。最大响应体为 16 MiB。
//【讲解 L1】按 UNICODE 是否已定义选择编译分支，避免重复定义或适配平台。
#ifndef UNICODE
//【讲解 L2】定义 UNICODE，用于编译配置或头文件保护。
#define UNICODE
//【讲解 L3】结束当前条件编译区域。
#endif
//【讲解 L4】按 _UNICODE 是否已定义选择编译分支，避免重复定义或适配平台。
#ifndef _UNICODE
//【讲解 L5】定义 _UNICODE，用于编译配置或头文件保护。
#define _UNICODE
//【讲解 L6】结束当前条件编译区域。
#endif
//【讲解 L7】引入 windows.h 的类型和函数声明。
#include <windows.h>
//【讲解 L8】引入 win_http.h 的类型和函数声明。
#include "win_http.h"
//【讲解 L9】引入 stdio.h 的类型和函数声明。
#include <stdio.h>
//【讲解 L10】引入 stdlib.h 的类型和函数声明。
#include <stdlib.h>
//【讲解 L11】引入 string.h 的类型和函数声明。
#include <string.h>
//【讲解 L12】引入 winhttp.h 的类型和函数声明。
#include <winhttp.h>
//【讲解 L13】函数 wide：把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
static wchar_t *wide(const char *s) {
    //【讲解 L14】把 UTF-8 转为 Windows 宽字符。
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    //【讲解 L15】申请并清零堆内存。
    wchar_t *p = n ? calloc((size_t)n, sizeof(wchar_t)) : NULL;
    //【讲解 L16】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (p)
        //【讲解 L17】把 UTF-8 转为 Windows 宽字符。
        MultiByteToWideChar(CP_UTF8, 0, s, -1, p, n);
    //【讲解 L18】结束当前函数并返回 p；具体成功或失败含义见函数说明。
    return p;
//【讲解 L19】结束当前代码块。
}
//【讲解 L20】函数 lab_win_http：Windows 传输回调；负责 HTTPS、状态码、响应缓冲及资源清理，返回堆内存给共享层。
char *lab_win_http(void *ctx, const char *host, const char *method, const char *path, const char *key,
                   //【讲解 L21】声明或更新 error（错误文本）、status（状态码或业务状态）；按本行表达式保存对应状态。
                   const char *token, const char *body, int *status, char *error, size_t cap) {
    //【讲解 L22】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    HINTERNET session = NULL, connection = NULL, request = NULL;
    //【讲解 L23】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    wchar_t *h = NULL, *m = NULL, *p = NULL, *headers = NULL;
    //【讲解 L24】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    char *result = NULL;
    //【讲解 L25】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    size_t used = 0;
    //【讲解 L26】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    DWORD n, got, code = 0, codeSize = sizeof(code), redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    //【讲解 L27】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
    char *utf8 = NULL;
    //【讲解 L28】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    (void)ctx;
    *status = 0;
    //【讲解 L30】声明或更新 error（错误文本）；按本行表达式保存对应状态。
    error[0] = 0;
    //【讲解 L31】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
    h = wide(host);
    //【讲解 L32】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
    m = wide(method);
    //【讲解 L33】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
    p = wide(path);
    //【讲解 L34】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!h || !m || !p)
        //【讲解 L35】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L36】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    session =
        //【讲解 L37】执行 WinHttpOpen，参数沿用本行给出的对象、标志和容量。
        WinHttpOpen(L"LabReservationNativeC/"
                    //【讲解 L38】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
                    L"3.0.0",
                    //【讲解 L39】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                    WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    //【讲解 L40】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!session)
        //【讲解 L41】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L42】执行 WinHttpSetTimeouts，参数沿用本行给出的对象、标志和容量。
    WinHttpSetTimeouts(session, 10000, 10000, 15000, 20000);
    //【讲解 L43】执行 WinHttpConnect，参数沿用本行给出的对象、标志和容量。
    connection = WinHttpConnect(session, h, INTERNET_DEFAULT_HTTPS_PORT, 0);
    //【讲解 L44】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!connection)
        //【讲解 L45】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L46】执行 WinHttpOpenRequest，参数沿用本行给出的对象、标志和容量。
    request = WinHttpOpenRequest(connection, m, p, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 //【讲解 L47】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                                 WINHTTP_FLAG_SECURE);
    //【讲解 L48】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!request)
        //【讲解 L49】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L50】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)))
        //【讲解 L51】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L52】申请并清零堆内存；计算字符串字节长度。
    utf8 = calloc(strlen(key) + strlen(token) + 160, 1);
    //【讲解 L53】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!utf8)
        //【讲解 L54】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L55】按容量格式化 UTF-8 文本；计算字符串字节长度。
    snprintf(utf8, strlen(key) + strlen(token) + 160,
             //【讲解 L56】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
             "Content-Type: application/json\r\nAccept: application/json\r\n%s%s%s%s%s%s",
             *key ? "apikey: " : "", key, *key ? "\r\n" : "", *token ? "Authorization: Bearer " : "", token,
             *token ? "\r\n" : "");
    //【讲解 L59】调用 wide，把业务层 UTF-8 转为 Windows UTF-16，分配的字符串由调用方 free。
    headers = wide(utf8);
    //【讲解 L60】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!headers)
        //【讲解 L61】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L62】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!WinHttpSendRequest(request, headers, (DWORD)-1, *body ? (void *)body : WINHTTP_NO_REQUEST_DATA,
                            //【讲解 L63】计算字符串字节长度。
                            (DWORD)strlen(body), (DWORD)strlen(body), 0) ||
        //【讲解 L64】执行 WinHttpReceiveResponse，参数沿用本行给出的对象、标志和容量。
        !WinHttpReceiveResponse(request, NULL))
        //【讲解 L65】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L66】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             //【讲解 L67】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
                             WINHTTP_HEADER_NAME_BY_INDEX, &code, &codeSize, WINHTTP_NO_HEADER_INDEX))
        //【讲解 L68】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    *status = (int)code;
    //【讲解 L70】申请并清零堆内存。
    result = calloc(1, 1);
    //【讲解 L71】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (!result)
        //【讲解 L72】转到 fail 统一处理，避免遗漏资源清理。
        goto fail;
    //【讲解 L73】承接相邻行的参数、表达式或初始化项；与前后行共同构成一条完整 C 语句。
    do {
        //【讲解 L74】声明本行类型的数据或初始化其值；数组方括号给出固定容量，指针按后续分支使用。
        char *next;
        //【讲解 L75】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        n = 0;
        //【讲解 L76】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!WinHttpQueryDataAvailable(request, &n))
            //【讲解 L77】转到 fail 统一处理，避免遗漏资源清理。
            goto fail;
        //【讲解 L78】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!n)
            //【讲解 L79】结束当前最近的循环或 switch 分支。
            break;
        //【讲解 L80】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (used + n > 16 * 1024 * 1024) {
            //【讲解 L81】执行 SetLastError，参数沿用本行给出的对象、标志和容量。
            SetLastError(ERROR_FILE_TOO_LARGE);
            //【讲解 L82】转到 fail 统一处理，避免遗漏资源清理。
            goto fail;
        //【讲解 L83】结束当前代码块。
        }
        //【讲解 L84】调整缓冲区容量。
        next = realloc(result, used + n + 1);
        //【讲解 L85】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!next)
            //【讲解 L86】转到 fail 统一处理，避免遗漏资源清理。
            goto fail;
        //【讲解 L87】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        result = next;
        //【讲解 L88】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!WinHttpReadData(request, result + used, n, &got))
            //【讲解 L89】转到 fail 统一处理，避免遗漏资源清理。
            goto fail;
        //【讲解 L90】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        used += got;
        //【讲解 L91】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
        result[used] = 0;
        //【讲解 L92】检查本行条件，满足后执行紧随其后的语句或代码块。
        if (!got)
            //【讲解 L93】结束当前最近的循环或 switch 分支。
            break;
    //【讲解 L94】结束当前代码块或类型声明。
    } while (n);
    //【讲解 L95】转到 done 统一处理，避免遗漏资源清理。
    goto done;
//【讲解 L96】失败清理入口，生成错误并释放资源。
fail:
    //【讲解 L97】按容量格式化 UTF-8 文本。
    snprintf(error, cap,
             //【讲解 L98】继续上一行的文本或初始化数据，保留接口字段、提示语及转义字符。
             "网络请求失败（Windows 错误 %lu），请检查网络后刷新；若提交超时，先刷新确认结果，再重试。",
             //【讲解 L99】执行 GetLastError，参数沿用本行给出的对象、标志和容量。
             (unsigned long)GetLastError());
    //【讲解 L100】释放堆内存。
    free(result);
    //【讲解 L101】按右侧表达式更新本行左侧变量或成员，供后续步骤继续使用。
    result = NULL;
//【讲解 L102】统一退出入口，释放网络句柄和临时缓冲区。
done:
    //【讲解 L103】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (headers) {
        //【讲解 L104】清除敏感临时数据。
        SecureZeroMemory(headers, wcslen(headers) * sizeof(wchar_t));
        //【讲解 L105】释放堆内存。
        free(headers);
    //【讲解 L106】结束当前代码块。
    }
    //【讲解 L107】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (utf8) {
        //【讲解 L108】清除敏感临时数据；计算字符串字节长度。
        SecureZeroMemory(utf8, strlen(utf8));
        //【讲解 L109】释放堆内存。
        free(utf8);
    //【讲解 L110】结束当前代码块。
    }
    //【讲解 L111】释放堆内存。
    free(h);
    //【讲解 L112】释放堆内存。
    free(m);
    //【讲解 L113】释放堆内存。
    free(p);
    //【讲解 L114】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (request)
        //【讲解 L115】执行 WinHttpCloseHandle，参数沿用本行给出的对象、标志和容量。
        WinHttpCloseHandle(request);
    //【讲解 L116】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (connection)
        //【讲解 L117】执行 WinHttpCloseHandle，参数沿用本行给出的对象、标志和容量。
        WinHttpCloseHandle(connection);
    //【讲解 L118】检查本行条件，满足后执行紧随其后的语句或代码块。
    if (session)
        //【讲解 L119】执行 WinHttpCloseHandle，参数沿用本行给出的对象、标志和容量。
        WinHttpCloseHandle(session);
    //【讲解 L120】结束当前函数并返回 result；具体成功或失败含义见函数说明。
    return result;
//【讲解 L121】结束当前代码块。
}
