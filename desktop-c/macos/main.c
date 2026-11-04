#include <gtk/gtk.h>
#include <curl/curl.h>
#include "../src/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
char *lab_mac_http(void *,const char *,const char *,const char *,const char *,const char *,const char *,int *,char *,size_t);
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
} Job;

static GtkWidget *window,*authBox,*workBox,*identity,*password,*nameEdit,*inviteEdit,*dateEdit,*startEdit,*endEdit,*noteEdit,*statusText,*grid,*list,*pageEdit,*actionBox;
static GtkWidget *deviceButtons[40],*actions[15];
static GtkListStore *rows;
static LabClient client;
static LabModel model;
static int busy,logged,page;
static cJSON *listed[100000];
static int listedCount;
static const int actionIds[]={ID_REFRESH,ID_LOGOUT,ID_BOOK,ID_ATTEND,ID_CANCEL,ID_REPORT,ID_REPAIR,ID_ARCHIVE,ID_DEACTIVATE,ID_INVITE,ID_METRICS,ID_UPDATE,ID_PROFILE,ID_SUGGEST,ID_AGENT};
static const char *actionNames[]={"刷新","退出账号","预约所选设备","签到 / 签退","取消预约","提交故障","完成维修","归档故障","停用学生","生成邀请码","统计报表","检查版本","修改资料","推荐时段","智能分析"};
static int admin(void){return !strcmp(client.session.role,"admin");}
static int64_t now_ms(void){return g_get_real_time()/1000;}
static void tell(const char *text){GtkWidget *d=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,"%s",text);gtk_dialog_run(GTK_DIALOG(d));gtk_widget_destroy(d);}
static int confirm(const char *text){GtkWidget *d=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,"%s",text);gtk_dialog_set_default_response(GTK_DIALOG(d),GTK_RESPONSE_NO);int r=gtk_dialog_run(GTK_DIALOG(d));gtk_widget_destroy(d);return r==GTK_RESPONSE_YES;}
static void render(void){
 gtk_widget_set_visible(authBox,!logged);gtk_widget_set_visible(workBox,logged);gtk_list_store_clear(rows);listedCount=0;
 if(!logged)return;
 gtk_widget_set_visible(grid,page==0);
 for(int i=0;i<15;i++){
 int id=actionIds[i],show=1;
 if(id==ID_BOOK||id==ID_SUGGEST)show=page==0;
 if(id==ID_ATTEND||id==ID_CANCEL)show=page==1||(page==0&&admin());
 if(id==ID_REPORT)show=page==2;
 if(id==ID_REPAIR||id==ID_ARCHIVE)show=page==2&&admin();
 if(id==ID_DEACTIVATE||id==ID_INVITE||id==ID_METRICS)show=page==3&&admin();
 if(id==ID_AGENT)show=page==0||(page==3&&admin());
 gtk_widget_set_visible(actions[i],show);
 }
 const char *date=gtk_entry_get_text(GTK_ENTRY(dateEdit));int start=8+gtk_combo_box_get_active(GTK_COMBO_BOX(startEdit)),end=9+gtk_combo_box_get_active(GTK_COMBO_BOX(endEdit));
 for(int i=0;i<40;i++){
 int fault=0,occupied=0;const cJSON *r;
 cJSON_ArrayForEach(r,model.devices)if(lab_int(r,"number")==i+1&&strcmp(lab_str(r,"status"),"available"))fault=1;
 cJSON_ArrayForEach(r,model.bookings)if(lab_int(r,"device_number")==i+1&&!strcmp(lab_str(r,"reservation_date"),date)&&lab_int(r,"start_hour")>=start&&lab_int(r,"start_hour")<end&&(!strcmp(lab_str(r,"status"),"reserved")||!strcmp(lab_str(r,"status"),"confirmed")||!strcmp(lab_str(r,"status"),"in_use")))occupied=1;
 char label[60];snprintf(label,sizeof(label),"%02d %s",i+1,fault?"故障":occupied?"占用":"空闲");gtk_button_set_label(GTK_BUTTON(deviceButtons[i]),label);
 }
 cJSON *r; cJSON *source=page<2?model.bookings:page==2?model.issues:model.profiles;
 cJSON_ArrayForEach(r,source){
 if(page==1&&strcmp(lab_str(r,"user_id"),client.session.uid))continue;
 if(page==0&&(!admin()||strcmp(lab_str(r,"reservation_date"),date)))continue;
 if(page==3&&!admin())continue;
 if(listedCount>=100000)break;
 char a[128],b[128],c[128];
 if(page<2){snprintf(a,sizeof(a),"设备 %02d",lab_int(r,"device_number"));snprintf(b,sizeof(b),"%s %02d:00",lab_str(r,"reservation_date"),lab_int(r,"start_hour"));snprintf(c,sizeof(c),"%s",lab_str(r,"status"));}
 else if(page==2){snprintf(a,sizeof(a),"设备 %02d",lab_int(r,"device_number"));snprintf(b,sizeof(b),"%s",lab_str(r,"description"));snprintf(c,sizeof(c),"%s",lab_str(r,"status"));}
 else {snprintf(a,sizeof(a),"%s",lab_str(r,"display_name"));snprintf(b,sizeof(b),"%s",lab_str(r,"login_id"));snprintf(c,sizeof(c),"%s",lab_str(r,"role"));}
 GtkTreeIter it;gtk_list_store_append(rows,&it);gtk_list_store_set(rows,&it,0,a,1,b,2,c,3,listedCount,-1);listed[listedCount++]=r;
 }
}
static gboolean finish_job(gpointer arg){
 Job *j=arg;busy=0;
 if(j->kind==JOB_UPDATE){if(j->ok&&confirm(j->message))gtk_show_uri_on_window(GTK_WINDOW(window),"https://github.com/alexis-zhang0503/intelligent-lab-booking-system/releases/latest",GDK_CURRENT_TIME,NULL);}
 else {client=j->client;if(j->loaded){lab_model_free(&model);model=j->model;memset(&j->model,0,sizeof(j->model));logged=1;gtk_entry_set_text(GTK_ENTRY(password),"");gtk_entry_set_text(GTK_ENTRY(inviteEdit),"");}if(!*client.session.access){logged=0;lab_model_free(&model);}render();if(*j->message)tell(j->message);else if(j->ok&&j->kind!=JOB_REFRESH&&j->kind!=JOB_LOGIN)tell("操作成功，已同步云端。");}
 gtk_widget_set_sensitive(authBox,TRUE);gtk_widget_set_sensitive(workBox,TRUE);
 gtk_label_set_text(GTK_LABEL(statusText),"原生 C 3.0.0 · 北京时间 · 每 30 秒刷新云端");
 lab_model_free(&j->model);lab_session_clear(&j->client.session);cJSON_Delete(j->row);free(j);return G_SOURCE_REMOVE;
}
static void parameter(cJSON *p, const char *key, const char *s) { cJSON_AddStringToObject(p, key, s); }
static gpointer worker(gpointer arg) {
    Job *j = arg;
    cJSON *p = NULL, *r = NULL;
    int success = 0, committed = 0;
    if (j->kind == JOB_UPDATE) {
        int status = 0;
        char *raw = lab_mac_http(NULL, LAB_SITE_HOST, "GET", LAB_SITE_BASE "native-desktop.json", "", "", "",
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
    memset(j->password, 0, sizeof(j->password));
    g_idle_add(finish_job, j);
    return NULL;
}
static void command(GtkWidget *widget,gpointer data){
 (void)widget;int id=GPOINTER_TO_INT(data);if(busy)return;
 if(id==ID_LOGOUT){lab_session_clear(&client.session);lab_model_free(&model);logged=0;render();return;}
 Job *j=calloc(1,sizeof(*j));if(!j)return;j->client=client;
 snprintf(j->date,sizeof(j->date),"%s",gtk_entry_get_text(GTK_ENTRY(dateEdit)));
 snprintf(j->note,sizeof(j->note),"%s",gtk_entry_get_text(GTK_ENTRY(noteEdit)));
 j->start=8+gtk_combo_box_get_active(GTK_COMBO_BOX(startEdit));j->end=9+gtk_combo_box_get_active(GTK_COMBO_BOX(endEdit));
 for(int i=0;i<40;i++)if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(deviceButtons[i])))j->devices[j->count++]=i+1;
 GtkTreeIter it;GtkTreeModel *tm=NULL;int index=-1;
 if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)),&tm,&it))gtk_tree_model_get(tm,&it,3,&index,-1);
 if(index>=0&&index<listedCount)j->row=cJSON_Duplicate(listed[index],1);
 switch(id){
 case ID_LOGIN:case ID_REGISTER:
  j->kind=id==ID_LOGIN?JOB_LOGIN:JOB_REGISTER;
  snprintf(j->identity,sizeof(j->identity),"%s",gtk_entry_get_text(GTK_ENTRY(identity)));
  snprintf(j->password,sizeof(j->password),"%s",gtk_entry_get_text(GTK_ENTRY(password)));
  snprintf(j->name,sizeof(j->name),"%s",gtk_entry_get_text(GTK_ENTRY(nameEdit)));
  snprintf(j->invite,sizeof(j->invite),"%s",gtk_entry_get_text(GTK_ENTRY(inviteEdit)));
  if(!*j->identity||!*j->password){tell("请填写学号或邮箱和密码。");goto abort_job;}break;
 case ID_REFRESH:j->kind=JOB_REFRESH;break;
 case ID_UPDATE:j->kind=JOB_UPDATE;break;
 case ID_BOOK:
  j->kind=JOB_BOOK;if(!j->count||j->start>=j->end){tell("请选择设备和有效时段。");goto abort_job;}
  for(int i=0;i<9;i++){char *uuid=g_uuid_string_random();snprintf(j->keys[i],64,"%s",uuid);g_free(uuid);}
  if(!confirm("确认预约所选设备和时段？多小时逐时提交，若后续冲突会保留已成功的小时。"))goto abort_job;break;
 case ID_ATTEND:case ID_CANCEL:case ID_REPAIR:case ID_ARCHIVE:case ID_DEACTIVATE:
  if(!j->row){tell("请先选择列表中的一条记录。");goto abort_job;}
  if(id==ID_ATTEND){j->kind=JOB_ATTEND;j->action=lab_attendance_action(j->row,now_ms());if(strcmp(lab_str(j->row,"user_id"),client.session.uid)||!j->action){tell("仅可对自己的预约在预约小时内签到，使用中可签退。请刷新核实。");goto abort_job;}}
  else j->kind=id==ID_CANCEL?JOB_CANCEL:id==ID_REPAIR?JOB_REPAIR:id==ID_ARCHIVE?JOB_ARCHIVE:JOB_DEACTIVATE;
  if(!confirm(id==ID_ATTEND?(j->action==1?"确认签到所选预约？":"确认签退所选预约？"):"确认对所选记录执行此操作？"))goto abort_job;break;
 case ID_REPORT:j->kind=JOB_REPORT;if(!j->count){tell("请在设备预约页先选中故障设备。");goto abort_job;}if(!confirm("确认报告第一台所选设备故障？"))goto abort_job;break;
 case ID_INVITE:j->kind=JOB_INVITE;break;
 case ID_METRICS:j->kind=JOB_METRICS;break;
 case ID_PROFILE:j->kind=JOB_PROFILE;if(!confirm("确认按说明框内的 姓名|学号或邮箱 修改本人资料？"))goto abort_job;break;
 case ID_SUGGEST:j->kind=JOB_SUGGEST;break;
 case ID_AGENT:j->kind=JOB_AGENT;j->action=page==3;break;
 default:goto abort_job;
 }
 busy=1;gtk_widget_set_sensitive(authBox,FALSE);gtk_widget_set_sensitive(workBox,FALSE);gtk_label_set_text(GTK_LABEL(statusText),"正在连接云端，请稍候…");
 GThread *thread=g_thread_new("cloud",worker,j);g_thread_unref(thread);return;
