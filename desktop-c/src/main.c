#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#define CINTERFACE
#define COBJMACROS
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#include <objbase.h>
#include <strsafe.h>
#include "WebView2.h"
#include "site_policy.h"

#define ID_REFRESH 101
#define ID_OPEN_BROWSER 102
#define ID_INSTALL_RUNTIME 103
#define TOOLBAR_HEIGHT 38
static HWND app_window, status_label;
static ICoreWebView2Controller *controller;
static ICoreWebView2 *webview;
static HMODULE loader_module;
static int closing;
static wchar_t start_url[1024];
static EventRegistrationToken navigation_start_token, navigation_end_token, new_window_token;
static int events_registered;
typedef HRESULT (STDAPICALLTYPE *CreateEnvironmentFn)(PCWSTR, PCWSTR,
    ICoreWebView2EnvironmentOptions *, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *);

static void set_status(const wchar_t *text) { if (!closing) SetWindowTextW(status_label, text); }
static void show_failure(const wchar_t *text) {
    set_status(text);
    ShowWindow(GetDlgItem(app_window, ID_OPEN_BROWSER), SW_SHOW);
}
static void resize_webview(void) {
    RECT bounds;
    if (!controller || closing) return;
    GetClientRect(app_window, &bounds);
    bounds.top = TOOLBAR_HEIGHT;
    if (bounds.bottom < bounds.top) bounds.bottom = bounds.top;
    ICoreWebView2Controller_put_Bounds(controller, bounds);
}
static void navigate_latest(void) {
    FILETIME ft;
    ULARGE_INTEGER timestamp;
    if (!webview || closing) return;
    GetSystemTimeAsFileTime(&ft);
    timestamp.LowPart = ft.dwLowDateTime;
    timestamp.HighPart = ft.dwHighDateTime;
    /* A new document URL on every launch/retry, without clearing the login profile. */
    StringCchPrintfW(start_url, ARRAYSIZE(start_url), L"%ls?desktop=1&launch=%llu", LAB_SITE_URL,
        (unsigned long long)timestamp.QuadPart);
    set_status(L"正在连接实验室…");
    if (FAILED(ICoreWebView2_Navigate(webview, start_url)))
        show_failure(L"网页打开失败，请点击刷新重试。");
}

/* Standard COM callbacks implemented as C structs and C vtables, not a C++ shim. */
#define CALLBACK_TYPE(Name, Interface, Id) \
    typedef struct Name { Interface iface; LONG refs; } Name; \
    static HRESULT STDMETHODCALLTYPE Name##_query(Interface *self, REFIID id, void **out) { \
        if (!out) return E_POINTER; \
        *out = NULL; \
        if (IsEqualIID(id, &IID_IUnknown) || IsEqualIID(id, &Id)) { \
            *out = self; InterlockedIncrement(&((Name *)self)->refs); return S_OK; \
        } \
        return E_NOINTERFACE; \
    } \
    static ULONG STDMETHODCALLTYPE Name##_addref(Interface *self) { \
        return (ULONG)InterlockedIncrement(&((Name *)self)->refs); \
    } \
    static ULONG STDMETHODCALLTYPE Name##_release(Interface *self) { \
        return (ULONG)InterlockedDecrement(&((Name *)self)->refs); \
    }
CALLBACK_TYPE(EnvironmentCallback, ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler,
    IID_ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler)
CALLBACK_TYPE(ControllerCallback, ICoreWebView2CreateCoreWebView2ControllerCompletedHandler,
    IID_ICoreWebView2CreateCoreWebView2ControllerCompletedHandler)
CALLBACK_TYPE(NavigationStartCallback, ICoreWebView2NavigationStartingEventHandler,
    IID_ICoreWebView2NavigationStartingEventHandler)
CALLBACK_TYPE(NavigationEndCallback, ICoreWebView2NavigationCompletedEventHandler,
    IID_ICoreWebView2NavigationCompletedEventHandler)
CALLBACK_TYPE(NewWindowCallback, ICoreWebView2NewWindowRequestedEventHandler,
    IID_ICoreWebView2NewWindowRequestedEventHandler)

