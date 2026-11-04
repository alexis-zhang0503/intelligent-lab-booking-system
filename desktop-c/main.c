#define UNICODE
#define _UNICODE
#include <windows.h>
#include <winhttp.h>
#include <shellapi.h>
#include <stdio.h>

/* Build: cl /utf-8 main.c /link /SUBSYSTEM:WINDOWS winhttp.lib shell32.lib */
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, PWSTR command, int show)
{
    const wchar_t *host = L"alexis-zhang0503.github.io";
    const wchar_t *path = L"/intelligent-lab-booking-system/";
    for (;;) {
        DWORD status = 0, size = sizeof(status);
        HINTERNET session = WinHttpOpen(L"LabBooking/1.0", WINHTTP_ACCESS_TYPE_AUTOMATIC_PROXY,
            WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
        HINTERNET connection = NULL, request = NULL;
        if (session) {
            WinHttpSetTimeouts(session, 8000, 8000, 8000, 8000);
            connection = WinHttpConnect(session, host, INTERNET_DEFAULT_HTTPS_PORT, 0);
        }
        if (connection) request = WinHttpOpenRequest(connection, L"HEAD", path, NULL,
            WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES, WINHTTP_FLAG_SECURE);
        if (request && WinHttpSendRequest(request, L"Cache-Control: no-cache\r\n", (DWORD)-1L,
            WINHTTP_NO_REQUEST_DATA, 0, 0, 0) && WinHttpReceiveResponse(request, NULL))
            WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                WINHTTP_HEADER_NAME_BY_INDEX, &status, &size, WINHTTP_NO_HEADER_INDEX);
        if (request) WinHttpCloseHandle(request);
        if (connection) WinHttpCloseHandle(connection);
        if (session) WinHttpCloseHandle(session);
        if (status >= 200 && status < 300) break;
        if (MessageBoxW(NULL, L"无法连接预约网站。请检查网络后重试。", L"实验室预约",
            MB_RETRYCANCEL | MB_ICONWARNING) != IDRETRY) return 1;
    }
    wchar_t args[512];
    swprintf_s(args, 512, L"--app=https://alexis-zhang0503.github.io/intelligent-lab-booking-system/?desktop_start=%llu",
        (unsigned long long)GetTickCount64());
    HINSTANCE result = ShellExecuteW(NULL, L"open", L"msedge.exe", args, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        MessageBoxW(NULL, L"未能启动 Microsoft Edge，请安装或修复 Edge 后重试。", L"实验室预约", MB_OK | MB_ICONERROR);
        return 2;
    }
    return 0;
}
