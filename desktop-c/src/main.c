#ifndef UNICODE
#define UNICODE
#endif
#ifndef _UNICODE
#define _UNICODE
#endif
#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0A00
#endif
#include <windows.h>
#include "client.h"
#include "win_http.h"
#include <commctrl.h>
#include <objbase.h>
#include <shellapi.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windowsx.h>
#define RESULT_MESSAGE (WM_APP + 1)
#define BG RGB(248, 240, 226)
#define INK RGB(67, 35, 15)
enum {
    ID_LOGIN = 100,
    ID_REGISTER,
    ID_REFRESH,
    ID_LOGOUT,
    ID_BOOK,
    ID_ATTEND,
    ID_CANCEL,
    ID_REPORT,
    ID_REPAIR,
    ID_ARCHIVE,
    ID_DEACTIVATE,
    ID_INVITE,
    ID_METRICS,
    ID_UPDATE,
    ID_PROFILE,
    ID_SUGGEST,
    ID_AGENT,
    ID_TAB = 200,
    ID_LIST,
    ID_DATE,
    ID_START,
    ID_END,
    ID_FIRST_DEVICE = 300
};
enum {
    JOB_LOGIN,
    JOB_REGISTER,
    JOB_REFRESH,
    JOB_BOOK,
    JOB_ATTEND,
    JOB_CANCEL,
    JOB_REPORT,
    JOB_REPAIR,
    JOB_ARCHIVE,
    JOB_DEACTIVATE,
    JOB_INVITE,
    JOB_METRICS,
    JOB_UPDATE,
    JOB_PROFILE,
    JOB_SUGGEST,
    JOB_AGENT
};
typedef struct {
    int kind, action, start, end, devices[40], count;
    LabClient client;
    LabModel model;
    cJSON *row;
    char identity[256], password[256], name[256], invite[256], date[16], note[2048];
    char keys[9][64], message[4096];
    int ok, loaded;
    HWND target;
} Job;
static HWND window, identity, password, nameEdit, inviteEdit, loginButton, registerButton, loginInfo;
static HWND tabs, list, dateEdit, startEdit, endEdit, noteEdit, statusText, helpText, whoText,
    deviceButtons[40];
static HWND actions[16], labels[4];
static HFONT font, bold;
static HBRUSH background;
static LabClient client;
static LabModel model;
static int busy, checkingUpdate, logged, page, selected[40];
static cJSON *listed[100000];
static int listedCount;
static const int actionIds[] = {ID_REFRESH, ID_LOGOUT, ID_BOOK,    ID_ATTEND,     ID_CANCEL,
                                ID_REPORT,  ID_REPAIR, ID_ARCHIVE, ID_DEACTIVATE, ID_INVITE,
                                ID_METRICS, ID_UPDATE, ID_PROFILE, ID_SUGGEST,    ID_AGENT};
static const wchar_t *actionNames[] = {L"刷新",         L"退出账号",     L"预约所选设备", L"签到 / 签退",
                                       L"取消预约",     L"提交故障",     L"完成维修",     L"归档故障",
                                       L"停用学生",     L"生成邀请码",   L"统计报表",     L"检查版本",
                                       L"修改本人资料", L"推荐空闲时段", L"智能分析"};
