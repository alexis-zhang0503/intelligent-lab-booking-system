#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#include <windows.h>
#include "win_http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winhttp.h>
static wchar_t *wide(const char *s) {
    int n = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, s, -1, NULL, 0);
    wchar_t *p = n ? calloc((size_t)n, sizeof(wchar_t)) : NULL;
    if (p)
        MultiByteToWideChar(CP_UTF8, 0, s, -1, p, n);
    return p;
}
char *lab_win_http(void *ctx, const char *host, const char *method, const char *path, const char *key,
                   const char *token, const char *body, int *status, char *error, size_t cap) {
    HINTERNET session = NULL, connection = NULL, request = NULL;
    wchar_t *h = NULL, *m = NULL, *p = NULL, *headers = NULL;
    char *result = NULL;
    size_t used = 0;
    DWORD n, got, code = 0, codeSize = sizeof(code), redirect = WINHTTP_OPTION_REDIRECT_POLICY_NEVER;
    char *utf8 = NULL;
    (void)ctx;
    *status = 0;
    error[0] = 0;
    h = wide(host);
    m = wide(method);
    p = wide(path);
    if (!h || !m || !p)
        goto fail;
    session =
        WinHttpOpen(L"LabReservationNativeC/"
                    L"3.0.0",
                    WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY, WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session)
        goto fail;
    WinHttpSetTimeouts(session, 10000, 10000, 15000, 20000);
    connection = WinHttpConnect(session, h, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection)
        goto fail;
    request = WinHttpOpenRequest(connection, m, p, NULL, WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 WINHTTP_FLAG_SECURE);
    if (!request)
        goto fail;
    if (!WinHttpSetOption(request, WINHTTP_OPTION_REDIRECT_POLICY, &redirect, sizeof(redirect)))
        goto fail;
    utf8 = calloc(strlen(key) + strlen(token) + 160, 1);
    if (!utf8)
        goto fail;
    snprintf(utf8, strlen(key) + strlen(token) + 160,
             "Content-Type: application/json\r\nAccept: application/json\r\n%s%s%s%s%s%s",
             *key ? "apikey: " : "", key, *key ? "\r\n" : "", *token ? "Authorization: Bearer " : "", token,
             *token ? "\r\n" : "");
    headers = wide(utf8);
    if (!headers)
        goto fail;
    if (!WinHttpSendRequest(request, headers, (DWORD)-1, *body ? (void *)body : WINHTTP_NO_REQUEST_DATA,
                            (DWORD)strlen(body), (DWORD)strlen(body), 0) ||
        !WinHttpReceiveResponse(request, NULL))
        goto fail;
    if (!WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                             WINHTTP_HEADER_NAME_BY_INDEX, &code, &codeSize, WINHTTP_NO_HEADER_INDEX))
        goto fail;
    *status = (int)code;
    result = calloc(1, 1);
    if (!result)
        goto fail;
    do {
        char *next;
        n = 0;
        if (!WinHttpQueryDataAvailable(request, &n))
            goto fail;
        if (!n)
            break;
        if (used + n > 16 * 1024 * 1024) {
            SetLastError(ERROR_FILE_TOO_LARGE);
            goto fail;
        }
        next = realloc(result, used + n + 1);
        if (!next)
            goto fail;
        result = next;
        if (!WinHttpReadData(request, result + used, n, &got))
            goto fail;
        used += got;
        result[used] = 0;
        if (!got)
            break;
    } while (n);
    goto done;
fail:
    snprintf(error, cap,
             "网络请求失败（Windows 错误 %lu），请检查网络后刷新；若提交超时，先刷新确认结果，再重试。",
             (unsigned long)GetLastError());
    free(result);
    result = NULL;
done:
    if (headers) {
        SecureZeroMemory(headers, wcslen(headers) * sizeof(wchar_t));
        free(headers);
    }
    if (utf8) {
        SecureZeroMemory(utf8, strlen(utf8));
        free(utf8);
    }
    free(h);
    free(m);
    free(p);
    if (request)
        WinHttpCloseHandle(request);
    if (connection)
        WinHttpCloseHandle(connection);
    if (session)
        WinHttpCloseHandle(session);
    return result;
}