abort_job:lab_session_clear(&j->client.session);memset(j->password,0,sizeof(j->password));cJSON_Delete(j->row);free(j);
}
static gboolean refresh_timer(gpointer data){(void)data;if(logged&&!busy)command(NULL,GINT_TO_POINTER(ID_REFRESH));return G_SOURCE_CONTINUE;}
static gboolean smoke_quit(gpointer data){(void)data;gtk_main_quit();return G_SOURCE_REMOVE;}
static gboolean update_start(gpointer data){(void)data;command(NULL,GINT_TO_POINTER(ID_UPDATE));return G_SOURCE_REMOVE;}
static void changed(GtkWidget *w,gpointer data){(void)w;(void)data;if(!busy)render();}
static void change_page(GtkWidget *w,gpointer data){(void)data;int next=gtk_combo_box_get_active(GTK_COMBO_BOX(w));if(next==3&&!admin()){gtk_combo_box_set_active(GTK_COMBO_BOX(w),1);tell("账号管理需要管理员权限。");return;}page=next;render();}
static GtkWidget *entry(GtkWidget *box,const char *placeholder,int secret){GtkWidget *e=gtk_entry_new();gtk_entry_set_placeholder_text(GTK_ENTRY(e),placeholder);gtk_entry_set_visibility(GTK_ENTRY(e),!secret);gtk_box_pack_start(GTK_BOX(box),e,FALSE,FALSE,3);return e;}
static GtkWidget *button(GtkWidget *box,const char *title,int id){GtkWidget *b=gtk_button_new_with_label(title);gtk_box_pack_start(GTK_BOX(box),b,FALSE,FALSE,3);g_signal_connect(b,"clicked",G_CALLBACK(command),GINT_TO_POINTER(id));return b;}
static gboolean close_window(GtkWidget *w,GdkEvent *e,gpointer data){(void)w;(void)e;(void)data;if(busy){tell("请求处理中，请等待完成后关闭。");return TRUE;}gtk_main_quit();return FALSE;}
int main(int argc,char **argv){
 curl_global_init(CURL_GLOBAL_DEFAULT);gtk_init(&argc,&argv);client.transport=lab_mac_http;
 window=gtk_window_new(GTK_WINDOW_TOPLEVEL);gtk_window_set_title(GTK_WINDOW(window),"实验室预约 · 原生 C macOS");gtk_window_set_default_size(GTK_WINDOW(window),1140,800);g_signal_connect(window,"delete-event",G_CALLBACK(close_window),NULL);
 GtkWidget *root=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);gtk_container_set_border_width(GTK_CONTAINER(root),24);gtk_container_add(GTK_CONTAINER(window),root);
 GtkWidget *title=gtk_label_new("实验室预约 · NJUPT");gtk_widget_set_halign(title,GTK_ALIGN_START);gtk_box_pack_start(GTK_BOX(root),title,FALSE,FALSE,4);
 authBox=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);gtk_box_pack_start(GTK_BOX(root),authBox,TRUE,TRUE,8);
 identity=entry(authBox,"学号 / 管理员邮箱",0);password=entry(authBox,"密码",1);nameEdit=entry(authBox,"姓名（注册）",0);inviteEdit=entry(authBox,"管理员邀请码（可选）",0);button(authBox,"登录",ID_LOGIN);button(authBox,"注册",ID_REGISTER);
 gtk_box_pack_start(GTK_BOX(authBox),gtk_label_new("使用与网页相同的账号；密码与令牌仅保留于进程内存。"),FALSE,FALSE,8);
 workBox=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);gtk_box_pack_start(GTK_BOX(root),workBox,TRUE,TRUE,0);
 GtkWidget *filters=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,6);gtk_box_pack_start(GTK_BOX(workBox),filters,FALSE,FALSE,0);
 pageEdit=gtk_combo_box_text_new();const char *pages[]={"设备预约","我的预约","故障管理","账号管理"};for(int i=0;i<4;i++)gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(pageEdit),pages[i]);gtk_combo_box_set_active(GTK_COMBO_BOX(pageEdit),0);gtk_box_pack_start(GTK_BOX(filters),pageEdit,FALSE,FALSE,0);
 dateEdit=entry(filters,"日期 YYYY-MM-DD（北京时间）",0);GDateTime *now=g_date_time_new_now_utc(),*bj=g_date_time_add_hours(now,8);char *date=g_date_time_format(bj,"%Y-%m-%d");gtk_entry_set_text(GTK_ENTRY(dateEdit),date);g_free(date);g_date_time_unref(bj);g_date_time_unref(now);
 startEdit=gtk_combo_box_text_new();endEdit=gtk_combo_box_text_new();for(int i=0;i<9;i++){char t[32];snprintf(t,sizeof(t),"开始 %02d:00",i+8);gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(startEdit),t);snprintf(t,sizeof(t),"结束 %02d:00",i+9);gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(endEdit),t);}gtk_combo_box_set_active(GTK_COMBO_BOX(startEdit),0);gtk_combo_box_set_active(GTK_COMBO_BOX(endEdit),0);gtk_box_pack_start(GTK_BOX(filters),startEdit,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(filters),endEdit,FALSE,FALSE,0);
 grid=gtk_grid_new();gtk_grid_set_row_spacing(GTK_GRID(grid),6);gtk_grid_set_column_spacing(GTK_GRID(grid),6);gtk_box_pack_start(GTK_BOX(workBox),grid,FALSE,FALSE,0);for(int i=0;i<40;i++){deviceButtons[i]=gtk_toggle_button_new_with_label("设备");gtk_widget_set_hexpand(deviceButtons[i],TRUE);gtk_grid_attach(GTK_GRID(grid),deviceButtons[i],i%10,i/10,1,1);}
 rows=gtk_list_store_new(4,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_INT);list=gtk_tree_view_new_with_model(GTK_TREE_MODEL(rows));const char *columns[]={"设备 / 姓名","预约时段 / 说明 / 账号","状态 / 角色"};for(int i=0;i<3;i++){GtkCellRenderer *cell=gtk_cell_renderer_text_new();GtkTreeViewColumn *col=gtk_tree_view_column_new_with_attributes(columns[i],cell,"text",i,NULL);gtk_tree_view_column_set_expand(col,TRUE);gtk_tree_view_append_column(GTK_TREE_VIEW(list),col);}GtkWidget *scroll=gtk_scrolled_window_new(NULL,NULL);gtk_container_add(GTK_CONTAINER(scroll),list);gtk_box_pack_start(GTK_BOX(workBox),scroll,TRUE,TRUE,0);
 noteEdit=entry(workBox,"故障 / 维修说明 / 智能分析问题；修改资料填写 姓名|学号或邮箱",0);
 actionBox=gtk_box_new(GTK_ORIENTATION_VERTICAL,4);gtk_box_pack_start(GTK_BOX(workBox),actionBox,FALSE,FALSE,0);for(int row=0;row<3;row++){GtkWidget *line=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,4);gtk_box_pack_start(GTK_BOX(actionBox),line,FALSE,FALSE,0);for(int col=0;col<5;col++){int i=row*5+col;actions[i]=button(line,actionNames[i],actionIds[i]);}}
 statusText=gtk_label_new("所有预约时段按北京时间计算。");gtk_box_pack_start(GTK_BOX(root),statusText,FALSE,FALSE,0);
 GtkCssProvider *css=gtk_css_provider_new();gtk_css_provider_load_from_data(css,"window { background: #f8f0e2; color: #43230f; } button { padding: 8px; } button:checked { background: #785332; color: white; }",-1,NULL);gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),GTK_STYLE_PROVIDER(css),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);g_object_unref(css);
 g_signal_connect(pageEdit,"changed",G_CALLBACK(change_page),NULL);g_signal_connect(dateEdit,"changed",G_CALLBACK(changed),NULL);g_signal_connect(startEdit,"changed",G_CALLBACK(changed),NULL);g_signal_connect(endEdit,"changed",G_CALLBACK(changed),NULL);
 gtk_widget_show_all(window);render();g_timeout_add_seconds(30,refresh_timer,NULL);g_idle_add(update_start,NULL);
 if(argc>1&&!strcmp(argv[1],"--smoke-test"))g_timeout_add_seconds(5,smoke_quit,NULL);
 gtk_main();lab_model_free(&model);lab_session_clear(&client.session);g_object_unref(rows);curl_global_cleanup();return 0;
}