static wchar_t *wide(const char *s) {
    int n = MultiByteToWideChar(CP_UTF8, 0, s, -1, NULL, 0);
    wchar_t *p = calloc(n ? n : 1, sizeof(wchar_t));
    if (p && n)
        MultiByteToWideChar(CP_UTF8, 0, s, -1, p, n);
    return p;
}
static void set_utf8(HWND h, const char *s) {
    wchar_t *p = wide(s);
    if (p) {
        SetWindowTextW(h, p);
        free(p);
    }
}
static void get_utf8(HWND h, char *s, int n) {
    wchar_t b[2048];
    GetWindowTextW(h, b, 2048);
    if (!WideCharToMultiByte(CP_UTF8, 0, b, -1, s, n, NULL, NULL))
        s[0] = 0;
}
static void tell(const char *s, UINT flags) {
    wchar_t *p = wide(s);
    if (p) {
        MessageBoxW(window, p, L"实验室预约 · 原生 C", flags);
        free(p);
    }
}
static int admin(void) { return !strcmp(client.session.role, "admin"); }
static int64_t now_ms(void) {
    FILETIME f;
    ULARGE_INTEGER u;
    GetSystemTimeAsFileTime(&f);
    u.LowPart = f.dwLowDateTime;
    u.HighPart = f.dwHighDateTime;
    return (int64_t)(u.QuadPart / 10000 - 11644473600000ULL);
}
static void date_string(char out[16]) {
    SYSTEMTIME t;
    DateTime_GetSystemtime(dateEdit, &t);
    snprintf(out, 16, "%04u-%02u-%02u", t.wYear, t.wMonth, t.wDay);
}
static void today_beijing(void) {
    FILETIME f;
    ULARGE_INTEGER u;
    SYSTEMTIME t;
    GetSystemTimeAsFileTime(&f);
    u.LowPart = f.dwLowDateTime;
    u.HighPart = f.dwHighDateTime;
    u.QuadPart += 8ULL * 3600 * 10000000;
    f.dwLowDateTime = u.LowPart;
    f.dwHighDateTime = u.HighPart;
    FileTimeToSystemTime(&f, &t);
    DateTime_SetSystemtime(dateEdit, GDT_VALID, &t);
}
static const char *state(const char *s) {
    if (!strcmp(s, "reserved") || !strcmp(s, "confirmed"))
        return "已预约";
    if (!strcmp(s, "in_use"))
        return "使用中";
    if (!strcmp(s, "completed"))
        return "已完成";
    if (!strcmp(s, "cancelled"))
        return "已取消";
    if (!strcmp(s, "no_show"))
        return "未到场";
    if (!strcmp(s, "resolved"))
        return "已修复";
    if (!strcmp(s, "reported") || !strcmp(s, "open"))
        return "待处理";
    return s;
}
static HWND control(const wchar_t *cls, const wchar_t *text, DWORD style, int id, int x, int y, int w,
                    int h) {
    HWND c = CreateWindowExW(0, cls, text, WS_CHILD | WS_VISIBLE | style, x, y, w, h, window,
                             (HMENU)(INT_PTR)id, GetModuleHandleW(NULL), NULL);
    SendMessageW(c, WM_SETFONT, (WPARAM)font, TRUE);
    return c;
}
static void add_cell(int row, int col, const char *s) {
    wchar_t *w = wide(s);
    LVITEMW item = {0};
    item.mask = LVIF_TEXT;
    item.iItem = row;
    item.iSubItem = col;
    item.pszText = w;
    if (col == 0)
        SendMessageW(list, LVM_INSERTITEMW, 0, (LPARAM)&item);
    else
        SendMessageW(list, LVM_SETITEMTEXTW, (WPARAM)row, (LPARAM)&item);
    free(w);
}
static void column(int i, const wchar_t *title, int width) {
    LVCOLUMNW c = {0};
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = (LPWSTR)title;
    c.cx = width;
    SendMessageW(list, LVM_INSERTCOLUMNW, (WPARAM)i, (LPARAM)&c);
}
static void render(void) {
    const cJSON *r;
    char b[1024], d[16];
    int i = 0;
    date_string(d);
    SendMessageW(list, WM_SETREDRAW, FALSE, 0);
    ListView_DeleteAllItems(list);
    while (ListView_DeleteColumn(list, 0)) {
    }
    listedCount = 0;
    if (page < 2) {
        column(0, L"设备", 75);
        column(1, L"日期（北京）", 120);
        column(2, L"时段", 100);
        column(3, L"状态", 95);
        column(4, L"预约人", 200);
        cJSON_ArrayForEach(r, model.bookings) {
            if (page == 1 && strcmp(lab_str(r, "user_id"), client.session.uid))
                continue;
            if (page == 0 && strcmp(lab_str(r, "reservation_date"), d))
                continue;
            if (i >= 100000)
                break;
            listed[i] = (cJSON *)r;
            snprintf(b, sizeof(b), "%02d", lab_int(r, "device_number"));
            add_cell(i, 0, b);
            add_cell(i, 1, lab_str(r, "reservation_date"));
            snprintf(b, sizeof(b), "%02d:00–%02d:00", lab_int(r, "start_hour"), lab_int(r, "start_hour") + 1);
            add_cell(i, 2, b);
            add_cell(i, 3, state(lab_str(r, "status")));
            const char *owner = lab_str(r, "user_id");
            const cJSON *p;
            cJSON_ArrayForEach(p, model.profiles) if (!strcmp(lab_str(p, "id"), owner)) {
                owner = lab_str(p, "display_name");
                break;
            }
            add_cell(i, 4, owner);
            i++;
        }
    } else if (page == 2) {
        column(0, L"设备", 65);
        column(1, L"状态", 85);
        column(2, L"故障说明", 290);
        column(3, L"处理记录", 230);
        column(4, L"报告时间", 190);
        cJSON_ArrayForEach(r, model.issues) {
            if (i >= 100000)
                break;
            listed[i] = (cJSON *)r;
            snprintf(b, sizeof(b), "%02d", lab_int(r, "device_number"));
            add_cell(i, 0, b);
            add_cell(i, 1, state(lab_str(r, "status")));
            add_cell(i, 2, lab_str(r, "description"));
            add_cell(i, 3, lab_str(r, "resolution_note"));
            add_cell(i, 4, lab_str(r, "created_at"));
            i++;
        }
    } else {
        column(0, L"姓名", 180);
        column(1, L"学号 / 登录名", 220);
        column(2, L"角色", 90);
        column(3, L"账号状态", 110);
        cJSON_ArrayForEach(r, model.profiles) {
            if (i >= 100000)
                break;
            listed[i] = (cJSON *)r;
            add_cell(i, 0, lab_str(r, "display_name"));
            add_cell(i, 1, lab_str(r, "login_id"));
            add_cell(i, 2, lab_str(r, "role"));
            add_cell(i, 3, cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(r, "active")) ? "已停用" : "可用");
            i++;
        }
    }
    listedCount = i;
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, NULL, TRUE);
    snprintf(b, sizeof(b), "%s  ·  %s  ·  原生 C %s", client.session.name, admin() ? "管理员" : "学生",
             LAB_NATIVE_VERSION);
    set_utf8(whoText, b);
    for (i = 0; i < 40; i++)
        InvalidateRect(deviceButtons[i], NULL, TRUE);
}
static void visibility(void) {
    HWND auth[] = {identity, password, nameEdit, inviteEdit, loginButton, registerButton, loginInfo};
    for (size_t i = 0; i < sizeof(auth) / sizeof(auth[0]); i++)
        ShowWindow(auth[i], logged ? SW_HIDE : SW_SHOW);
    for (int i = 0; i < 4; i++)
        ShowWindow(labels[i], logged ? SW_HIDE : SW_SHOW);
    ShowWindow(tabs, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(list, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(whoText, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(dateEdit, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(startEdit, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(endEdit, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(noteEdit, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(helpText, logged ? SW_SHOW : SW_HIDE);
    for (int i = 0; i < 40; i++)
        ShowWindow(deviceButtons[i], logged && page == 0 ? SW_SHOW : SW_HIDE);
    for (int i = 0; i < 15; i++) {
        int show = logged;
        int id = actionIds[i];
        if (id == ID_AGENT)
            show = logged && (page == 0 || (page == 3 && admin()));
        if (id == ID_BOOK || id == ID_SUGGEST)
            show = logged && page == 0;
        if (id == ID_ATTEND || id == ID_CANCEL)
            show = logged && (page == 1 || (page == 0 && admin()));
        if (id == ID_REPORT)
            show = logged && page == 2;
        if (id == ID_REPAIR || id == ID_ARCHIVE)
            show = logged && page == 2 && admin();
        if (id == ID_DEACTIVATE || id == ID_INVITE || id == ID_METRICS)
            show = logged && page == 3 && admin();
        ShowWindow(actions[i], show ? SW_SHOW : SW_HIDE);
        EnableWindow(actions[i], !busy && (id != ID_UPDATE || !checkingUpdate));
    }
    EnableWindow(loginButton, !busy);
    EnableWindow(registerButton, !busy);
    EnableWindow(tabs, !busy);
    if (logged) {
        RECT rc;
        GetClientRect(window, &rc);
        int top = page == 0 ? 315 : 145;
        MoveWindow(list, 24, top, rc.right - 48, rc.bottom - top - 135, TRUE);
    }
    SetWindowTextW(
        helpText, page == 0 ? L"选择日期、开始/结束小时和设备；多小时预约逐时提交。设备颜色对应所选整个时段。"
                  : page == 1
                      ? L"选择自己的预约，再点击签到 / 签退。签到仅限预约时段（北京时间），签退需先签到。"
                  : page == 2 ? L"在说明框输入故障/维修说明。报告故障使用预约页中选中的第一台设备。"
                              : L"管理操作由云端验证权限。修改本人资料：说明框输入“姓名|学号或邮箱”。");
}
static cJSON *selected_row(void) {
    int i = ListView_GetNextItem(list, -1, LVNI_SELECTED);
    return i >= 0 && i < listedCount ? listed[i] : NULL;
}
static void parameter(cJSON *p, const char *key, const char *s) { cJSON_AddStringToObject(p, key, s); }
static DWORD WINAPI worker(void *arg) {
    Job *j = arg;
    cJSON *p = NULL, *r = NULL;
    int success = 0, committed = 0;
    if (j->kind == JOB_UPDATE) {
        int status = 0;
        char *raw = lab_win_http(NULL, LAB_SITE_HOST, "GET", LAB_SITE_BASE "native-desktop.json", "", "", "",
                                 &status, j->client.error, sizeof(j->client.error));
        if (raw) {
            r = cJSON_Parse(raw);
            free(raw);
        }
        if (status == 200 && r && lab_semver_newer(lab_str(r, "version"), LAB_NATIVE_VERSION)) {
            snprintf(j->message, sizeof(j->message), "发现原生桌面新版本 %s。是否打开官网下载页？",
                     lab_str(r, "version"));
            j->ok = 1;
        }
        cJSON_Delete(r);
        goto post;
    }
    if (j->kind == JOB_LOGIN)
        success = lab_login(&j->client, j->identity, j->password);
    else if (j->kind == JOB_REGISTER)
        success = lab_register(&j->client, j->name, j->identity, j->password, j->invite);
    else if (j->kind == JOB_REFRESH)
        success = 1;
    else if (j->kind == JOB_BOOK) {
        const char *keys[9];
        for (int i = 0; i < 9; i++)
            keys[i] = j->keys[i];
        success = lab_reserve_range(&j->client, j->date, j->start, j->end, j->devices, (size_t)j->count, keys,
                                    &committed);
        if (!success)
            snprintf(j->message, sizeof(j->message),
                     "已确认成功 %d 个小时；本次未能完成全部提交，请刷新核实剩余小时。%s", committed,
                     j->client.error);
    } else if (j->kind == JOB_ATTEND)
        success = lab_attend(&j->client, j->row, j->action, now_ms());
    else if (j->kind == JOB_AGENT) {
        p = cJSON_CreateObject();
        cJSON *input = cJSON_CreateObject(), *devices = cJSON_CreateArray();
        parameter(p, "mode", j->action ? "admin" : "booking");
        parameter(p, "language", "zh");
        parameter(p, "message", *j->note ? j->note : "请分析当前情况。");
        parameter(input, "date", j->date);
        parameter(input, "startDate", j->date);
        parameter(input, "endDate", j->date);
        cJSON_AddNumberToObject(input, "preferredHour", j->start);
        cJSON_AddNumberToObject(input, "requiredDeviceCount", j->count ? j->count : 1);
        for (int i = 0; i < j->count; i++)
            cJSON_AddItemToArray(devices, cJSON_CreateNumber(j->devices[i]));
        cJSON_AddItemToObject(input, "preferredDevices", devices);
        cJSON_AddItemToObject(p, "input", input);
        r = lab_request(&j->client, "POST", "/functions/v1/deepseek-booking-agent", p);
        cJSON_Delete(p);
        success = r != NULL;
        if (success)
            snprintf(j->message, sizeof(j->message), "%s\n\n如需预约，请根据建议选择设备和时段，再点击预约。",
                     *lab_str(r, "message") ? lab_str(r, "message")
                                            : "分析完成，请使用推荐空闲时段查看可预约方案。");
        cJSON_Delete(r);
    } else {
        const char *rpc = NULL;
        p = cJSON_CreateObject();
        switch (j->kind) {
        case JOB_CANCEL:
            rpc = "v2_cancel_reservation";
            parameter(p, "p_reservation_id", lab_str(j->row, "id"));
            break;
        case JOB_REPORT:
            rpc = "report_device_fault";
            cJSON_AddNumberToObject(p, "p_device_number", j->devices[0]);
            parameter(p, "p_description", j->note);
            break;
        case JOB_REPAIR:
            rpc = "v2_complete_fault_repair";
            parameter(p, "p_issue_id", lab_str(j->row, "id"));
            parameter(p, "p_note", j->note);
            break;
        case JOB_ARCHIVE:
            rpc = "archive_resolved_issue";
            parameter(p, "p_issue_id", lab_str(j->row, "id"));
            break;
        case JOB_DEACTIVATE:
            rpc = "deactivate_student";
            parameter(p, "target_user", lab_str(j->row, "id"));
            break;
        case JOB_INVITE:
            rpc = "create_admin_invite_for_days";
            cJSON_AddNumberToObject(p, "p_valid_days", 7);
            break;
        case JOB_METRICS:
            rpc = "v2_admin_metrics";
            parameter(p, "p_start_date", j->date);
            parameter(p, "p_end_date", j->date);
            break;
        case JOB_PROFILE: {
            char *sep = strchr(j->note, '|');
            if (!sep) {
                snprintf(j->client.error, sizeof(j->client.error), "请在说明框输入：姓名|学号或邮箱");
                break;
            }
            *sep++ = 0;
            rpc = "update_profile_identity";
            parameter(p, "target_user", j->client.session.uid);
            parameter(p, "next_display_name", j->note);
            parameter(p, "next_login_id", sep);
            break;
        }
        case JOB_SUGGEST:
            rpc = "v2_schedule_candidates";
            parameter(p, "p_date", j->date);
            cJSON_AddNumberToObject(p, "p_preferred_hour", j->start);
            cJSON_AddNumberToObject(p, "p_required_device_count", j->count ? j->count : 1);
            cJSON_AddItemToObject(p, "p_preferred_devices", cJSON_CreateArray());
            cJSON_AddNumberToObject(p, "p_limit", 3);
            break;
        }
        if (rpc)
            r = lab_rpc(&j->client, rpc, p);
        cJSON_Delete(p);
        success = r != NULL;
        if (success && j->kind == JOB_INVITE) {
            const cJSON *v = cJSON_IsArray(r) ? cJSON_GetArrayItem(r, 0) : r;
            snprintf(j->message, sizeof(j->message),
                     "管理员邀请码：%s\n有效期至：%s\n请妥善保管，仅分享给授权管理员。",
                     lab_str(v, "invite_code"), lab_str(v, "expires_at"));
        }
        if (success && j->kind == JOB_METRICS) {
            snprintf(j->message, sizeof(j->message),
                     "%s "
                     "当日统计\n可用设备小时：%d\n预约设备小时：%d\n已签到设备小时：%d\n使用中/"
                     "已完成但缺少签到记录：%d",
                     j->date, lab_int(r, "capacity_device_hours"), lab_int(r, "booked_device_hours"),
                     lab_int(r, "checked_in_device_hours"),
                     lab_int(cJSON_GetObjectItemCaseSensitive(r, "data_completeness"),
                             "active_or_completed_missing_check_in"));
        }
        if (success && j->kind == JOB_SUGGEST) {
            const cJSON *v;
            size_t used = 0;
            snprintf(j->message, sizeof(j->message), "可预约方案（请选择设备后确认预约）：\n");
            used = strlen(j->message);
            cJSON_ArrayForEach(v, r) {
                int n = snprintf(j->message + used, sizeof(j->message) - used, "%s %02d:00，设备：",
                                 lab_str(v, "reservation_date"), lab_int(v, "start_hour"));
                if (n < 0 || (size_t)n >= sizeof(j->message) - used)
                    break;
                used += (size_t)n;
                const cJSON *device;
                cJSON_ArrayForEach(device, cJSON_GetObjectItemCaseSensitive(v, "device_numbers")) {
                    n = snprintf(j->message + used, sizeof(j->message) - used, "%02d ", device->valueint);
                    if (n < 0 || (size_t)n >= sizeof(j->message) - used)
                        break;
                    used += (size_t)n;
                }
                if (used + 2 < sizeof(j->message)) {
                    j->message[used++] = '\n';
                    j->message[used] = 0;
                }
            }
            if (cJSON_GetArraySize(r) == 0)
                snprintf(j->message, sizeof(j->message), "当前条件下没有可预约的方案。");
        }
        cJSON_Delete(r);
    }
    j->ok = success;
    if (!success && !*j->message)
        snprintf(j->message, sizeof(j->message), "%s", j->client.error);
    if (*j->client.session.access) {
        j->loaded = lab_load_model(&j->client, &j->model);
        if (success && !j->loaded) {
            snprintf(j->message, sizeof(j->message), "操作已完成，但刷新失败：%s。请刷新核实后再操作。",
                     j->client.error);
        }
    }
post:
    SecureZeroMemory(j->password, sizeof(j->password));
    if (!PostMessageW(j->target, RESULT_MESSAGE, 0, (LPARAM)j)) {
        lab_model_free(&j->model);
        lab_session_clear(&j->client.session);
        cJSON_Delete(j->row);
        free(j);
    }
    return 0;
}
static void start_job(Job *j) {
    HANDLE thread;
    j->target = window;
    j->client = client;
    if (j->kind == JOB_UPDATE)
        checkingUpdate = 1;
    else
        busy = 1;
    visibility();
    if (j->kind != JOB_UPDATE)
        SetWindowTextW(statusText, L"正在连接云端，请稍候…");
    thread = CreateThread(NULL, 0, worker, j, 0, NULL);
    if (thread)
        CloseHandle(thread);
    else {
        if (j->kind == JOB_UPDATE)
            checkingUpdate = 0;
        else
            busy = 0;
        visibility();
        cJSON_Delete(j->row);
        lab_session_clear(&j->client.session);
        free(j);
        tell("无法创建请求线程，请重试。", MB_ICONERROR);
    }
}
static void command(int id) {
    Job *j;
    cJSON *row;
    char confirmation[512];
    if (busy)
        return;
    if (id == ID_LOGOUT) {
        lab_session_clear(&client.session);
        lab_model_free(&model);
        logged = 0;
        listedCount = 0;
        ListView_DeleteAllItems(list);
        SetWindowTextW(password, L"");
        visibility();
        return;
    }
    j = calloc(1, sizeof(*j));
    if (!j)
        return;
    date_string(j->date);
    j->start = 8 + (int)SendMessageW(startEdit, CB_GETCURSEL, 0, 0);
    j->end = 9 + (int)SendMessageW(endEdit, CB_GETCURSEL, 0, 0);
    get_utf8(noteEdit, j->note, sizeof(j->note));
    for (int i = 0; i < 40; i++)
        if (selected[i])
            j->devices[j->count++] = i + 1;
    switch (id) {
    case ID_LOGIN:
    case ID_REGISTER:
        get_utf8(identity, j->identity, sizeof(j->identity));
        get_utf8(password, j->password, sizeof(j->password));
        get_utf8(nameEdit, j->name, sizeof(j->name));
        get_utf8(inviteEdit, j->invite, sizeof(j->invite));
        j->kind = id == ID_LOGIN ? JOB_LOGIN : JOB_REGISTER;
        if (!*j->identity || !*j->password) {
            tell("请填写学号 / 邮箱与密码。", MB_ICONINFORMATION);
            goto abort;
        }
        break;
    case ID_REFRESH:
        j->kind = JOB_REFRESH;
        break;
    case ID_BOOK:
        j->kind = JOB_BOOK;
        if (!j->count || j->start >= j->end) {
            tell("请选择设备和有效时段。", MB_ICONINFORMATION);
            goto abort;
        }
        for (int i = 0; i < 9; i++) {
            GUID g;
            if (FAILED(CoCreateGuid(&g))) {
                tell("生成请求编号失败。", MB_ICONERROR);
                goto abort;
            }
            snprintf(j->keys[i], 64, "%08lx-%04x-%04x-%02x%02x-%02x%02x%02x%02x%02x%02x",
                     (unsigned long)g.Data1, g.Data2, g.Data3, g.Data4[0], g.Data4[1], g.Data4[2], g.Data4[3],
                     g.Data4[4], g.Data4[5], g.Data4[6], g.Data4[7]);
        }
        snprintf(
            confirmation, sizeof(confirmation),
            "预约 %s %02d:00–%02d:00，共 %d 台设备？多小时会逐时提交；若后续冲突，会保留先前成功的小时。",
            j->date, j->start, j->end, j->count);
        goto confirm;
    case ID_ATTEND:
    case ID_CANCEL:
    case ID_REPAIR:
    case ID_ARCHIVE:
    case ID_DEACTIVATE:
        row = selected_row();
        if (!row) {
            tell("请先选择列表中的一行。", MB_ICONINFORMATION);
            goto abort;
        }
        j->row = cJSON_Duplicate(row, 1);
        if (id == ID_ATTEND) {
            j->kind = JOB_ATTEND;
            j->action = lab_attendance_action(row, now_ms());
            if (strcmp(lab_str(row, "user_id"), client.session.uid) || !j->action) {
                tell("仅可操作自己的预约：在预约小时内签到，使用中可签退。请刷新核实当前状态。",
                     MB_ICONINFORMATION);
                goto abort;
            }
            snprintf(confirmation, sizeof(confirmation), "确认%s设备 %02d（%s %02d:00）的预约？",
                     j->action == 1 ? "签到" : "签退", lab_int(row, "device_number"),
                     lab_str(row, "reservation_date"), lab_int(row, "start_hour"));
        } else {
            j->kind = id == ID_CANCEL    ? JOB_CANCEL
                      : id == ID_REPAIR  ? JOB_REPAIR
                      : id == ID_ARCHIVE ? JOB_ARCHIVE
                                         : JOB_DEACTIVATE;
            snprintf(confirmation, sizeof(confirmation), "确认%s所选记录？",
                     id == ID_CANCEL    ? "取消预约"
                     : id == ID_REPAIR  ? "完成维修"
                     : id == ID_ARCHIVE ? "归档故障"
                                        : "停用学生账号");
        }
        goto confirm;
    case ID_REPORT:
        j->kind = JOB_REPORT;
        if (!j->count) {
            tell("请先到设备预约页选中一台设备。", MB_ICONINFORMATION);
            goto abort;
        }
        snprintf(confirmation, sizeof(confirmation), "确认报告设备 %02d 故障？", j->devices[0]);
        goto confirm;
    case ID_INVITE:
        j->kind = JOB_INVITE;
        break;
    case ID_METRICS:
        j->kind = JOB_METRICS;
        break;
    case ID_PROFILE:
        j->kind = JOB_PROFILE;
        snprintf(confirmation, sizeof(confirmation), "确认按说明框内容修改本人姓名和登录名？");
        goto confirm;
    case ID_SUGGEST:
        j->kind = JOB_SUGGEST;
        break;
    case ID_AGENT:
        j->kind = JOB_AGENT;
        j->action = page == 3;
        break;
    case ID_UPDATE:
        if (checkingUpdate)
            goto abort;
        j->kind = JOB_UPDATE;
        break;
    default:
        goto abort;
    }
    start_job(j);
    return;
confirm:
    {
        wchar_t *w = wide(confirmation);
        int answer = MessageBoxW(window, w, L"请确认", MB_YESNO | MB_ICONQUESTION | MB_DEFBUTTON2);
        free(w);
        if (answer == IDYES) {
            start_job(j);
            return;
        }
    }
abort:
    SecureZeroMemory(j->password, sizeof(j->password));
    cJSON_Delete(j->row);
    free(j);
}
static void layout(void) {
    RECT r;
    GetClientRect(window, &r);
    int w = r.right;
    MoveWindow(statusText, 24, r.bottom - 32, w - 48, 24, TRUE);
    MoveWindow(helpText, 24, 91, w - 48, 28, TRUE);
    MoveWindow(noteEdit, 24, r.bottom - 115, w - 48, 28, TRUE);
    for (int i = 0; i < 15; i++) {
        int x = 24, y = r.bottom - 75;
        switch (actionIds[i]) {
        case ID_LOGOUT:
            x = w - 120;
            break;
        case ID_BOOK:
        case ID_ATTEND:
        case ID_REPORT:
        case ID_DEACTIVATE:
            x = 125;
            break;
        case ID_CANCEL:
        case ID_REPAIR:
        case ID_INVITE:
            x = 270;
            break;
        case ID_ARCHIVE:
        case ID_METRICS:
        case ID_SUGGEST:
            x = 410;
            break;
        case ID_UPDATE:
            x = w - 250;
            break;
        case ID_AGENT:
            x = 555;
            break;
        case ID_PROFILE:
            x = w - 405;
            break;
        }
        MoveWindow(actions[i], x, y, 135, 30, TRUE);
    }
    visibility();
}
static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        window = h;
        LOGFONTW lf = {0};
        lf.lfHeight = -17;
        wcscpy(lf.lfFaceName, L"Microsoft YaHei UI");
        font = CreateFontIndirectW(&lf);
        lf.lfWeight = FW_BOLD;
        bold = CreateFontIndirectW(&lf);
        background = CreateSolidBrush(BG);
        control(L"STATIC", L"实验室预约 · 原生 C 桌面", 0, 0, 24, 16, 460, 30);
        whoText = control(L"STATIC", L"", 0, 0, 500, 20, 570, 26);
        const wchar_t *texts[] = {L"学号 / 邮箱", L"密码", L"姓名（注册）", L"管理员邀请码（可选）"};
        for (int i = 0; i < 4; i++)
            labels[i] = control(L"STATIC", texts[i], 0, 0, 155, 120 + i * 65, 280, 24);
        identity = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 120, 370, 30);
        password = control(L"EDIT", L"", WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 185,
                           370, 30);
        nameEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 250, 370, 30);
        inviteEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 315, 370, 30);
        loginButton = control(L"BUTTON", L"登录", BS_DEFPUSHBUTTON | WS_TABSTOP, ID_LOGIN, 450, 380, 175, 38);
        registerButton = control(L"BUTTON", L"注册", WS_TABSTOP, ID_REGISTER, 645, 380, 175, 38);
        loginInfo = control(L"STATIC",
                            L"使用与网页相同的账号。界面、签到确认与网络请求全部由原生 C "
                            L"实现。\n学生使用学号；管理员使用邮箱。密码与令牌仅保留于进程内存。",
                            0, 0, 155, 460, 800, 80);
        tabs = control(WC_TABCONTROLW, L"", WS_TABSTOP, ID_TAB, 24, 54, 440, 32);
        const wchar_t *tabNames[] = {L"设备预约", L"我的预约", L"故障管理", L"账号管理"};
        for (int i = 0; i < 4; i++) {
            TCITEMW t = {0};
            t.mask = TCIF_TEXT;
            t.pszText = (LPWSTR)tabNames[i];
            SendMessageW(tabs, TCM_INSERTITEMW, (WPARAM)i, (LPARAM)&t);
        }
        dateEdit =
            control(DATETIMEPICK_CLASSW, L"", DTS_SHORTDATEFORMAT | WS_TABSTOP, ID_DATE, 490, 55, 180, 30);
        DateTime_SetFormat(dateEdit, L"yyyy-MM-dd");
        today_beijing();
        startEdit = control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ID_START, 685, 55,
                            130, 300);
        endEdit =
            control(L"COMBOBOX", L"", CBS_DROPDOWNLIST | WS_VSCROLL | WS_TABSTOP, ID_END, 830, 55, 130, 300);
        for (int i = 0; i < 9; i++) {
            wchar_t b[50];
            swprintf(b, 50, L"开始 %02d:00", i + 8);
            SendMessageW(startEdit, CB_ADDSTRING, 0, (LPARAM)b);
            swprintf(b, 50, L"结束 %02d:00", i + 9);
            SendMessageW(endEdit, CB_ADDSTRING, 0, (LPARAM)b);
        }
        SendMessageW(startEdit, CB_SETCURSEL, 0, 0);
        SendMessageW(endEdit, CB_SETCURSEL, 0, 0);
        helpText = control(L"STATIC", L"", 0, 0, 24, 91, 1040, 28);
        for (int i = 0; i < 40; i++)
            deviceButtons[i] = control(L"BUTTON", L"", BS_OWNERDRAW | WS_TABSTOP, ID_FIRST_DEVICE + i,
                                       24 + (i % 10) * 104, 127 + (i / 10) * 44, 95, 35);
        list = control(WC_LISTVIEWW, L"", LVS_REPORT | LVS_SINGLESEL | WS_BORDER | WS_TABSTOP, ID_LIST, 24,
                       315, 1040, 230);
        ListView_SetExtendedListViewStyle(list,
                                          LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER | LVS_EX_GRIDLINES);
        noteEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 24, 560, 1040, 28);
        SendMessageW(noteEdit, EM_SETCUEBANNER, TRUE,
                     (LPARAM)L"故障 / 维修说明；修改本人资料时输入 姓名|登录名");
        for (int i = 0; i < 15; i++)
            actions[i] = control(L"BUTTON", actionNames[i], WS_TABSTOP, actionIds[i], 24, 600, 135, 30);
        statusText = control(L"STATIC", L"欢迎使用。所有预约时段按北京时间计算。", 0, 0, 24, 650, 1040, 24);
        SetTimer(h, 1, 30000, NULL);
        layout();
        Job *j = calloc(1, sizeof(*j));
        if (j) {
            j->kind = JOB_UPDATE;
            start_job(j);
        }
        return 0;
    }
    case WM_SIZE:
        if (list)
            layout();
        return 0;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lp)->ptMinTrackSize.x = 1120;
        ((MINMAXINFO *)lp)->ptMinTrackSize.y = 740;
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(wp);
        if (id >= ID_FIRST_DEVICE && id < ID_FIRST_DEVICE + 40) {
            int i = id - ID_FIRST_DEVICE;
            selected[i] = !selected[i];
            InvalidateRect(deviceButtons[i], NULL, TRUE);
        } else if (id == ID_START || id == ID_END) {
            if (HIWORD(wp) == CBN_SELCHANGE)
                render();
        } else if (HIWORD(wp) == BN_CLICKED)
            command(id);
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR *n = (NMHDR *)lp;
        if (n->idFrom == ID_TAB && n->code == TCN_SELCHANGE) {
            page = TabCtrl_GetCurSel(tabs);
            if (page == 3 && !admin()) {
                page = 1;
                TabCtrl_SetCurSel(tabs, page);
                tell("账号管理需要管理员权限。", MB_ICONINFORMATION);
            }
            render();
            visibility();
        }
        if (n->idFrom == ID_DATE && n->code == DTN_DATETIMECHANGE && logged)
            render();
        return 0;
    }
    case RESULT_MESSAGE: {
        Job *j = (Job *)lp;
        if (j->kind == JOB_UPDATE) {
            checkingUpdate = 0;
            if (j->ok) {
                wchar_t *w = wide(j->message);
                if (MessageBoxW(h, w, L"桌面版本更新", MB_YESNO | MB_ICONINFORMATION) == IDYES)
                    ShellExecuteW(h, L"open",
                                  L"https://alexis-zhang0503.github.io/intelligent-lab-booking-system/"
                                  L"downloads/LabReservation-Windows-x64.zip",
                                  NULL, NULL, SW_SHOWNORMAL);
                free(w);
            }
            if (!busy)
                SetWindowTextW(statusText, L"原生 C 3.0.0 · 云端数据每 30 秒刷新");
        } else {
            busy = 0;
            client = j->client;
            if (j->loaded) {
                lab_model_free(&model);
                model = j->model;
                memset(&j->model, 0, sizeof(j->model));
                logged = 1;
                if (j->kind == JOB_LOGIN || j->kind == JOB_REGISTER) {
                    page = 0;
                    TabCtrl_SetCurSel(tabs, 0);
                    memset(selected, 0, sizeof(selected));
                }
                SetWindowTextW(password, L"");
                SetWindowTextW(inviteEdit, L"");
                render();
            }
            if (!*client.session.access) {
                logged = 0;
                lab_model_free(&model);
            }
            if (*j->message)
                tell(j->message, j->ok ? MB_ICONINFORMATION : MB_ICONERROR);
            else if (j->ok && j->kind != JOB_REFRESH && j->kind != JOB_LOGIN)
                tell("操作成功，已刷新云端数据。", MB_ICONINFORMATION);
            SetWindowTextW(statusText, j->loaded ? L"已同步云端数据 · 预约时段为北京时间 · 每 30 秒自动刷新"
                                                 : L"请求未完成，请查看提示并刷新核实。请勿反复提交。");
        }
        lab_model_free(&j->model);
        lab_session_clear(&j->client.session);
        cJSON_Delete(j->row);
        free(j);
        visibility();
        return 0;
    }
    case WM_TIMER:
        if (logged && !busy)
            command(ID_REFRESH);
        return 0;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT *)lp;
        int i = (int)d->CtlID - ID_FIRST_DEVICE;
        if (i < 0 || i >= 40)
            break;
        int occupied = 0, fault = 0;
        const cJSON *r;
        char date[16];
        date_string(date);
        int start = 8 + (int)SendMessageW(startEdit, CB_GETCURSEL, 0, 0),
            end = 9 + (int)SendMessageW(endEdit, CB_GETCURSEL, 0, 0);
        cJSON_ArrayForEach(r, model.devices) if (lab_int(r, "number") == i + 1 &&
                                                 strcmp(lab_str(r, "status"), "available")) fault = 1;
        cJSON_ArrayForEach(r, model.bookings) if (lab_int(r, "device_number") == i + 1 &&
                                                  !strcmp(lab_str(r, "reservation_date"), date) &&
                                                  lab_int(r, "start_hour") >= start &&
                                                  lab_int(r, "start_hour") < end &&
                                                  (!strcmp(lab_str(r, "status"), "reserved") ||
                                                   !strcmp(lab_str(r, "status"), "confirmed") ||
                                                   !strcmp(lab_str(r, "status"), "in_use"))) occupied = 1;
        COLORREF color = selected[i] ? INK
                         : fault     ? RGB(162, 69, 52)
                         : occupied  ? RGB(218, 190, 147)
                                     : RGB(230, 218, 197);
        HBRUSH brush = CreateSolidBrush(color);
        FillRect(d->hDC, &d->rcItem, brush);
        DeleteObject(brush);
        SetBkMode(d->hDC, TRANSPARENT);
        SetTextColor(d->hDC, selected[i] || fault ? RGB(255, 255, 255) : INK);
        SelectObject(d->hDC, bold);
        wchar_t b[40];
        swprintf(b, 40, L"%02d %ls", i + 1, fault ? L"故障" : occupied ? L"占用" : L"空闲");
        DrawTextW(d->hDC, b, -1, &d->rcItem, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
        if (d->itemState & ODS_FOCUS)
            DrawFocusRect(d->hDC, &d->rcItem);
        return TRUE;
    }
    case WM_CTLCOLORSTATIC:
        SetBkColor((HDC)wp, BG);
        SetTextColor((HDC)wp, INK);
        return (LRESULT)background;
    case WM_ERASEBKGND: {
        RECT r;
        GetClientRect(h, &r);
        FillRect((HDC)wp, &r, background);
        return 1;
    }
    case WM_DESTROY:
        KillTimer(h, 1);
        lab_model_free(&model);
        lab_session_clear(&client.session);
        PostQuitMessage(0);
        return 0;
    }
    return DefWindowProcW(h, msg, wp, lp);
}
int WINAPI wWinMain(HINSTANCE instance, HINSTANCE previous, LPWSTR commandLine, int show) {
    (void)previous;
    (void)commandLine;
    INITCOMMONCONTROLSEX ic = {sizeof(ic), ICC_LISTVIEW_CLASSES | ICC_DATE_CLASSES | ICC_TAB_CLASSES};
    InitCommonControlsEx(&ic);
    CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    client.transport = lab_win_http;
    WNDCLASSEXW wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = wndproc;
    wc.hInstance = instance;
    wc.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(1));
    wc.hIconSm = wc.hIcon;
    wc.hCursor = LoadCursorW(NULL, IDC_ARROW);
    wc.lpszClassName = L"NJUPTNativeCLabReservation";
    if (!RegisterClassExW(&wc))
        return 1;
    HWND h = CreateWindowExW(0, wc.lpszClassName, L"实验室预约 · 原生 C", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT,
                             CW_USEDEFAULT, 1140, 800, NULL, NULL, instance, NULL);
    if (!h)
        return 1;
    ShowWindow(h, show);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(h, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    DeleteObject(font);
    DeleteObject(bold);
    DeleteObject(background);
    CoUninitialize();
    return 0;
}
