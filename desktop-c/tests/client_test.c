#include "../src/client.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/ui_theme.h"

typedef struct {
    const char *path, *response;
    int status;
    const char *bodyContains, *token;
} Step;
typedef struct {
    Step *steps;
    int count, index;
} Mock;
static char *send_mock(void *ctx, const char *host, const char *method, const char *path, const char *key,
                       const char *token, const char *body, int *status, char *error, size_t cap) {
    Mock *m = ctx;
    assert(m->index < m->count);
    Step *s = &m->steps[m->index++];
    assert(!strcmp(host, LAB_API_HOST));
    assert(!strcmp(key, LAB_PUBLIC_KEY));
    assert(*method);
    assert(!strcmp(path, s->path));
    if (s->bodyContains)
        assert(strstr(body, s->bodyContains));
    if (s->token)
        assert(!strcmp(token, s->token));
    *status = s->status;
    *error = 0;
    (void)cap;
    char *out = malloc(strlen(s->response) + 1);
    strcpy(out, s->response);
    return out;
}
static LabClient client(Mock *m) {
    LabClient c = {0};
    c.transport = send_mock;
    c.context = m;
    return c;
}
static void auth(void) {
    Step steps[] = {{"/rest/v1/rpc/resolve_student_auth_email",
                     "{\"code\":\"PGRST202\",\"message\":\"missing\"}", 404, "entered_login_id", NULL},
                    {"/auth/v1/token?grant_type=password",
                     "{\"access_token\":\"jwt\",\"refresh_token\":\"refresh\",\"user\":{\"id\":\"u\"}}", 200,
                     "abc@lab.njupt.local", ""}};
    Mock m = {steps, 2, 0};
    LabClient c = client(&m);
    assert(lab_login(&c, "ABC", "p\"ass"));
    assert(m.index == 2 && !strcmp(c.session.uid, "u"));
    lab_session_clear(&c.session);
    Step denied[] = {
        {"/rest/v1/rpc/resolve_student_auth_email", "{\"message\":\"not found\"}", 404, NULL, NULL}};
    m = (Mock){denied, 1, 0};
    c = client(&m);
    assert(!lab_login(&c, "ABC", "password"));
    assert(m.index == 1);
    Step malformed[] = {{"/auth/v1/token?grant_type=password", "{\"access_token\":\"x\"}", 200, NULL, NULL}};
    m = (Mock){malformed, 1, 0};
    c = client(&m);
    assert(!lab_login(&c, "a@b.c", "password"));
    assert(!*c.session.access);
}
static void refresh(void) {
    Step steps[] = {{"/test", "{\"message\":\"expired\"}", 401, NULL, "old"},
                    {"/auth/v1/token?grant_type=refresh_token",
                     "{\"access_token\":\"new\",\"refresh_token\":\"r2\",\"user\":{\"id\":\"u\"}}", 200, "r1",
                     NULL},
                    {"/test", "[]", 200, NULL, "new"}};
    Mock m = {steps, 3, 0};
    LabClient c = client(&m);
    strcpy(c.session.access, "old");
    strcpy(c.session.refresh, "r1");
    cJSON *r = lab_request(&c, "GET", "/test", NULL);
    assert(r && m.index == 3);
    cJSON_Delete(r);
    lab_session_clear(&c.session);
    Step failed[] = {{"/test", "{}", 401, NULL, NULL},
                     {"/auth/v1/token?grant_type=refresh_token", "{}", 401, NULL, NULL}};
    m = (Mock){failed, 2, 0};
    c = client(&m);
    strcpy(c.session.refresh, "r1");
    assert(!lab_request(&c, "POST", "/test", NULL));
    assert(m.index == 2);
}
static void attendance(void) {
    cJSON *b = cJSON_Parse("{\"id\":\"reservation\",\"user_id\":\"u\",\"reservation_date\":\"2026-11-04\","
                           "\"start_hour\":8,\"status\":\"reserved\"}");
    int64_t t = lab_booking_start(b);
    assert(t == 1793750400000LL);
    assert(!lab_attendance_action(b, t - 1));
    assert(lab_attendance_action(b, t) == 1);
    assert(lab_attendance_action(b, t + 3599999) == 1);
    assert(!lab_attendance_action(b, t + 3600000));
    Step steps[] = {{"/rest/v1/rpc/v2_check_in", "null", 200, "\"p_reservation_id\":\"reservation\"", NULL},
                    {"/rest/v1/rpc/v2_check_out", "null", 200, "p_reservation_id", NULL}};
    Mock m = {steps, 2, 0};
    LabClient c = client(&m);
    strcpy(c.session.uid, "other");
    assert(!lab_attend(&c, b, 1, t));
    assert(m.index == 0);
    strcpy(c.session.uid, "u");
    assert(!lab_attend(&c, b, 0, t - 1));
    assert(lab_attend(&c, b, 1, t));
    cJSON_ReplaceItemInObject(b, "status", cJSON_CreateString("in_use"));
    assert(lab_attend(&c, b, 2, t + 7200000));
    assert(m.index == 2);
    cJSON_ReplaceItemInObject(b, "reservation_date", cJSON_CreateString("2026-02-30"));
    assert(lab_booking_start(b) == -1);
    cJSON_Delete(b);
}
static void partial_booking(void) {
    Step steps[] = {{"/rest/v1/rpc/v2_create_reservations", "[]", 200, "\"p_idempotency_key\":\"k0\"", NULL},
                    {"/rest/v1/rpc/v2_create_reservations", "{\"message\":\"conflict\"}", 409,
                     "\"p_idempotency_key\":\"k1\"", NULL}};
    Mock m = {steps, 2, 0};
    LabClient c = client(&m);
    const char *keys[9] = {"k0", "k1"};
    int devices[] = {1, 40}, n;
    assert(!lab_reserve_range(&c, "2026-11-04", 8, 10, devices, 2, keys, &n));
    assert(n == 1 && m.index == 2);
    devices[1] = 1;
    assert(!lab_reserve_range(&c, "2026-11-04", 8, 9, devices, 2, keys, &n));
    assert(n == 0 && m.index == 2);
}
static void model_and_signup(void) {
    Step steps[] = {
        {"/rest/v1/profiles?select=id,login_id,display_name,role,active&order=id&limit=1000&offset=0",
         "[{\"id\":\"u\",\"role\":\"student\",\"active\":true,\"display_name\":\"Test\"}]", 200, NULL, NULL},
        {"/rest/v1/devices?select=number,status&order=number&limit=1000&offset=0",
         "[{\"number\":1,\"status\":\"available\"}]", 200, NULL, NULL},
        {"/rest/v1/"
         "reservations?select=id,device_number,user_id,reservation_date,start_hour,status&order=id&limit="
         "1000&offset=0",
         "[]", 200, NULL, NULL},
        {"/rest/v1/"
         "issue_reports?select=id,device_number,reporter_id,description,status,created_at,resolved_at,"
         "resolution_note&order=id&limit=1000&offset=0",
         "[]", 200, NULL, NULL},
        {"/rest/v1/"
         "reservation_usage_records?select=reservation_id,checked_in_at,checked_out_at&order=reservation_id&"
         "limit=1000&offset=0",
         "[]", 200, NULL, NULL}};
    Mock m = {steps, 5, 0};
    LabClient c = client(&m);
    LabModel model = {0};
    strcpy(c.session.uid, "u");
    strcpy(c.session.role, "admin");
    assert(lab_load_model(&c, &model));
    assert(m.index == 5);
    assert(!strcmp(c.session.role, "student"));
    assert(cJSON_GetArraySize(model.devices) == 1);
    lab_model_free(&model);
    Step disabled[] = {
        {"/rest/v1/profiles?select=id,login_id,display_name,role,active&order=id&limit=1000&offset=0",
         "[{\"id\":\"u\",\"active\":false}]", 200, NULL, NULL}};
    m = (Mock){disabled, 1, 0};
    c = client(&m);
    strcpy(c.session.uid, "u");
    strcpy(c.session.access, "token");
    assert(!lab_load_model(&c, &model));
    assert(!*c.session.access);
    assert(!model.profiles);
    Step signup[] = {{"/auth/v1/signup",
                      "{\"access_token\":\"jwt\",\"refresh_token\":\"r\",\"user\":{\"id\":\"u\"}}", 200,
                      "admin_invite_code", NULL}};
    m = (Mock){signup, 1, 0};
    c = client(&m);
    assert(lab_register(&c, "Name \"Test\"", "admin@example.test", "password", "INVITE"));
    lab_session_clear(&c.session);
}
int main(void) {
    LabClient previewClient={0};LabModel previewModel={0};char display[160],prefs[256];
    ui_preview_model(&previewClient,&previewModel);
    ui_time("2026-11-04T18:02:00Z",display,sizeof(display));assert(!strcmp(display,"2026-11-05 02:02"));
    assert(ui_timestamp("2026-11-04T09:02:00+08:00")==ui_timestamp("2026-11-04T01:02:00.000+00:00"));
    ui_duration(ui_usage(&previewModel,cJSON_GetArrayItem(previewModel.bookings,0)),display,sizeof(display));assert(!strcmp(display,"50 分钟"));
    ui_summary(&previewModel,&previewClient.session,"2026-11-04",display,sizeof(display),prefs,sizeof(prefs));assert(strstr(display,"1 个预约"));
    assert(!strcmp(ui_page_title(4),"智能助手"));assert(!strcmp(ui_status("confirmed"),"已预约"));
    lab_model_free(&previewModel);lab_session_clear(&previewClient.session);
    model_and_signup();
    auth();
    refresh();
    attendance();
    partial_booking();
    assert(lab_semver_newer("3.0.1", "3.0.0"));
    assert(lab_semver_newer("4.0.0", "3.9.9"));
    assert(!lab_semver_newer("-1.0.0", "3.0.0"));
    assert(!lab_semver_newer("3.0.0-beta", "3.0.0"));
    assert(!lab_semver_newer("3.0.0", "3.0.0"));
    assert(lab_int(NULL, "x") == 0);
    puts("Native C tests passed: authentication, refresh, CST boundaries, ownership, attendance RPCs, "
         "partial booking and versions.");
    return 0;
}