static HRESULT STDMETHODCALLTYPE navigation_start(ICoreWebView2NavigationStartingEventHandler *self,
    ICoreWebView2 *sender, ICoreWebView2NavigationStartingEventArgs *args) {
    LPWSTR uri = NULL;
    BOOL user_initiated = FALSE;
    (void)self; (void)sender;
    if (closing) return S_OK;
    ICoreWebView2NavigationStartingEventArgs_get_Uri(args, &uri);
    ICoreWebView2NavigationStartingEventArgs_get_IsUserInitiated(args, &user_initiated);
    if (!lab_is_site_url(uri)) {
        ICoreWebView2NavigationStartingEventArgs_put_Cancel(args, TRUE);
        if (user_initiated && lab_is_https(uri))
            ShellExecuteW(app_window, L"open", uri, NULL, NULL, SW_SHOWNORMAL);
    } else set_status(L"正在加载…");
    CoTaskMemFree(uri);
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE navigation_end(ICoreWebView2NavigationCompletedEventHandler *self,
    ICoreWebView2 *sender, ICoreWebView2NavigationCompletedEventArgs *args) {
    BOOL success = FALSE;
    (void)self; (void)sender;
    if (closing) return S_OK;
    ICoreWebView2NavigationCompletedEventArgs_get_IsSuccess(args, &success);
    if (success) {
        set_status(L"已连接");
        ShowWindow(GetDlgItem(app_window, ID_OPEN_BROWSER), SW_HIDE);
    } else show_failure(L"连接失败，请检查网络后刷新。");
    return S_OK;
}
static HRESULT STDMETHODCALLTYPE new_window_requested(ICoreWebView2NewWindowRequestedEventHandler *self,
    ICoreWebView2 *sender, ICoreWebView2NewWindowRequestedEventArgs *args) {
    LPWSTR uri = NULL;
    BOOL user_initiated = FALSE;
    (void)self; (void)sender;
    ICoreWebView2NewWindowRequestedEventArgs_put_Handled(args, TRUE);
    if (closing) return S_OK;
    ICoreWebView2NewWindowRequestedEventArgs_get_Uri(args, &uri);
    ICoreWebView2NewWindowRequestedEventArgs_get_IsUserInitiated(args, &user_initiated);
    if (user_initiated && lab_is_https(uri))
        ShellExecuteW(app_window, L"open", uri, NULL, NULL, SW_SHOWNORMAL);
    CoTaskMemFree(uri);
    return S_OK;
}
static ICoreWebView2NavigationStartingEventHandlerVtbl start_vtable = {
    NavigationStartCallback_query, NavigationStartCallback_addref, NavigationStartCallback_release, navigation_start
};
static NavigationStartCallback start_callback = {{&start_vtable}, 1};
static ICoreWebView2NavigationCompletedEventHandlerVtbl end_vtable = {
    NavigationEndCallback_query, NavigationEndCallback_addref, NavigationEndCallback_release, navigation_end
};
static NavigationEndCallback end_callback = {{&end_vtable}, 1};
static ICoreWebView2NewWindowRequestedEventHandlerVtbl new_window_vtable = {
    NewWindowCallback_query, NewWindowCallback_addref, NewWindowCallback_release, new_window_requested
};
static NewWindowCallback new_window_callback = {{&new_window_vtable}, 1};

static HRESULT STDMETHODCALLTYPE controller_created(ICoreWebView2CreateCoreWebView2ControllerCompletedHandler *self,
    HRESULT error, ICoreWebView2Controller *result) {
    ICoreWebView2Settings *settings = NULL;
    HRESULT hr;
    (void)self;
    if (closing) { if (result) ICoreWebView2Controller_Close(result); return S_OK; }
    if (FAILED(error) || !result) { show_failure(L"界面初始化失败，请重启程序。"); return S_OK; }
    controller = result;
    ICoreWebView2Controller_AddRef(controller);
    hr = ICoreWebView2Controller_get_CoreWebView2(controller, &webview);
    if (FAILED(hr) || !webview) { show_failure(L"网页组件初始化失败。"); return S_OK; }
    if (SUCCEEDED(ICoreWebView2_get_Settings(webview, &settings))) {
        ICoreWebView2Settings_put_IsScriptEnabled(settings, TRUE);
        ICoreWebView2Settings_put_AreDefaultScriptDialogsEnabled(settings, TRUE);
        /* No native messaging bridge is exposed to the remote page. */
        ICoreWebView2Settings_put_IsWebMessageEnabled(settings, FALSE);
        ICoreWebView2Settings_Release(settings);
    }
    hr = ICoreWebView2_add_NavigationStarting(webview, &start_callback.iface, &navigation_start_token);
    if (SUCCEEDED(hr)) hr = ICoreWebView2_add_NavigationCompleted(webview, &end_callback.iface, &navigation_end_token);
    if (SUCCEEDED(hr)) hr = ICoreWebView2_add_NewWindowRequested(webview, &new_window_callback.iface, &new_window_token);
    if (FAILED(hr)) { show_failure(L"网页事件初始化失败，请重启程序。"); return S_OK; }
    events_registered = 1;
    resize_webview();
    ICoreWebView2Controller_put_IsVisible(controller, TRUE);
    navigate_latest();
    return S_OK;
}
static ICoreWebView2CreateCoreWebView2ControllerCompletedHandlerVtbl controller_vtable = {
    ControllerCallback_query, ControllerCallback_addref, ControllerCallback_release, controller_created
};
static ControllerCallback controller_callback = {{&controller_vtable}, 1};
static HRESULT STDMETHODCALLTYPE environment_created(ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandler *self,
    HRESULT error, ICoreWebView2Environment *result) {
    (void)self;
    if (closing) return S_OK;
    if (FAILED(error) || !result) {
        show_failure(L"运行环境初始化失败，请检查 WebView2 Runtime。");
        ShowWindow(GetDlgItem(app_window, ID_INSTALL_RUNTIME), SW_SHOW);
        return S_OK;
    }
    if (FAILED(ICoreWebView2Environment_CreateCoreWebView2Controller(result, app_window, &controller_callback.iface)))
        show_failure(L"无法创建网页窗口。");
    return S_OK;
}
static ICoreWebView2CreateCoreWebView2EnvironmentCompletedHandlerVtbl environment_vtable = {
    EnvironmentCallback_query, EnvironmentCallback_addref, EnvironmentCallback_release, environment_created
};
static EnvironmentCallback environment_callback = {{&environment_vtable}, 1};

static void initialize_webview(void) {
    wchar_t dll_path[MAX_PATH], profile[MAX_PATH], base[MAX_PATH];
    wchar_t *slash;
    CreateEnvironmentFn create_environment;
    HRESULT hr;
    if (!GetModuleFileNameW(NULL, dll_path, ARRAYSIZE(dll_path))) return;
    slash = wcsrchr(dll_path, L'\\');
    if (!slash) return;
    slash[1] = L'\0';
    if (FAILED(StringCchCatW(dll_path, ARRAYSIZE(dll_path), L"WebView2Loader.dll"))) return;
    /* Load the packaged loader, never a DLL from the current working directory. */
    loader_module = LoadLibraryExW(dll_path, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_SYSTEM32);
    if (!loader_module) { show_failure(L"缺少 WebView2Loader.dll，请完整解压程序包。"); return; }
    create_environment = (CreateEnvironmentFn)(void *)GetProcAddress(loader_module, "CreateCoreWebView2EnvironmentWithOptions");
    if (!create_environment) { show_failure(L"网页组件版本无效，请重新下载程序。"); return; }
    if (FAILED(SHGetFolderPathW(NULL, CSIDL_LOCAL_APPDATA, NULL, SHGFP_TYPE_CURRENT, base))) {
        show_failure(L"无法访问用户数据目录。"); return;
    }
    if (FAILED(StringCchPrintfW(profile, ARRAYSIZE(profile), L"%ls\\NJUPT-Lab-Desktop", base))) return;
    hr = create_environment(NULL, profile, NULL, &environment_callback.iface);
    if (hr == HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND)) {
        show_failure(L"需要安装 Microsoft Edge WebView2 Runtime。");
        ShowWindow(GetDlgItem(app_window, ID_INSTALL_RUNTIME), SW_SHOW);
    } else if (FAILED(hr)) show_failure(L"网页运行环境启动失败，请重启后重试。");
}
static LRESULT CALLBACK window_proc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE:
        CreateWindowW(L"BUTTON", L"刷新 (F5)", WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
            8, 5, 100, 28, hwnd, (HMENU)ID_REFRESH, NULL, NULL);
        status_label = CreateWindowW(L"STATIC", L"正在启动…", WS_CHILD | WS_VISIBLE | SS_LEFT,
            122, 11, 470, 22, hwnd, NULL, NULL, NULL);
        CreateWindowW(L"BUTTON", L"在浏览器打开", WS_CHILD | BS_PUSHBUTTON,
            600, 5, 128, 28, hwnd, (HMENU)ID_OPEN_BROWSER, NULL, NULL);
        CreateWindowW(L"BUTTON", L"安装运行环境", WS_CHILD | BS_PUSHBUTTON,
            740, 5, 128, 28, hwnd, (HMENU)ID_INSTALL_RUNTIME, NULL, NULL);
        return 0;
    case WM_SIZE: resize_webview(); return 0;
    case WM_MOVE:
        if (controller) ICoreWebView2Controller_NotifyParentWindowPositionChanged(controller);
        return 0;
    case WM_COMMAND:
        if (LOWORD(wp) == ID_REFRESH) {
            if (webview) navigate_latest();
            else MessageBoxW(hwnd, L"请关闭程序并重新启动；如缺少运行环境，请先安装。", L"实验室预约", MB_OK);
        } else if (LOWORD(wp) == ID_OPEN_BROWSER)
            ShellExecuteW(hwnd, L"open", LAB_SITE_URL, NULL, NULL, SW_SHOWNORMAL);
        else if (LOWORD(wp) == ID_INSTALL_RUNTIME)
            ShellExecuteW(hwnd, L"open", L"https://developer.microsoft.com/microsoft-edge/webview2/", NULL, NULL, SW_SHOWNORMAL);
        return 0;
    case WM_DESTROY:
        closing = 1;
        if (webview) {
            if (events_registered) {
                ICoreWebView2_remove_NavigationStarting(webview, navigation_start_token);
                ICoreWebView2_remove_NavigationCompleted(webview, navigation_end_token);
                ICoreWebView2_remove_NewWindowRequested(webview, new_window_token);
            }
            ICoreWebView2_Release(webview); webview = NULL;
        }
        if (controller) { ICoreWebView2Controller_Close(controller); ICoreWebView2Controller_Release(controller); controller = NULL; }
        PostQuitMessage(0); return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR command_line, int show) {
    WNDCLASSW wc = {0};
    MSG msg;
    HRESULT hr;
    (void)previous; (void)command_line;
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(hr)) return 1;
    wc.lpfnWndProc = window_proc; wc.hInstance = instance;
    wc.lpszClassName = L"NJUPTLabDesktopC";
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    if (!RegisterClassW(&wc)) { CoUninitialize(); return 1; }
    app_window = CreateWindowExW(0, wc.lpszClassName, L"NJUPT 实验室预约", WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 1280, 840, NULL, NULL, instance, NULL);
    if (!app_window) { CoUninitialize(); return 1; }
    ShowWindow(app_window, show); UpdateWindow(app_window);
    initialize_webview();
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (msg.message == WM_KEYDOWN && msg.wParam == VK_F5) { navigate_latest(); continue; }
        TranslateMessage(&msg); DispatchMessageW(&msg);
    }
    CoUninitialize();
    /* Callbacks can outlive Close during teardown. Keep the loader mapped until process exit. */
    return (int)msg.wParam;
}
