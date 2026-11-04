#include "client.h"
#include <ctype.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

const char *lab_str(const cJSON *j, const char *key) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(j, key);
    return cJSON_IsString(v) && v->valuestring ? v->valuestring : "";
}
int lab_int(const cJSON *j, const char *key) {
    const cJSON *v = cJSON_GetObjectItemCaseSensitive(j, key);
    return cJSON_IsNumber(v) && v->valuedouble >= INT_MIN && v->valuedouble <= INT_MAX ? v->valueint : 0;
}
static int copy_text(char *dst, size_t cap, const char *src) {
    if (!src || strlen(src) >= cap)
        return 0;
    memcpy(dst, src, strlen(src) + 1);
    return 1;
}
void lab_session_clear(LabSession *s) {
    volatile unsigned char *p = (volatile unsigned char *)s;
    size_t n = sizeof(*s);
    while (n--)
        *p++ = 0;
}
void lab_model_free(LabModel *m) {
    cJSON_Delete(m->devices);
    cJSON_Delete(m->bookings);
    cJSON_Delete(m->profiles);
    cJSON_Delete(m->issues);
    cJSON_Delete(m->usage);
    memset(m, 0, sizeof(*m));
}
static void explain(LabClient *c, cJSON *j) {
    const char *msg = lab_str(j, "message"), *code = lab_str(j, "code");
    if (!*msg)
        msg = lab_str(j, "error_description");
    if (!*msg)
        msg = lab_str(j, "msg");
    if (!*msg)
        msg = lab_str(j, "error");
    if (!strcmp(code, "PGRST202") || !strcmp(code, "42883"))
        msg = "云端接口尚未安装，请管理员执行对应数据库迁移。";
    else if (strstr(msg, "during the booked hour"))
        msg = "仅可在预约时段内签到（北京时间）。";
    else if (strstr(msg, "Check-in is required"))
        msg = "请先签到，再签退。";
    else if (strstr(msg, "Invalid login credentials"))
        msg = "账号或密码不正确。";
    snprintf(c->error, sizeof(c->error), "%s%s%s（HTTP %d）", *msg ? msg : "云端请求失败", *code ? " · " : "",
             code, c->status);
}
static int accept_tokens(LabClient *c, cJSON *j) {
    LabSession next = c->session;
    const cJSON *u = cJSON_GetObjectItemCaseSensitive(j, "user");
    if (!*lab_str(j, "access_token") ||
        !copy_text(next.access, sizeof(next.access), lab_str(j, "access_token")) ||
        !copy_text(next.refresh, sizeof(next.refresh), lab_str(j, "refresh_token")) ||
        !copy_text(next.uid, sizeof(next.uid), lab_str(u, "id")) || !*next.uid) {
        snprintf(c->error, sizeof(c->error), "登录响应不完整，请重新登录。");
        return 0;
    }
    c->session = next;
    lab_session_clear(&next);
    return 1;
}
static cJSON *raw_request(LabClient *c, const char *method, const char *path, const cJSON *body, int retry) {
    char *serialized = body ? cJSON_PrintUnformatted(body) : NULL;
    char *response;
    cJSON *j;
    c->error[0] = 0;
    c->status = 0;
    if (body && !serialized) {
        snprintf(c->error, sizeof(c->error), "内存不足。");
        return NULL;
    }
    response = c->transport(c->context, LAB_API_HOST, method, path, LAB_PUBLIC_KEY, c->session.access,
                            serialized ? serialized : "", &c->status, c->error, sizeof(c->error));
    if (serialized) {
        memset(serialized, 0, strlen(serialized));
        cJSON_free(serialized);
    }
    if (!response)
        return NULL;
    j = *response ? cJSON_Parse(response) : cJSON_CreateNull();
    memset(response, 0, strlen(response));
    free(response);
    if (c->status == 401 && retry && *c->session.refresh) {
        cJSON *payload = cJSON_CreateObject(), *fresh;
        cJSON_Delete(j);
        cJSON_AddStringToObject(payload, "refresh_token", c->session.refresh);
        fresh = raw_request(c, "POST", "/auth/v1/token?grant_type=refresh_token", payload, 0);
        cJSON_Delete(payload);
        if (!fresh)
            return NULL;
        if (!accept_tokens(c, fresh)) {
            cJSON_Delete(fresh);
            return NULL;
        }
        cJSON_Delete(fresh);
        return raw_request(c, method, path, body, 0);
    }
    if (c->status < 200 || c->status >= 300) {
        explain(c, j);
        cJSON_Delete(j);
        return NULL;
    }
    if (!j)
        snprintf(c->error, sizeof(c->error), "云端返回了无法识别的数据，请稍后重试。");
    return j;
}
cJSON *lab_request(LabClient *c, const char *method, const char *path, const cJSON *body) {
    return raw_request(c, method, path, body, 1);
}
cJSON *lab_rpc(LabClient *c, const char *name, const cJSON *body) {
    char path[180];
    const char *p;
    for (p = name; *p; p++)
        if (!(isalnum((unsigned char)*p) || *p == '_'))
            return NULL;
    if (strlen(name) > 100)
        return NULL;
    snprintf(path, sizeof(path), "/rest/v1/rpc/%s", name);
    return lab_request(c, "POST", path, body);
}
static int student_email(const char *id, char *email, size_t cap) {
    size_t n = strlen(id), i;
    if (n < 2 || n > 20)
        return 0;
    for (i = 0; i < n; i++)
        if (!(isalnum((unsigned char)id[i]) || id[i] == '.' || id[i] == '_' || id[i] == '-'))
            return 0;
    if (n + 17 >= cap)
        return 0;
    for (i = 0; i < n; i++)
        email[i] = (char)tolower((unsigned char)id[i]);
    strcpy(email + n, "@lab.njupt.local");
    return 1;
}
int lab_login(LabClient *c, const char *identity, const char *password) {
    char email[256];
    cJSON *p, *j;
    int ok;
    lab_session_clear(&c->session);
    if (strchr(identity, '@')) {
        if (!copy_text(email, sizeof(email), identity))
            return 0;
    } else {
        p = cJSON_CreateObject();
        cJSON_AddStringToObject(p, "entered_login_id", identity);
        j = lab_rpc(c, "resolve_student_auth_email", p);
        cJSON_Delete(p);
        if (j) {
            ok = cJSON_IsString(j) && copy_text(email, sizeof(email), j->valuestring);
            cJSON_Delete(j);
            if (!ok) {
                snprintf(c->error, sizeof(c->error), "账号或密码不正确。");
                return 0;
            }
        } else if (c->status == 404 && (strstr(c->error, "PGRST202") || strstr(c->error, "42883"))) {
            if (!student_email(identity, email, sizeof(email))) {
                snprintf(c->error, sizeof(c->error), "请检查学号格式。");
                return 0;
            }
        } else
            return 0;
    }
    p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "email", email);
    cJSON_AddStringToObject(p, "password", password);
    j = raw_request(c, "POST", "/auth/v1/token?grant_type=password", p, 0);
    cJSON_Delete(p);
    if (!j)
        return 0;
    ok = accept_tokens(c, j);
    cJSON_Delete(j);
    return ok;
}
int lab_register(LabClient *c, const char *name, const char *id, const char *password, const char *invite) {
    char email[256];
    cJSON *p, *meta, *j;
    int ok;
    if (!*name || strlen(name) > 64 || strlen(password) < 6) {
        snprintf(c->error, sizeof(c->error), "请填写姓名，密码至少 6 个字符。");
        return 0;
    }
    if (*invite) {
        if (!strchr(id, '@') || !copy_text(email, sizeof(email), id)) {
            snprintf(c->error, sizeof(c->error), "管理员注册需填写邮箱和邀请码。");
            return 0;
        }
    } else if (!student_email(id, email, sizeof(email))) {
        snprintf(c->error, sizeof(c->error), "请检查学号格式（2–20 位字母、数字、点、横线或下划线）。");
        return 0;
    }
    lab_session_clear(&c->session);
    p = cJSON_CreateObject();
    meta = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "email", email);
    cJSON_AddStringToObject(p, "password", password);
    cJSON_AddStringToObject(meta, "login_id", id);
    cJSON_AddStringToObject(meta, "display_name", name);
    if (*invite)
        cJSON_AddStringToObject(meta, "admin_invite_code", invite);
    cJSON_AddItemToObject(p, "data", meta);
    j = raw_request(c, "POST", "/auth/v1/signup", p, 0);
    cJSON_Delete(p);
    if (!j)
        return 0;
    if (!*lab_str(j, "access_token")) {
        snprintf(c->error, sizeof(c->error),
                 "账号已创建，需完成邮箱验证后登录；学生内部邮箱请联系管理员检查认证配置。");
        cJSON_Delete(j);
        return 0;
    }
    ok = accept_tokens(c, j);
    cJSON_Delete(j);
    return ok;
}
static cJSON *load_table(LabClient *c, const char *table, const char *columns, const char *order) {
    cJSON *all = cJSON_CreateArray();
    int offset = 0;
    for (;;) {
        char path[900];
        cJSON *rows, *item;
        int n;
        snprintf(path, sizeof(path), "/rest/v1/%s?select=%s&order=%s&limit=1000&offset=%d", table, columns,
                 order, offset);
        rows = lab_request(c, "GET", path, NULL);
        if (!cJSON_IsArray(rows)) {
            cJSON_Delete(rows);
            cJSON_Delete(all);
            return NULL;
        }
        n = cJSON_GetArraySize(rows);
        while ((item = cJSON_DetachItemFromArray(rows, 0)) != NULL)
            cJSON_AddItemToArray(all, item);
        cJSON_Delete(rows);
        if (n < 1000)
            return all;
        offset += n;
        if (offset >= 100000) {
            cJSON_Delete(all);
            snprintf(c->error, sizeof(c->error), "数据量过大，请使用网页管理端筛选后操作。");
            return NULL;
        }
    }
}
int lab_load_model(LabClient *c, LabModel *out) {
    LabModel next = {0};
    const cJSON *p = NULL, *item;
    next.profiles = load_table(c, "profiles", "id,login_id,display_name,role,active", "id");
    if (!next.profiles)
        goto fail;
    cJSON_ArrayForEach(item, next.profiles) if (!strcmp(lab_str(item, "id"), c->session.uid)) {
        p = item;
        break;
    }
    if (!p || cJSON_IsFalse(cJSON_GetObjectItemCaseSensitive(p, "active"))) {
        snprintf(c->error, sizeof(c->error), "账号资料不可用或已停用，请联系管理员。");
        lab_session_clear(&c->session);
        goto fail;
    }
    copy_text(c->session.name, sizeof(c->session.name), lab_str(p, "display_name"));
    copy_text(c->session.login, sizeof(c->session.login), lab_str(p, "login_id"));
    copy_text(c->session.role, sizeof(c->session.role), lab_str(p, "role"));
    next.devices = load_table(c, "devices", "number,status", "number");
    if (!next.devices)
        goto fail;
    next.bookings =
        load_table(c, "reservations", "id,device_number,user_id,reservation_date,start_hour,status", "id");
    if (!next.bookings)
        goto fail;
    next.issues = load_table(
        c, "issue_reports",
        "id,device_number,reporter_id,description,status,created_at,resolved_at,resolution_note", "id");
    if (!next.issues)
        goto fail;
    next.usage = load_table(c, "reservation_usage_records", "reservation_id,checked_in_at,checked_out_at",
                            "reservation_id");
    if (!next.usage) {
        next.usage = cJSON_CreateArray();
        c->error[0] = 0;
    }
    *out = next;
    return 1;
fail:
    lab_model_free(&next);
    return 0;
}
/* Gregorian civil day conversion, independent of the computer timezone. */
static int64_t civil_days(int y, unsigned m, unsigned d) {
    int era;
    unsigned yoe, doy, doe;
    y -= m <= 2;
    era = (y >= 0 ? y : y - 399) / 400;
    yoe = (unsigned)(y - era * 400);
    doy = (153 * (m > 2 ? m - 3 : m + 9) + 2) / 5 + d - 1;
    doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;
    return (int64_t)era * 146097 + doe - 719468;
}
int64_t lab_booking_start(const cJSON *b) {
    const char *date = lab_str(b, "reservation_date");
    int y, m, d, h = lab_int(b, "start_hour"), days;
    for (size_t i = 0; i < strlen(date); i++)
        if (i != 4 && i != 7 && !isdigit((unsigned char)date[i]))
            return -1;
    if (strlen(date) != 10 || sscanf(date, "%4d-%2d-%2d", &y, &m, &d) != 3 || date[4] != '-' ||
        date[7] != '-' || y < 1970 || y > 9999 || m < 1 || m > 12 || h < 8 || h > 16)
        return -1;
    days = (int[]){31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31}[m - 1];
    if (m == 2 && (y % 4 == 0 && (y % 100 != 0 || y % 400 == 0)))
        days = 29;
    if (d < 1 || d > days)
        return -1;
    return (civil_days(y, (unsigned)m, (unsigned)d) * 86400 + (h - 8) * 3600) * 1000;
}
int lab_attendance_action(const cJSON *b, int64_t now) {
    const char *s = lab_str(b, "status");
    int64_t start;
    if (!strcmp(s, "in_use"))
        return 2;
    if (strcmp(s, "reserved") && strcmp(s, "confirmed"))
        return 0;
    start = lab_booking_start(b);
    return start >= 0 && now >= start && now < start + 3600000 ? 1 : 0;
}
int lab_attend(LabClient *c, const cJSON *b, int action, int64_t now) {
    cJSON *p, *r;
    if ((action != 1 && action != 2) || !b || strcmp(lab_str(b, "user_id"), c->session.uid) ||
        lab_attendance_action(b, now) != action) {
        snprintf(c->error, sizeof(c->error), "请检查当前账号、预约时段（北京时间）及签到状态，刷新后重试。");
        return 0;
    }
    p = cJSON_CreateObject();
    cJSON_AddStringToObject(p, "p_reservation_id", lab_str(b, "id"));
    r = lab_rpc(c, action == 1 ? "v2_check_in" : "v2_check_out", p);
    cJSON_Delete(p);
    if (!r)
        return 0;
    cJSON_Delete(r);
    return 1;
}
int lab_reserve_range(LabClient *c, const char *date, int start, int end, const int *devices, size_t count,
                      const char *keys[9], int *committed) {
    int hour;
    size_t i;
    *committed = 0;
    if (start < 8 || end > 17 || start >= end || !count || count > 40) {
        snprintf(c->error, sizeof(c->error), "请选择有效的设备和时间范围。");
        return 0;
    }
    for (i = 0; i < count; i++)
        if (devices[i] < 1 || devices[i] > 40) {
            snprintf(c->error, sizeof(c->error), "设备编号无效。");
            return 0;
        }
    for (i = 0; i < count; i++)
        for (size_t k = 0; k < i; k++)
            if (devices[k] == devices[i]) {
                snprintf(c->error, sizeof(c->error), "设备不能重复选择。");
                return 0;
            }
    for (hour = start; hour < end; hour++) {
        cJSON *p, *a, *r;
        if (!keys[hour - 8] || !*keys[hour - 8]) {
            snprintf(c->error, sizeof(c->error), "请求编号缺失。");
            return 0;
        }
        p = cJSON_CreateObject();
        a = cJSON_CreateArray();
        for (i = 0; i < count; i++)
            cJSON_AddItemToArray(a, cJSON_CreateNumber(devices[i]));
        cJSON_AddStringToObject(p, "p_date", date);
        cJSON_AddNumberToObject(p, "p_start_hour", hour);
        cJSON_AddItemToObject(p, "p_device_numbers", a);
        cJSON_AddStringToObject(p, "p_idempotency_key", keys[hour - 8]);
        r = lab_rpc(c, "v2_create_reservations", p);
        cJSON_Delete(p);
        if (!r)
            return 0;
        cJSON_Delete(r);
        (*committed)++;
    }
    return 1;
}
int lab_semver_newer(const char *a, const char *b) {
    unsigned x[3], y[3];
    char tail;
    for (const char *p = a; *p; p++)
        if (!isdigit((unsigned char)*p) && *p != '.')
            return 0;
    for (const char *p = b; *p; p++)
        if (!isdigit((unsigned char)*p) && *p != '.')
            return 0;
    if (sscanf(a, "%u.%u.%u%c", &x[0], &x[1], &x[2], &tail) != 3 ||
        sscanf(b, "%u.%u.%u%c", &y[0], &y[1], &y[2], &tail) != 3)
        return 0;
    for (int i = 0; i < 3; i++) {
        if (x[i] > y[i])
            return 1;
        if (x[i] < y[i])
            return 0;
    }
    return 0;
}
