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
#include "ui_theme.h"
#define RESULT_MESSAGE (WM_APP + 1)
#define BG RGB(247,247,245)
#define ACCENT RGB(92,99,214)
#define BORDER RGB(232,232,227)
#define MUTED RGB(104,107,115)
#define INK RGB(35,37,43)
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
static HFONT font, bold, headingFont, heroFont;
static HWND navigation[5],profileFault,usageList,agentReply;
static int scrollY,preview,rendering,selectedTable;
static char selectedId[80];
static wchar_t previewOutput[MAX_PATH];
static void layout(void);
static HBRUSH background;
static LabClient client;
static LabModel model;
static int busy, checkingUpdate, logged, page, selected[40];
static cJSON *listed[100000];
static int listedCount;
static const int actionIds[] = {ID_REFRESH, ID_LOGOUT, ID_BOOK,    ID_ATTEND,     ID_CANCEL,
                                ID_REPORT,  ID_REPAIR, ID_ARCHIVE, ID_DEACTIVATE, ID_INVITE,
                                ID_METRICS, ID_UPDATE, ID_PROFILE, ID_SUGGEST,    ID_AGENT};
static const wchar_t *actionNames[] = {L"刷新",         L"退出账号",     L"确认预约 →", L"签到 / 签退",
                                       L"取消预约",     L"报告设备故障",     L"完成维修",     L"归档故障",
                                       L"停用学生",     L"生成邀请码",   L"统计报表",     L"检查版本",
                                       L"修改本人资料", L"智能推荐", L"智能分析"};
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
    if (page == 1 && usageList) {
        if(col==0)SendMessageW(usageList,LVM_INSERTITEMW,0,(LPARAM)&item);
        else SendMessageW(usageList,LVM_SETITEMTEXTW,(WPARAM)row,(LPARAM)&item);
    }
    free(w);
}
static void column(int i, const wchar_t *title, int width) {
    LVCOLUMNW c = {0};
    c.mask = LVCF_TEXT | LVCF_WIDTH;
    c.pszText = (LPWSTR)title;
    c.cx = width;
    SendMessageW(list, LVM_INSERTCOLUMNW, (WPARAM)i, (LPARAM)&c);
    if(page==1&&usageList)SendMessageW(usageList,LVM_INSERTCOLUMNW,(WPARAM)i,(LPARAM)&c);
}
static void render(void) {
    const cJSON *r;
    char b[1024], d[16];
    int i = 0;
    date_string(d);
    rendering=1;
    ListView_DeleteAllItems(usageList);
    while(ListView_DeleteColumn(usageList,0)){}
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
        column(4, page==1?L"实际签到":L"预约人", 160);
        if(page==1){column(5,L"实际签退",160);column(6,L"实际使用",100);}
        cJSON_ArrayForEach(r, model.bookings) {
            if (page == 1 && strcmp(lab_str(r, "user_id"), client.session.uid))
                continue;
            if (page == 0 && (strcmp(lab_str(r, "reservation_date"), d)||(!admin()&&strcmp(lab_str(r,"user_id"),client.session.uid))))
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
            if(page==1){
                const cJSON *u=ui_usage(&model,r);char value[96];
                ui_time(lab_str(u,"checked_in_at"),value,sizeof(value));add_cell(i,4,value);
                ui_time(lab_str(u,"checked_out_at"),value,sizeof(value));add_cell(i,5,value);
                ui_duration(u,value,sizeof(value));add_cell(i,6,value);
            }else add_cell(i, 4, owner);
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
    } else if(page==3) {
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
    for(int row=0;row<i;row++)if(*selectedId&&!strcmp(lab_str(listed[row],"id"),selectedId)){
        ListView_SetItemState(list,row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
        if(page==1)ListView_SetItemState(usageList,row,LVIS_SELECTED|LVIS_FOCUSED,LVIS_SELECTED|LVIS_FOCUSED);
    }
    rendering=0;
    SendMessageW(list, WM_SETREDRAW, TRUE, 0);
    InvalidateRect(list, NULL, TRUE);
    snprintf(b, sizeof(b), "%s  ·  %s  ·  原生 C %s", client.session.name, admin() ? "管理员" : "学生",
             LAB_NATIVE_VERSION);
    set_utf8(whoText, b);
    for (i = 0; i < 40; i++)
        InvalidateRect(deviceButtons[i], NULL, TRUE);
    InvalidateRect(window,NULL,TRUE);
}
static void visibility(void) {
    HWND auth[] = {identity, password, nameEdit, inviteEdit, loginButton, registerButton, loginInfo};
    for (size_t i = 0; i < sizeof(auth) / sizeof(auth[0]); i++)
        ShowWindow(auth[i], logged ? SW_HIDE : SW_SHOW);
    for (int i = 0; i < 4; i++)
        ShowWindow(labels[i], logged ? SW_HIDE : SW_SHOW);
    ShowWindow(tabs,SW_HIDE);
    for(int i=0;i<5;i++)ShowWindow(navigation[i],logged&&((i!=2&&i!=3)||admin())?SW_SHOW:SW_HIDE);
    ShowWindow(profileFault,logged&&page==1?SW_SHOW:SW_HIDE);
    ShowWindow(usageList,logged&&page==1?SW_SHOW:SW_HIDE);
    ShowWindow(agentReply,logged&&page==4?SW_SHOW:SW_HIDE);
    ShowWindow(list, logged&&page!=4 ? SW_SHOW : SW_HIDE);
    ShowWindow(whoText, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(dateEdit, logged&&page==0 ? SW_SHOW : SW_HIDE);
    ShowWindow(startEdit, logged&&page==0 ? SW_SHOW : SW_HIDE);
    ShowWindow(endEdit, logged&&page==0 ? SW_SHOW : SW_HIDE);
    ShowWindow(noteEdit, logged ? SW_SHOW : SW_HIDE);
    ShowWindow(helpText, logged ? SW_SHOW : SW_HIDE);
    for (int i = 0; i < 40; i++)
        ShowWindow(deviceButtons[i], logged && page == 0 ? SW_SHOW : SW_HIDE);
    for (int i = 0; i < 15; i++) {
        int show = logged;
        int id = actionIds[i];
        if (id == ID_AGENT)
            show = logged && (page == 4 || (page == 3 && admin()));
        if (id == ID_PROFILE)show=logged&&page==1;
        if (id == ID_BOOK || id == ID_SUGGEST)
            show = logged && page == 0;
        if (id == ID_ATTEND || id == ID_CANCEL)
            show = logged && page == 1;
        if (id == ID_REPORT)
            show = logged && (page == 2 || page == 0);
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
    SetWindowTextW(
        helpText, page == 0 ? L"选择日期、开始/结束小时和设备；多小时预约逐时提交。设备颜色对应所选整个时段。"
                  : page == 1
                      ? L"选择自己的预约，再点击签到 / 签退。签到仅限预约时段（北京时间），签退需先签到。"
                  : page == 4 ? L"根据设备、日期和时段获取建议；确认后再预约。"
                  : page == 2 ? L"在说明框输入故障/维修说明。报告故障使用预约页中选中的第一台设备。"
                              : L"管理操作由云端验证权限。修改本人资料：说明框输入“姓名|学号或邮箱”。");
}
static cJSON *selected_row(void) {
    int i = ListView_GetNextItem(page==1&&selectedTable?usageList:list, -1, LVNI_SELECTED);
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
    if (busy || preview)
        return;
    if (id == ID_LOGOUT) {
        lab_session_clear(&client.session);
        lab_model_free(&model);
        logged = 0;
        scrollY=0;selectedId[0]=0;
        listedCount = 0;
        ListView_DeleteAllItems(list);
        SetWindowTextW(password, L"");
        layout();
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
static void place(HWND h,int x,int y,int w,int height){MoveWindow(h,x,y-scrollY,w,height,TRUE);}
static void layout(void) {
    RECT r;GetClientRect(window,&r);int w=r.right,x=UI_SIDEBAR+32,cw=w-x-32,rail=w-324,labw=cw-308;
    if(labw<420)labw=420;
    SCROLLINFO si={sizeof(si),SIF_RANGE|SIF_PAGE|SIF_POS,0,logged?(page==0?1160:page==1?1080:900):r.bottom,(UINT)r.bottom,scrollY,0};
    SetScrollInfo(window,SB_VERT,&si,TRUE);GetScrollInfo(window,SB_VERT,&si);scrollY=si.nPos;
    visibility();
    MoveWindow(whoText,20,r.bottom-170,188,64,TRUE);
    const int order[]={0,4,1,2,3};for(int k=0;k<5;k++)MoveWindow(navigation[order[k]],16,152+k*50,196,44,TRUE);
    int fx=w/2+36,fw=w-fx-70; if(fw>430)fw=430;
    for(int i=0;i<4;i++){MoveWindow(labels[i],fx,170+i*70,fw,22,TRUE);}
    HWND inputs[]={identity,password,nameEdit,inviteEdit};for(int i=0;i<4;i++)MoveWindow(inputs[i],fx,195+i*70,fw,36,TRUE);
    MoveWindow(loginButton,fx,488,fw,40,TRUE);MoveWindow(registerButton,fx,540,fw,40,TRUE);MoveWindow(loginInfo,fx,600,fw,64,TRUE);
    place(helpText,x,82,cw,28);
    place(dateEdit,x+labw-200,294,175,34);place(startEdit,rail+16,330,120,300);place(endEdit,rail+156,330,120,300);
    for(int i=0;i<40;i++){int cell=(labw-48-7*8-32)/8;place(deviceButtons[i],x+24+(i%8)*(cell+8)+(i%8>=4?32:0),390+(i/8)*80,cell,68);}
    place(list,x,page==0?880:page==1?800:160,page==0?labw:cw,page==1?220:page==0?220:480);
    place(usageList,x,438,cw,220);place(agentReply,x+24,170,cw-48,280);
    place(noteEdit,page==0?rail+22:x,page==0?866:page==1?1040:page==4?490:666,page==0?246:cw,36);
    place(profileFault,x+470,384,150,38);
    int slot=0;
    for(int i=0;i<15;i++){
      int id=actionIds[i];
      if(id==ID_LOGOUT||id==ID_UPDATE||id==ID_REFRESH){int sy=id==ID_REFRESH?r.bottom-104:id==ID_UPDATE?r.bottom-68:r.bottom-36;MoveWindow(actions[i],20,sy,188,30,TRUE);continue;}
      if(!IsWindowVisible(actions[i]))continue;
      if(page==0){int sy=id==ID_BOOK?438:id==ID_SUGGEST?490:814;place(actions[i],rail+22,sy,246,40);}
      else{int sy=page==1?384:page==4?550:720;place(actions[i],x+slot*150,sy,140,38);++slot;}
    }
    MoveWindow(statusText,x,r.bottom-26,cw,22,TRUE);InvalidateRect(window,NULL,TRUE);
}
static void box(HDC dc,int x,int y,int w,int h,COLORREF fill){HBRUSH b=CreateSolidBrush(fill);HPEN pen=CreatePen(PS_SOLID,1,BORDER);HGDIOBJ ob=SelectObject(dc,b),op=SelectObject(dc,pen);RoundRect(dc,x,y-scrollY,x+w,y+h-scrollY,32,32);SelectObject(dc,ob);SelectObject(dc,op);DeleteObject(b);DeleteObject(pen);}
static void text(HDC dc,int x,int y,int w,int h,const char *value,HFONT f,COLORREF color){RECT r={x,y-scrollY,x+w,y+h-scrollY};wchar_t *s=wide(value);SelectObject(dc,f);SetBkMode(dc,TRANSPARENT);SetTextColor(dc,color);DrawTextW(dc,s,-1,&r,DT_LEFT|DT_WORDBREAK|DT_NOPREFIX);free(s);}
static void paint_workspace(HDC dc){
    RECT r;GetClientRect(window,&r);FillRect(dc,&r,background);int w=r.right,x=UI_SIDEBAR+32,cw=w-x-32,rail=w-324,labw=cw-308;
    if(!logged){box(dc,w/2,100,w/2-40,620,RGB(255,255,255));text(dc,64,180,w/2-100,40,"NJUPT · 实验室预约",bold,MUTED);text(dc,64,245,w/2-100,100,"把时间留给探索",heroFont,INK);text(dc,64,370,w/2-100,80,"选定设备，约好时间。\n设备、时段和你的记录，都在这里。",font,MUTED);text(dc,w/2+36,120,400,40,"欢迎回来",headingFont,INK);return;}
    RECT side={0,0,UI_SIDEBAR,r.bottom};FillRect(dc,&side,(HBRUSH)GetStockObject(WHITE_BRUSH));
    /* Sidebar stays anchored while the workspace scrolls. */
    int old=scrollY;scrollY=0;DrawIconEx(dc,20,26,LoadIconW(GetModuleHandleW(NULL),MAKEINTRESOURCEW(1)),28,28,0,NULL,DI_NORMAL);text(dc,58,30,160,40,"NJUPT 实验室预约",bold,INK);scrollY=old;
    text(dc,x,22,cw,20,"NJUPT · 实验室预约",font,MUTED);text(dc,x,48,cw,40,ui_page_title(page),headingFont,INK);
    if(page==0){
      text(dc,x,208,cw,56,"把时间留给探索",heroFont,INK);box(dc,rail,112,292,130,RGB(252,252,251));text(dc,rail+22,132,248,28,"预约流程 · 三步完成",bold,INK);text(dc,rail+22,180,248,46,"01 选择设备   02 设置时间\n03 确认预约",font,MUTED);
      box(dc,x,272,labw,530,RGB(255,255,255));text(dc,x+24,296,250,28,"实验室平面图",bold,INK);text(dc,x+24,335,labw-48,28,"* 空闲    已预约    使用中    ! 故障    已选择",font,MUTED);
      box(dc,rail,272,292,286,RGB(255,255,255));text(dc,rail+22,294,248,28,"预约时间",bold,INK);
      int n=0;for(int i=0;i<40;i++)n+=selected[i];char selection[96];snprintf(selection,sizeof(selection),n?"已选择 %d 台设备":"尚未选择设备",n);box(dc,rail+22,382,246,38,RGB(239,240,255));text(dc,rail+34,391,218,24,selection,font,INK);int faulty=0;const cJSON *dv;cJSON_ArrayForEach(dv,model.devices)if(strcmp(lab_str(dv,"status"),"available"))++faulty;char count[100];snprintf(count,sizeof(count),"%d / 40\n台设备可预约",40-faulty);box(dc,rail,584,292,190,RGB(255,255,255));text(dc,rail+22,604,246,24,"实验室状态",bold,INK);text(dc,rail+22,646,246,100,count,headingFont,ACCENT);text(dc,x,838,labw,30,"近期预约",bold,INK);
    } else if(page==1){
      char today[160],prefs[256],identityText[256],date[16];date_string(date);ui_summary(&model,&client.session,date,today,sizeof(today),prefs,sizeof(prefs));snprintf(identityText,sizeof(identityText),"%s  ·  %s  ·  %s",client.session.name,admin()?"管理员":"学生",client.session.login);
      box(dc,x,124,cw,94,RGB(255,255,255));text(dc,x+24,148,cw-48,32,identityText,headingFont,INK);text(dc,x+24,188,cw-48,22,"个人资料 · 使用与网页相同的云端账号",font,MUTED);
      box(dc,x,238,cw/2-8,104,RGB(255,255,255));text(dc,x+22,254,cw/2-40,25,"今天的预约",bold,INK);text(dc,x+22,290,cw/2-40,44,today,font,MUTED);
      box(dc,x+cw/2+8,238,cw/2-8,104,RGB(255,255,255));text(dc,x+cw/2+30,254,cw/2-40,25,"预约偏好",bold,INK);text(dc,x+cw/2+30,290,cw/2-48,44,prefs,font,MUTED);
      text(dc,x,354,cw,28,"手动签到与签退",bold,INK);text(dc,x,748,cw,30,"我的预约",bold,INK);text(dc,x,780,cw,20,"完整记录与实际使用时间（北京时间）",font,MUTED);
    } else if(page==4){box(dc,x,124,cw,350,RGB(255,255,255));text(dc,x+24,142,cw-48,24,"智能助手",bold,INK);text(dc,x,464,cw,24,"输入设备、日期或时段需求，获取预约建议。",font,MUTED);}
    else text(dc,x,122,cw,24,page==2?"故障说明和维修进度":"用户和账号管理",font,MUTED);
    if(preview)text(dc,x,1112,cw,24,"界面预览 · 示例数据 · 未连接真实账号",font,MUTED);
}
/* Capture only our fixture window; never reads another application's pixels. */
static int preview_capture(HWND h) {
 RECT r;GetClientRect(h,&r);int w=r.right,height=r.bottom;HDC dc=GetDC(h),mem=CreateCompatibleDC(dc);HBITMAP bitmap=CreateCompatibleBitmap(dc,w,height);HGDIOBJ old=SelectObject(mem,bitmap);
 PrintWindow(h,mem,3);SelectObject(mem,old);
 BITMAPINFO info={0};info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=w;info.bmiHeader.biHeight=-height;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;
 DWORD bytes=(DWORD)(w*height*4);void *pixels=calloc(1,bytes);int ok=pixels&&GetDIBits(dc,bitmap,0,height,pixels,&info,DIB_RGB_COLORS);FILE *f=NULL;
 if(ok)f=_wfopen(previewOutput,L"wb");
 if(f){BITMAPFILEHEADER file={0};file.bfType=0x4d42;file.bfOffBits=sizeof(file)+sizeof(BITMAPINFOHEADER);file.bfSize=file.bfOffBits+bytes;ok=fwrite(&file,sizeof(file),1,f)==1&&fwrite(&info.bmiHeader,sizeof(BITMAPINFOHEADER),1,f)==1&&fwrite(pixels,bytes,1,f)==1;fclose(f);}else ok=0;
 free(pixels);DeleteObject(bitmap);DeleteDC(mem);ReleaseDC(h,dc);return ok;
}
static LRESULT CALLBACK wndproc(HWND h, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_CREATE: {
        window = h;
        LOGFONTW lf = {0};
        lf.lfHeight = -15;
        wcscpy(lf.lfFaceName, L"Microsoft YaHei UI");
        font = CreateFontIndirectW(&lf);
        lf.lfWeight = FW_BOLD;
        bold = CreateFontIndirectW(&lf);
        background = CreateSolidBrush(BG);
        lf.lfHeight=-28;headingFont=CreateFontIndirectW(&lf);lf.lfHeight=-34;heroFont=CreateFontIndirectW(&lf);
        const wchar_t *navNames[]={L"预约",L"我的",L"故障记录",L"用户管理",L"智能助手"};
        for(int i=0;i<5;i++)navigation[i]=control(L"BUTTON",navNames[i],BS_OWNERDRAW|WS_TABSTOP,600+i,16,152+i*50,196,44);
        profileFault=control(L"BUTTON",L"我的故障记录",BS_OWNERDRAW|WS_TABSTOP,605,260,340,150,38);
        agentReply=control(L"EDIT",L"告诉我你的预约需求。",ES_MULTILINE|ES_READONLY|WS_VSCROLL,0,260,170,800,280);
        whoText = control(L"STATIC", L"", 0, 0, 500, 20, 570, 26);
        const wchar_t *texts[] = {L"学号 / 邮箱", L"密码", L"姓名（注册）", L"管理员邀请码（可选）"};
        for (int i = 0; i < 4; i++)
            labels[i] = control(L"STATIC", texts[i], 0, 0, 155, 120 + i * 65, 280, 24);
        identity = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 120, 370, 30);
        password = control(L"EDIT", L"", WS_BORDER | ES_PASSWORD | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 185,
                           370, 30);
        nameEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 250, 370, 30);
        inviteEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 450, 315, 370, 30);
        loginButton = control(L"BUTTON", L"登录并继续", BS_OWNERDRAW | WS_TABSTOP, ID_LOGIN, 450, 380, 175, 38);
        registerButton = control(L"BUTTON", L"创建账号", BS_OWNERDRAW | WS_TABSTOP, ID_REGISTER, 645, 380, 175, 38);
        loginInfo = control(L"STATIC",
                            L"使用与网页相同的云端账号。\n学生使用学号；管理员使用邮箱。",
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
                                          LVS_EX_FULLROWSELECT | LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(list,RGB(255,255,255));ListView_SetTextBkColor(list,RGB(255,255,255));ListView_SetTextColor(list,INK);
        usageList=control(WC_LISTVIEWW,L"",LVS_REPORT|LVS_SINGLESEL|WS_BORDER|WS_TABSTOP,210,260,438,800,220);
        ListView_SetExtendedListViewStyle(usageList,LVS_EX_FULLROWSELECT|LVS_EX_DOUBLEBUFFER);
        ListView_SetBkColor(usageList,RGB(255,255,255));ListView_SetTextBkColor(usageList,RGB(255,255,255));ListView_SetTextColor(usageList,INK);
        noteEdit = control(L"EDIT", L"", WS_BORDER | ES_AUTOHSCROLL | WS_TABSTOP, 0, 24, 560, 1040, 28);
        SendMessageW(noteEdit, EM_SETCUEBANNER, TRUE,
                     (LPARAM)L"故障 / 维修说明或智能助手问题；修改资料：姓名|登录名");
        for (int i = 0; i < 15; i++)
            actions[i] = control(L"BUTTON", actionNames[i], BS_OWNERDRAW|WS_TABSTOP, actionIds[i], 24, 600, 135, 30);
        statusText = control(L"STATIC", L"欢迎使用。所有预约时段按北京时间计算。", 0, 0, 24, 650, 1040, 24);
        if(preview){SetWindowTextW(statusText,L"界面预览 · 示例数据 · 未连接真实账号");ui_preview_model(&client,&model);logged=preview!=1;page=preview==3?1:preview==4?4:0;render();}
        else SetTimer(h,1,30000,NULL);
        layout();
        Job *j = calloc(1, sizeof(*j));
        if (j && !preview) {
            j->kind = JOB_UPDATE;
            start_job(j);
        }else free(j);
        return 0;
    }
    case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(h,&ps);paint_workspace(dc);EndPaint(h,&ps);return 0;}
    case WM_MOUSEWHEEL:scrollY-=GET_WHEEL_DELTA_WPARAM(wp)/WHEEL_DELTA*80;if(scrollY<0)scrollY=0;layout();return 0;
    case WM_VSCROLL:{SCROLLINFO si={0};si.cbSize=sizeof(si);si.fMask=SIF_ALL;GetScrollInfo(h,SB_VERT,&si);int code=LOWORD(wp);if(code==SB_THUMBTRACK)scrollY=si.nTrackPos;else if(code==SB_LINEUP)scrollY-=40;else if(code==SB_LINEDOWN)scrollY+=40;else if(code==SB_PAGEUP)scrollY-=si.nPage;else if(code==SB_PAGEDOWN)scrollY+=si.nPage;if(scrollY<0)scrollY=0;layout();return 0;}
    case WM_SIZE:
        if (list)
            layout();
        return 0;
    case WM_GETMINMAXINFO:
        ((MINMAXINFO *)lp)->ptMinTrackSize.x = 1200;
        ((MINMAXINFO *)lp)->ptMinTrackSize.y = 800;
        return 0;
    case WM_COMMAND: {
        int id = LOWORD(wp);
        if(id>=600&&id<=605){if(busy)return 0;page=id==605?2:id-600;if(page==3&&!admin())page=1;scrollY=0;render();layout();return 0;}
        if(preview&&!(id>=ID_FIRST_DEVICE&&id<ID_FIRST_DEVICE+40))return 0;
        if (id >= ID_FIRST_DEVICE && id < ID_FIRST_DEVICE + 40) {
            int i = id - ID_FIRST_DEVICE;
            selected[i] = !selected[i];
            InvalidateRect(deviceButtons[i], NULL, TRUE);
            InvalidateRect(window,NULL,TRUE);
        } else if (id == ID_START || id == ID_END) {
            if (HIWORD(wp) == CBN_SELCHANGE)
                render();
        } else if (HIWORD(wp) == BN_CLICKED)
            command(id);
        return 0;
    }
    case WM_NOTIFY: {
        NMHDR *n = (NMHDR *)lp;
        if(!rendering&&(n->idFrom==ID_LIST||n->idFrom==210)&&n->code==LVN_ITEMCHANGED){
            NMLISTVIEW *v=(NMLISTVIEW *)lp;if((v->uNewState&LVIS_SELECTED)&&v->iItem>=0&&v->iItem<listedCount){selectedTable=n->idFrom==210;snprintf(selectedId,sizeof(selectedId),"%s",lab_str(listed[v->iItem],"id"));}
        }
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
                SetWindowTextW(statusText, L"原生 C 3.1.0 · 云端数据每 30 秒刷新");
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
            if(*j->message&&j->kind==JOB_AGENT&&page==4)set_utf8(agentReply,j->message);
            else if (*j->message)
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
        layout();
        return 0;
    }
    case WM_TIMER:
        if(wp==2){KillTimer(h,2);if(!preview_capture(h))PostQuitMessage(2);else DestroyWindow(h);return 0;}
        if (logged && !busy)
            command(ID_REFRESH);
        return 0;
    case WM_DRAWITEM: {
        DRAWITEMSTRUCT *d = (DRAWITEMSTRUCT *)lp;
        int i = (int)d->CtlID - ID_FIRST_DEVICE;
        if(i<0||i>=40){
            int id=(int)d->CtlID,nav=id>=600&&id<=604,primary=id==ID_BOOK||id==ID_LOGIN||id==ID_AGENT,active=nav&&page==id-600;
            COLORREF bg=primary?ACCENT:active?RGB(242,242,239):RGB(255,255,255);
            HBRUSH brush=CreateSolidBrush(bg);HPEN pen=CreatePen(PS_SOLID,1,nav?bg:BORDER);HGDIOBJ oldb=SelectObject(d->hDC,brush),oldp=SelectObject(d->hDC,pen);
            RoundRect(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom,16,16);SelectObject(d->hDC,oldb);SelectObject(d->hDC,oldp);DeleteObject(brush);DeleteObject(pen);
            RECT rect=d->rcItem;if(nav)rect.left+=14;SetBkMode(d->hDC,TRANSPARENT);SetTextColor(d->hDC,primary?RGB(255,255,255):INK);SelectObject(d->hDC,active?bold:font);wchar_t label[128];GetWindowTextW(d->hwndItem,label,128);DrawTextW(d->hDC,label,-1,&rect,(nav?DT_LEFT:DT_CENTER)|DT_VCENTER|DT_SINGLELINE);if(active){RECT line={d->rcItem.left,d->rcItem.top+9,d->rcItem.left+3,d->rcItem.bottom-9};HBRUSH a=CreateSolidBrush(ACCENT);FillRect(d->hDC,&line,a);DeleteObject(a);}if(d->itemState&ODS_FOCUS)DrawFocusRect(d->hDC,&rect);return TRUE;
        }
        int occupied = 0, fault = 0, live = 0;
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
        cJSON_ArrayForEach(r,model.bookings)if(lab_int(r,"device_number")==i+1&&!strcmp(lab_str(r,"status"),"in_use"))live=1;
        COLORREF color = selected[i] ? RGB(239,240,255)
                         : fault     ? RGB(252,240,239)
                         : live      ? RGB(232,246,250)
                         : occupied  ? RGB(242,242,239)
                                     : RGB(252,252,251);
        HBRUSH brush = CreateSolidBrush(color);
        HPEN pen=CreatePen(PS_SOLID,selected[i]?2:1,selected[i]?ACCENT:fault?RGB(233,194,191):BORDER);
        HGDIOBJ ob=SelectObject(d->hDC,brush),op=SelectObject(d->hDC,pen);RoundRect(d->hDC,d->rcItem.left,d->rcItem.top,d->rcItem.right,d->rcItem.bottom,16,16);SelectObject(d->hDC,ob);SelectObject(d->hDC,op);DeleteObject(pen);
        DeleteObject(brush);
        SetBkMode(d->hDC, TRANSPARENT);
        SetTextColor(d->hDC, fault ? RGB(189,82,82) : INK);
        SelectObject(d->hDC, bold);
        wchar_t b[40];
        swprintf(b, 40, L"%02d\n%ls", i + 1, fault ? L"! 故障" : live ? L"使用中" : occupied ? L"已预约" : L"* 空闲");
        RECT seatText=d->rcItem;seatText.top+=10;DrawTextW(d->hDC, b, -1, &seatText, DT_CENTER | DT_WORDBREAK);
        if (d->itemState & ODS_FOCUS)
            DrawFocusRect(d->hDC, &d->rcItem);
        return TRUE;
    }
    case WM_CTLCOLOREDIT:
        SetBkColor((HDC)wp,RGB(255,255,255));SetTextColor((HDC)wp,INK);return (LRESULT)GetStockObject(WHITE_BRUSH);
    case WM_CTLCOLORSTATIC:
        SetBkMode((HDC)wp,TRANSPARENT);
        SetBkColor((HDC)wp, BG);
        SetTextColor((HDC)wp, INK);
        return (LRESULT)((HWND)lp==helpText||(HWND)lp==statusText?background:GetStockObject(WHITE_BRUSH));
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
    if(wcsstr(commandLine,L"--preview=login"))preview=1;
    else if(wcsstr(commandLine,L"--preview=booking"))preview=2;
    else if(wcsstr(commandLine,L"--preview=profile"))preview=3;
    else if(wcsstr(commandLine,L"--preview=agent"))preview=4;
    LPWSTR output=wcsstr(commandLine,L"--preview-output=");if(preview&&output){output+=17;wcsncpy(previewOutput,output,MAX_PATH-1);}
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
    HWND h = CreateWindowExW(0, wc.lpszClassName, L"实验室预约 · 原生 C", WS_OVERLAPPEDWINDOW|WS_VSCROLL, CW_USEDEFAULT,
                             CW_USEDEFAULT, 1280, 940, NULL, NULL, instance, NULL);
    if (!h)
        return 1;
    ShowWindow(h, show);
    if(preview&&*previewOutput)SetTimer(h,2,2500,NULL);
    MSG msg;
    while (GetMessageW(&msg, NULL, 0, 0) > 0) {
        if (!IsDialogMessageW(h, &msg)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
    }
    DeleteObject(font);
    DeleteObject(bold);
    DeleteObject(headingFont);DeleteObject(heroFont);
    DeleteObject(background);
    CoUninitialize();
    return (int)msg.wParam;
}
