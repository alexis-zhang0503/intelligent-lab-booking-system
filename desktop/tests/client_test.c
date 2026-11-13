// 业务层测试：通过 LabTransport 注入模拟响应，用 assert 检查请求协议与边界行为；不访问真实云端，不能代替端到端测试。
#include "../src/core/client.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../src/ui/ui_theme.h"

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
         "reservations?select=id,device_number,user_id,reservation_date,start_hour,status,created_at&order=id&limit="
         "1000&offset=0",
         "[]", 200, NULL, NULL},
        {"/rest/v1/"
         "issue_reports?select=id,device_number,reporter_id,description,status,created_at,resolved_at,"
         "resolved_by,resolution_note&order=id&limit=1000&offset=0",
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
static void parity_contracts(void) {
    LabSession student={0},admin={0};strcpy(student.role,"student");strcpy(admin.role,"admin");
    int devices[]={4,8};
    cJSON *history=cJSON_Parse("[{\"role\":\"user\",\"message\":\"previous\"}]");
    cJSON *confirmation=cJSON_Parse("{\"proposalId\":\"proposal\",\"idempotencyKey\":\"confirm-key\"}");
    cJSON *p=lab_agent_payload(&student,"tomorrow","2026-11-06",9,12,devices,2,"request", "session",history,confirmation);
    assert(p && !strcmp(lab_str(p,"agentType"),"booking_planner"));
    assert(!strcmp(lab_str(p,"sessionId"),"session"));
    const cJSON *selection=cJSON_GetObjectItemCaseSensitive(cJSON_GetObjectItemCaseSensitive(p,"input"),"bookingSelection");
    assert(lab_int(selection,"durationHours")==3 && lab_int(selection,"requiredDeviceCount")==2);
    assert(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(selection,"preferredDevices"))==2);
    cJSON_Delete(history);cJSON_Delete(confirmation);
    assert(cJSON_GetArraySize(cJSON_GetObjectItemCaseSensitive(p,"history"))==1);
    assert(!strcmp(lab_str(cJSON_GetObjectItemCaseSensitive(p,"confirmation"),"proposalId"),"proposal"));cJSON_Delete(p);
    p=lab_agent_payload(&admin,"usage","2026-11-06",8,9,NULL,0,"request",NULL,NULL,NULL);
    assert(p && !strcmp(lab_str(p,"agentType"),"admin_analyst"));
    assert(!strcmp(lab_str(cJSON_GetObjectItemCaseSensitive(p,"input"),"startDate"),"2026-11-06"));cJSON_Delete(p);
    assert(!lab_agent_payload(&student,"x","2026-11-06",9,9,NULL,0,"id",NULL,NULL,NULL));
    assert(!lab_agent_payload(&student,"x","2026-11-06",8,9,NULL,2,"id",NULL,NULL,NULL));
    Step steps[]={{"/rest/v1/rpc/v3_admin_usage_metrics","{\"actual_used_minutes\":50,\"booked_device_minutes\":60,\"capacity_device_minutes\":null}",200,"\"p_device_number\":null",NULL}};
    Mock mock={steps,1,0};LabClient c=client(&mock);p=lab_usage_metrics(&c,"2026-11-01","2026-11-07",0);
    assert(p && lab_int(p,"actual_used_minutes")==50 && cJSON_IsNull(cJSON_GetObjectItemCaseSensitive(p,"capacity_device_minutes")));
    cJSON_Delete(p);assert(mock.index==1);
    Step malformed[]={{"/rest/v1/rpc/v3_admin_usage_metrics","{}",200,NULL,NULL}};
    mock=(Mock){malformed,1,0};c=client(&mock);assert(!lab_usage_metrics(&c,"2026-11-01","2026-11-07",4));
    assert(*c.error && mock.index==1);
    p=cJSON_Parse("{\"reservation_date\":\"2026-11-06\",\"start_hour\":9,\"status\":\"in_use\"}");
    int64_t start=lab_booking_start(p);assert(!lab_booking_is_live(p,start-1));
    assert(lab_booking_is_live(p,start));assert(lab_booking_is_live(p,start+3599999));
    assert(!lab_booking_is_live(p,start+3600000));cJSON_Delete(p);
}
int main(void) {
    parity_contracts();
    LabModel exports={0};exports.bookings=cJSON_Parse("[{\"id\":\"cancelled-record\",\"status\":\"cancelled\",\"reservation_date\":\"2025-01-01\"}]");exports.issues=cJSON_CreateArray();cJSON *issue=cJSON_CreateObject();cJSON_AddStringToObject(issue,"description","=SUM(A1)");cJSON_AddStringToObject(issue,"resolution_note","repair, note");cJSON_AddItemToArray(exports.issues,issue);
    FILE *csv=tmpfile();assert(csv&&lab_export_csv(&exports,0,csv));rewind(csv);char exported[2048];size_t exportedSize=fread(exported,1,sizeof(exported)-1,csv);exported[exportedSize]=0;assert(strstr(exported,"cancelled-record")&&strstr(exported,"2025-01-01"));fclose(csv);csv=tmpfile();assert(csv&&lab_export_csv(&exports,1,csv));rewind(csv);exportedSize=fread(exported,1,sizeof(exported)-1,csv);exported[exportedSize]=0;assert(strstr(exported,"'=SUM(A1)")&&strstr(exported,"repair, note"));fclose(csv);lab_model_free(&exports);

    LabAgentState agent={0};lab_agent_bind(&agent,"user-a");
    cJSON *reply=cJSON_Parse("{\"sessionId\":\"session-a\",\"message\":\"choose\",\"outcome\":\"suggested\",\"proposals\":[{\"proposalId\":\"proposal-a\",\"candidate\":{\"items\":[{\"reservation_date\":\"2026-11-06\",\"start_hour\":9,\"device_numbers\":[4,8]}]}}]}");
    lab_agent_accept(&agent,"user-a","two devices",reply);assert(cJSON_GetArraySize(agent.history)==2);
    cJSON *first=lab_agent_confirmation(&agent,0,"key-first"),*retry=lab_agent_confirmation(&agent,0,"key-second");
    assert(!strcmp(lab_str(first,"idempotencyKey"),lab_str(retry,"idempotencyKey")));
    char option[512];lab_agent_option(cJSON_GetArrayItem(lab_agent_proposals(&agent),0),1,option,sizeof(option));assert(strstr(option,"2026-11-06")&&strstr(option,"#04"));
    cJSON_Delete(first);cJSON_Delete(retry);
    for(int k=0;k<20;k++)lab_agent_accept(&agent,"user-a","another",reply);
    assert(cJSON_GetArraySize(agent.history)==20);
    lab_agent_bind(&agent,"user-b");assert(!*agent.session&&!agent.response&&cJSON_GetArraySize(agent.history)==0);
    lab_agent_accept(&agent,"user-a","late",reply);assert(!agent.response&&cJSON_GetArraySize(agent.history)==0);
    lab_agent_accept(&agent,"user-b","new",reply);
    cJSON *confirmed=cJSON_Parse("{\"outcome\":\"confirmed\",\"message\":\"done\",\"proposals\":[{\"proposalId\":\"old\"}]}");
    lab_agent_accept(&agent,"user-b","",confirmed);assert(!lab_agent_confirmation(&agent,0,"next-key"));
    cJSON_Delete(confirmed);cJSON_Delete(reply);lab_agent_clear(&agent);
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
