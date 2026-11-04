#include <gtk/gtk.h>
#include <curl/curl.h>
#include "../src/client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../src/ui_theme.h"
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

static GtkWidget *window,*authBox,*workBox,*identity,*password,*nameEdit,*inviteEdit,*dateEdit,*startEdit,*endEdit,*noteEdit,*statusText,*grid,*list,*actionBox;
static GtkWidget *deviceButtons[40],*actions[15];
static GtkListStore *rows;
static GtkWidget *nav[5],*heading,*subtitle,*bookingBox,*filters,*profileBox,*profileName,*todayLabel,*preferenceLabel,*tableBox,*agentBox,*commonActions,*bookingActions,*selectionLabel,*profileFault,*agentReply,*sidebarActions,*sidebarIdentity,*labStatus,*faultActions;
static int preview;
static const char *previewOutput;
static int previewFailed;
static gboolean preview_capture(gpointer unused){(void)unused;GdkWindow *w=gtk_widget_get_window(window);GdkPixbuf *image=gdk_pixbuf_get_from_window(w,0,0,gdk_window_get_width(w),gdk_window_get_height(w));GError *error=NULL;previewFailed=!image||!gdk_pixbuf_save(image,previewOutput,"png",&error,NULL);if(error){g_printerr("%s\n",error->message);g_error_free(error);}if(image)g_object_unref(image);gtk_main_quit();return G_SOURCE_REMOVE;}
static LabClient client;
static LabModel model;
static int busy,logged,page;
static cJSON *listed[100000];
static int listedCount;
static const int actionIds[]={ID_REFRESH,ID_LOGOUT,ID_BOOK,ID_ATTEND,ID_CANCEL,ID_REPORT,ID_REPAIR,ID_ARCHIVE,ID_DEACTIVATE,ID_INVITE,ID_METRICS,ID_UPDATE,ID_PROFILE,ID_SUGGEST,ID_AGENT};
static const char *actionNames[]={"刷新","退出账号","确认预约 →","签到 / 签退","取消预约","报告设备故障","完成维修","归档故障","停用学生","生成邀请码","统计报表","检查版本","修改资料","智能推荐","智能分析"};
static int admin(void){return !strcmp(client.session.role,"admin");}
static int64_t now_ms(void){return g_get_real_time()/1000;}
static void tell(const char *text){GtkWidget *d=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_INFO,GTK_BUTTONS_OK,"%s",text);gtk_dialog_run(GTK_DIALOG(d));gtk_widget_destroy(d);}
static int confirm(const char *text){GtkWidget *d=gtk_message_dialog_new(GTK_WINDOW(window),GTK_DIALOG_MODAL,GTK_MESSAGE_QUESTION,GTK_BUTTONS_YES_NO,"%s",text);gtk_dialog_set_default_response(GTK_DIALOG(d),GTK_RESPONSE_NO);int r=gtk_dialog_run(GTK_DIALOG(d));gtk_widget_destroy(d);return r==GTK_RESPONSE_YES;}
static void render(void){
 gtk_widget_set_visible(authBox,!logged);gtk_widget_set_visible(workBox,logged);gtk_list_store_clear(rows);listedCount=0;
 if(!logged)return;
 gtk_label_set_text(GTK_LABEL(heading),ui_page_title(page));
 gtk_label_set_text(GTK_LABEL(subtitle),page==0?"选定设备，约好时间。":page==1?"个人资料、今日预约和实际使用记录":page==4?"根据设备与时段，寻找合适的预约方案":"NJUPT · 实验室预约");
 gtk_widget_set_visible(bookingBox,page==0);gtk_widget_set_visible(profileBox,page==1);
 gtk_widget_set_visible(tableBox,page!=4);gtk_widget_set_visible(agentBox,page==4);
 for(int i=0;i<5;i++){
   gtk_widget_set_visible(nav[i],(i!=2&&i!=3)||admin());
   GtkStyleContext *style=gtk_widget_get_style_context(nav[i]);
   if(i==page)gtk_style_context_add_class(style,"active");else gtk_style_context_remove_class(style,"active");
 }
 gtk_widget_set_visible(profileFault,page==1||page==2);
 GtkWidget *faultDestination=page==0?faultActions:commonActions;if(gtk_widget_get_parent(actions[5])!=faultDestination){g_object_ref(actions[5]);gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(actions[5])),actions[5]);gtk_box_pack_start(GTK_BOX(faultDestination),actions[5],FALSE,FALSE,0);g_object_unref(actions[5]);}
 gtk_orientable_set_orientation(GTK_ORIENTABLE(actionBox),page==0?GTK_ORIENTATION_VERTICAL:GTK_ORIENTATION_HORIZONTAL);
 GtkWidget *destination=page==0?bookingActions:commonActions;
 if(gtk_widget_get_parent(actionBox)!=destination){g_object_ref(actionBox);gtk_container_remove(GTK_CONTAINER(gtk_widget_get_parent(actionBox)),actionBox);gtk_box_pack_start(GTK_BOX(destination),actionBox,FALSE,FALSE,0);g_object_unref(actionBox);}
 int faults=0;const cJSON *device;cJSON_ArrayForEach(device,model.devices)if(strcmp(lab_str(device,"status"),"available"))++faults;char availability[100];snprintf(availability,sizeof(availability),"%d / 40 台设备可预约\n\n故障设备  %d 台",40-faults,faults);gtk_label_set_text(GTK_LABEL(labStatus),availability);
 for(int i=0;i<15;i++){
 int id=actionIds[i],show=1;
 if(id==ID_BOOK||id==ID_SUGGEST)show=page==0;
 if(id==ID_ATTEND||id==ID_CANCEL)show=page==1;
 if(id==ID_REPORT)show=page==2||page==0;
 if(id==ID_REPAIR||id==ID_ARCHIVE)show=page==2&&admin();
 if(id==ID_DEACTIVATE||id==ID_INVITE||id==ID_METRICS)show=page==3&&admin();
 if(id==ID_PROFILE)show=page==1;
 if(id==ID_AGENT)show=page==4||(page==3&&admin());
 if(id==ID_UPDATE||id==ID_LOGOUT||id==ID_REFRESH)show=1;
 gtk_widget_set_visible(actions[i],show);
 }
 const char *date=gtk_entry_get_text(GTK_ENTRY(dateEdit));int start=8+gtk_combo_box_get_active(GTK_COMBO_BOX(startEdit)),end=9+gtk_combo_box_get_active(GTK_COMBO_BOX(endEdit)),selectedCount=0;
 char today[160],prefs[256],identityText[384];
 GDateTime *utcNow=g_date_time_new_now_utc(),*cstNow=g_date_time_add_hours(utcNow,8);char *todayDate=g_date_time_format(cstNow,"%Y-%m-%d");ui_summary(&model,&client.session,todayDate,today,sizeof(today),prefs,sizeof(prefs));g_free(todayDate);g_date_time_unref(cstNow);g_date_time_unref(utcNow);
 snprintf(identityText,sizeof(identityText),"%s  ·  %s  ·  %s",client.session.name,admin()?"管理员":"学生",client.session.login);
 gtk_label_set_text(GTK_LABEL(sidebarIdentity),identityText);gtk_label_set_text(GTK_LABEL(profileName),identityText);gtk_label_set_text(GTK_LABEL(todayLabel),today);gtk_label_set_text(GTK_LABEL(preferenceLabel),prefs);
 for(int i=0;i<40;i++){
 int fault=0,occupied=0,live=0;const cJSON *r;
 cJSON_ArrayForEach(r,model.devices)if(lab_int(r,"number")==i+1&&strcmp(lab_str(r,"status"),"available"))fault=1;
 cJSON_ArrayForEach(r,model.bookings)if(lab_int(r,"device_number")==i+1){
   if(!strcmp(lab_str(r,"status"),"in_use"))live=1;
   if(!strcmp(lab_str(r,"reservation_date"),date)&&lab_int(r,"start_hour")>=start&&lab_int(r,"start_hour")<end&&(!strcmp(lab_str(r,"status"),"reserved")||!strcmp(lab_str(r,"status"),"confirmed")||!strcmp(lab_str(r,"status"),"in_use")))occupied=1;
 }
 char label[60];snprintf(label,sizeof(label),"%02d\n%s",i+1,fault?"! 故障":live?"使用中":occupied?"已预约":"* 空闲");gtk_button_set_label(GTK_BUTTON(deviceButtons[i]),label);
 GtkStyleContext *style=gtk_widget_get_style_context(deviceButtons[i]);
 const char *classes[]={"faulty","occupied","live"};for(int j=0;j<3;j++)gtk_style_context_remove_class(style,classes[j]);
 if(fault)gtk_style_context_add_class(style,"faulty");else if(live)gtk_style_context_add_class(style,"live");else if(occupied)gtk_style_context_add_class(style,"occupied");
 if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(deviceButtons[i])))++selectedCount;
 }
 char selection[96];snprintf(selection,sizeof(selection),selectedCount?"已选择 %d 台设备":"尚未选择设备",selectedCount);gtk_label_set_text(GTK_LABEL(selectionLabel),selection);
 cJSON *r; cJSON *source=page<2?model.bookings:page==2?model.issues:page==3?model.profiles:NULL;
 cJSON_ArrayForEach(r,source){
 if(page==1&&strcmp(lab_str(r,"user_id"),client.session.uid))continue;
 if(page==0&&(strcmp(lab_str(r,"reservation_date"),date)||(!admin()&&strcmp(lab_str(r,"user_id"),client.session.uid))))continue;
 if(page==2&&!admin()&&strcmp(lab_str(r,"reporter_id"),client.session.uid))continue;
 if(page==3&&!admin())continue;
 if(listedCount>=100000)break;
 char a[128],b[256],c[128],in[80]="",out[80]="",duration[80]="";
 if(page<2){snprintf(a,sizeof(a),"#%02d",lab_int(r,"device_number"));snprintf(b,sizeof(b),"%s %02d:00–%02d:00",lab_str(r,"reservation_date"),lab_int(r,"start_hour"),lab_int(r,"start_hour")+1);snprintf(c,sizeof(c),"%s",ui_status(lab_str(r,"status")));const cJSON *u=ui_usage(&model,r);ui_time(lab_str(u,"checked_in_at"),in,sizeof(in));ui_time(lab_str(u,"checked_out_at"),out,sizeof(out));ui_duration(u,duration,sizeof(duration));}
 else if(page==2){snprintf(a,sizeof(a),"#%02d",lab_int(r,"device_number"));snprintf(b,sizeof(b),"%s",lab_str(r,"description"));snprintf(c,sizeof(c),"%s",ui_status(lab_str(r,"status")));}
 else {snprintf(a,sizeof(a),"%s",lab_str(r,"display_name"));snprintf(b,sizeof(b),"%s",lab_str(r,"login_id"));snprintf(c,sizeof(c),"%s",lab_str(r,"role"));}
 GtkTreeIter it;gtk_list_store_append(rows,&it);gtk_list_store_set(rows,&it,0,a,1,b,2,c,3,in,4,out,5,duration,6,listedCount,-1);listed[listedCount++]=r;
 }
}
static gboolean finish_job(gpointer arg){
 Job *j=arg;busy=0;
 if(j->kind==JOB_UPDATE){if(j->ok&&confirm(j->message))gtk_show_uri_on_window(GTK_WINDOW(window),"https://github.com/alexis-zhang0503/intelligent-lab-booking-system/releases/latest",GDK_CURRENT_TIME,NULL);}
 else {client=j->client;if(j->loaded){lab_model_free(&model);model=j->model;memset(&j->model,0,sizeof(j->model));logged=1;gtk_entry_set_text(GTK_ENTRY(password),"");gtk_entry_set_text(GTK_ENTRY(inviteEdit),"");}if(!*client.session.access){logged=0;lab_model_free(&model);}render();if(*j->message){if(j->kind==JOB_AGENT)gtk_label_set_text(GTK_LABEL(agentReply),j->message);else tell(j->message);}else if(j->ok&&j->kind!=JOB_REFRESH&&j->kind!=JOB_LOGIN)tell("操作成功，已同步云端。");}
 gtk_widget_set_sensitive(authBox,TRUE);gtk_widget_set_sensitive(workBox,TRUE);
 gtk_label_set_text(GTK_LABEL(statusText),"原生 C 3.1.0 · 北京时间 · 每 30 秒刷新云端");
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
 (void)widget;int id=GPOINTER_TO_INT(data);if(busy||preview)return;
 if(id==ID_LOGOUT){lab_session_clear(&client.session);lab_model_free(&model);logged=0;render();return;}
 Job *j=calloc(1,sizeof(*j));if(!j)return;j->client=client;
 snprintf(j->date,sizeof(j->date),"%s",gtk_entry_get_text(GTK_ENTRY(dateEdit)));
 snprintf(j->note,sizeof(j->note),"%s",gtk_entry_get_text(GTK_ENTRY(noteEdit)));
 j->start=8+gtk_combo_box_get_active(GTK_COMBO_BOX(startEdit));j->end=9+gtk_combo_box_get_active(GTK_COMBO_BOX(endEdit));
 for(int i=0;i<40;i++)if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(deviceButtons[i])))j->devices[j->count++]=i+1;
 GtkTreeIter it;GtkTreeModel *tm=NULL;int index=-1;
 if(gtk_tree_selection_get_selected(gtk_tree_view_get_selection(GTK_TREE_VIEW(list)),&tm,&it))gtk_tree_model_get(tm,&it,6,&index,-1);
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
  if(!confirm("确认预约所选设备和时段？多小时逐时提交，若后续冲突会保留已成功的小时。"))goto abort_job;
  break;
 case ID_ATTEND:case ID_CANCEL:case ID_REPAIR:case ID_ARCHIVE:case ID_DEACTIVATE:
  if(!j->row){tell("请先选择列表中的一条记录。");goto abort_job;}
  if(id==ID_ATTEND){j->kind=JOB_ATTEND;j->action=lab_attendance_action(j->row,now_ms());if(strcmp(lab_str(j->row,"user_id"),client.session.uid)||!j->action){tell("仅可对自己的预约在预约小时内签到，使用中可签退。请刷新核实。");goto abort_job;}}
  else j->kind=id==ID_CANCEL?JOB_CANCEL:id==ID_REPAIR?JOB_REPAIR:id==ID_ARCHIVE?JOB_ARCHIVE:JOB_DEACTIVATE;
  if(!confirm(id==ID_ATTEND?(j->action==1?"确认签到所选预约？":"确认签退所选预约？"):"确认对所选记录执行此操作？"))goto abort_job;
  break;
 case ID_REPORT:j->kind=JOB_REPORT;if(!j->count){tell("请在设备预约页先选中故障设备。");goto abort_job;}if(!confirm("确认报告第一台所选设备故障？"))goto abort_job;
  break;
 case ID_INVITE:j->kind=JOB_INVITE;break;
 case ID_METRICS:j->kind=JOB_METRICS;break;
 case ID_PROFILE:j->kind=JOB_PROFILE;if(!confirm("确认按说明框内的 姓名|学号或邮箱 修改本人资料？"))goto abort_job;
  break;
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
static void navigate(GtkWidget *w,gpointer data){(void)w;int next=GPOINTER_TO_INT(data);if(busy)return;if(next==3&&!admin())return;page=next;render();}
static void choose_seat(GtkToggleButton *w,gpointer data){(void)w;(void)data;int n=0;for(int i=0;i<40;i++)if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(deviceButtons[i])))++n;char text[96];snprintf(text,sizeof(text),n?"已选择 %d 台设备":"尚未选择设备",n);gtk_label_set_text(GTK_LABEL(selectionLabel),text);}
static GtkWidget *entry(GtkWidget *box,const char *placeholder,int secret){GtkWidget *e=gtk_entry_new();gtk_entry_set_placeholder_text(GTK_ENTRY(e),placeholder);gtk_entry_set_visibility(GTK_ENTRY(e),!secret);gtk_box_pack_start(GTK_BOX(box),e,FALSE,FALSE,3);return e;}
static GtkWidget *button(GtkWidget *box,const char *title,int id){GtkWidget *b=gtk_button_new_with_label(title);gtk_box_pack_start(GTK_BOX(box),b,FALSE,FALSE,3);g_signal_connect(b,"clicked",G_CALLBACK(command),GINT_TO_POINTER(id));return b;}
static gboolean close_window(GtkWidget *w,GdkEvent *e,gpointer data){(void)w;(void)e;(void)data;if(busy){tell("请求处理中，请等待完成后关闭。");return TRUE;}gtk_main_quit();return FALSE;}
static GtkWidget *label(GtkWidget *box,const char *text,const char *klass){GtkWidget *w=gtk_label_new(text);gtk_label_set_xalign(GTK_LABEL(w),0);gtk_label_set_line_wrap(GTK_LABEL(w),TRUE);if(klass)gtk_style_context_add_class(gtk_widget_get_style_context(w),klass);gtk_box_pack_start(GTK_BOX(box),w,FALSE,TRUE,0);return w;}
static GtkWidget *card(GtkWidget *box,const char *title){GtkWidget *w=gtk_box_new(GTK_ORIENTATION_VERTICAL,14);gtk_style_context_add_class(gtk_widget_get_style_context(w),"card");gtk_box_pack_start(GTK_BOX(box),w,FALSE,TRUE,0);if(title)label(w,title,"card-title");return w;}
int main(int argc,char **argv){
 curl_global_init(CURL_GLOBAL_DEFAULT);gtk_init(&argc,&argv);client.transport=lab_mac_http;
 const char *previewPage=NULL;for(int i=1;i<argc;i++)if(!strncmp(argv[i],"--preview=",10)){preview=1;previewPage=argv[i]+10;}
 for(int i=1;i<argc;i++)if(!strncmp(argv[i],"--preview-output=",17))previewOutput=argv[i]+17;
 window=gtk_window_new(GTK_WINDOW_TOPLEVEL);gtk_window_set_title(GTK_WINDOW(window),"实验室预约 · NJUPT");gtk_window_set_default_size(GTK_WINDOW(window),1280,900);g_signal_connect(window,"delete-event",G_CALLBACK(close_window),NULL);
 GtkWidget *root=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);gtk_container_add(GTK_CONTAINER(window),root);
 authBox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,64);gtk_container_set_border_width(GTK_CONTAINER(authBox),64);gtk_box_pack_start(GTK_BOX(root),authBox,TRUE,TRUE,0);
 GtkWidget *intro=gtk_box_new(GTK_ORIENTATION_VERTICAL,22);gtk_widget_set_valign(intro,GTK_ALIGN_CENTER);gtk_box_pack_start(GTK_BOX(authBox),intro,TRUE,TRUE,0);
 label(intro,"NJUPT · 实验室预约","eyebrow");label(intro,"把时间留给探索","hero-title");label(intro,"选定设备，约好时间。\n设备、时段和你的记录，都在这里。","muted");
 GtkWidget *login=card(authBox,"欢迎回来");gtk_widget_set_size_request(login,360,-1);gtk_widget_set_valign(login,GTK_ALIGN_CENTER);
 identity=entry(login,"学号 / 管理员邮箱",0);password=entry(login,"密码",1);nameEdit=entry(login,"姓名（注册需要）",0);inviteEdit=entry(login,"管理员邀请码（可选）",0);GtkWidget *signIn=button(login,"登录并继续",ID_LOGIN);gtk_style_context_add_class(gtk_widget_get_style_context(signIn),"primary");button(login,"创建账号",ID_REGISTER);label(login,"使用与网页相同的云端账号","muted");
 workBox=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,0);gtk_box_pack_start(GTK_BOX(root),workBox,TRUE,TRUE,0);
 GtkWidget *sidebar=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);gtk_widget_set_size_request(sidebar,UI_SIDEBAR,-1);gtk_style_context_add_class(gtk_widget_get_style_context(sidebar),"sidebar");gtk_box_pack_start(GTK_BOX(workBox),sidebar,FALSE,FALSE,0);
 GtkWidget *brand=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);gtk_box_pack_start(GTK_BOX(sidebar),brand,FALSE,FALSE,0);char logoPath[1024];snprintf(logoPath,sizeof(logoPath),"%s/njupt-lab-reservation-logo.png",g_getenv("LAB_RESOURCE_DIR")?g_getenv("LAB_RESOURCE_DIR"):"assets/brand");GdkPixbuf *logo=gdk_pixbuf_new_from_file_at_scale(logoPath,28,28,TRUE,NULL);if(logo){GtkWidget *icon=gtk_image_new_from_pixbuf(logo);gtk_box_pack_start(GTK_BOX(brand),icon,FALSE,FALSE,0);g_object_unref(logo);}label(brand,"NJUPT 实验室预约","brand");const int order[]={0,4,1,2,3};const char *names[]={"预约","我的","故障记录","用户管理","智能助手"};for(int k=0;k<5;k++){int i=order[k];nav[i]=gtk_button_new_with_label(names[i]);gtk_widget_set_halign(nav[i],GTK_ALIGN_FILL);gtk_style_context_add_class(gtk_widget_get_style_context(nav[i]),"nav");gtk_box_pack_start(GTK_BOX(sidebar),nav[i],FALSE,FALSE,0);g_signal_connect(nav[i],"clicked",G_CALLBACK(navigate),GINT_TO_POINTER(i));}
 GtkWidget *spacer=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);gtk_box_pack_start(GTK_BOX(sidebar),spacer,TRUE,TRUE,0);sidebarIdentity=label(sidebar,"NJUPT · 云端预约","muted");sidebarActions=gtk_box_new(GTK_ORIENTATION_VERTICAL,6);gtk_box_pack_start(GTK_BOX(sidebar),sidebarActions,FALSE,FALSE,0);
 GtkWidget *viewport=gtk_scrolled_window_new(NULL,NULL);gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(viewport),GTK_POLICY_AUTOMATIC,GTK_POLICY_AUTOMATIC);gtk_box_pack_start(GTK_BOX(workBox),viewport,TRUE,TRUE,0);
 GtkWidget *content=gtk_box_new(GTK_ORIENTATION_VERTICAL,12);gtk_container_set_border_width(GTK_CONTAINER(content),32);gtk_container_add(GTK_CONTAINER(viewport),content);
 label(content,"NJUPT · 实验室预约","eyebrow");heading=label(content,"实验室预约","page-title");subtitle=label(content,"把时间留给探索","muted");
 bookingBox=gtk_box_new(GTK_ORIENTATION_VERTICAL,20);gtk_box_pack_start(GTK_BOX(content),bookingBox,FALSE,FALSE,0);
 GtkWidget *hero=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,16);gtk_box_pack_start(GTK_BOX(bookingBox),hero,FALSE,FALSE,0);GtkWidget *heroText=label(hero,"把时间留给探索","hero-title");gtk_widget_set_hexpand(heroText,TRUE);GtkWidget *guide=card(hero,"预约流程 · 三步完成");gtk_widget_set_size_request(guide,292,-1);label(guide,"01 选择设备   02 设置时间\n03 确认预约","muted");
 GtkWidget *booking=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,16);gtk_box_pack_start(GTK_BOX(bookingBox),booking,FALSE,FALSE,0);
 GtkWidget *lab=gtk_box_new(GTK_ORIENTATION_VERTICAL,14);gtk_widget_set_hexpand(lab,TRUE);gtk_style_context_add_class(gtk_widget_get_style_context(lab),"card");gtk_box_pack_start(GTK_BOX(booking),lab,TRUE,TRUE,0);label(lab,"实验室平面图","card-title");label(lab,"点击空闲设备","muted");
 filters=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,6);gtk_box_pack_start(GTK_BOX(lab),filters,FALSE,FALSE,0);dateEdit=entry(filters,"预约日期 YYYY-MM-DD",0);GDateTime *now=g_date_time_new_now_utc(),*bj=g_date_time_add_hours(now,8);char *date=g_date_time_format(bj,"%Y-%m-%d");gtk_entry_set_text(GTK_ENTRY(dateEdit),date);g_free(date);g_date_time_unref(bj);g_date_time_unref(now);label(lab,"* 空闲    已预约    使用中    ! 故障    已选择","muted");
 grid=gtk_grid_new();gtk_grid_set_row_spacing(GTK_GRID(grid),12);gtk_grid_set_column_spacing(GTK_GRID(grid),8);gtk_box_pack_start(GTK_BOX(lab),grid,FALSE,FALSE,0);GtkWidget *aisle=gtk_box_new(GTK_ORIENTATION_VERTICAL,0);gtk_widget_set_size_request(aisle,24,-1);gtk_grid_attach(GTK_GRID(grid),aisle,4,0,1,5);for(int i=0;i<40;i++){deviceButtons[i]=gtk_toggle_button_new_with_label("设备");gtk_widget_set_hexpand(deviceButtons[i],TRUE);gtk_widget_set_size_request(deviceButtons[i],56,68);gtk_style_context_add_class(gtk_widget_get_style_context(deviceButtons[i]),"seat");gtk_grid_attach(GTK_GRID(grid),deviceButtons[i],i%UI_DEVICE_COLUMNS+(i%UI_DEVICE_COLUMNS>=4?1:0),i/UI_DEVICE_COLUMNS,1,1);g_signal_connect(deviceButtons[i],"toggled",G_CALLBACK(choose_seat),NULL);}
 GtkWidget *railColumn=gtk_box_new(GTK_ORIENTATION_VERTICAL,16);gtk_box_pack_start(GTK_BOX(booking),railColumn,FALSE,FALSE,0);GtkWidget *rail=gtk_box_new(GTK_ORIENTATION_VERTICAL,14);gtk_widget_set_size_request(rail,292,-1);gtk_widget_set_valign(rail,GTK_ALIGN_START);gtk_style_context_add_class(gtk_widget_get_style_context(rail),"card");gtk_box_pack_start(GTK_BOX(railColumn),rail,FALSE,FALSE,0);label(rail,"预约时间","card-title");
 startEdit=gtk_combo_box_text_new();endEdit=gtk_combo_box_text_new();for(int i=0;i<9;i++){char t[32];snprintf(t,sizeof(t),"开始 %02d:00",i+8);gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(startEdit),t);snprintf(t,sizeof(t),"结束 %02d:00",i+9);gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(endEdit),t);}gtk_combo_box_set_active(GTK_COMBO_BOX(startEdit),0);gtk_combo_box_set_active(GTK_COMBO_BOX(endEdit),0);GtkWidget *timeRow=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,8);gtk_box_pack_start(GTK_BOX(rail),timeRow,FALSE,FALSE,0);gtk_box_pack_start(GTK_BOX(timeRow),startEdit,TRUE,TRUE,0);label(timeRow,"至","muted");gtk_box_pack_start(GTK_BOX(timeRow),endEdit,TRUE,TRUE,0);selectionLabel=label(rail,"尚未选择设备","selection");bookingActions=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);gtk_box_pack_start(GTK_BOX(rail),bookingActions,FALSE,FALSE,0);
 GtkWidget *stateCard=card(railColumn,"实验室状态");labStatus=label(stateCard,"","card-title");faultActions=gtk_box_new(GTK_ORIENTATION_VERTICAL,6);gtk_box_pack_start(GTK_BOX(railColumn),faultActions,FALSE,FALSE,0);
 profileBox=gtk_box_new(GTK_ORIENTATION_VERTICAL,16);gtk_box_pack_start(GTK_BOX(content),profileBox,FALSE,FALSE,0);GtkWidget *profile=card(profileBox,NULL);profileName=label(profile,"","card-title");label(profile,"个人资料 · 使用与网页相同的账号","muted");GtkWidget *summary=gtk_box_new(GTK_ORIENTATION_HORIZONTAL,16);gtk_box_pack_start(GTK_BOX(profileBox),summary,FALSE,FALSE,0);GtkWidget *today=card(summary,"今天的预约");gtk_widget_set_hexpand(today,TRUE);todayLabel=label(today,"","body");GtkWidget *pref=card(summary,"预约偏好");gtk_widget_set_hexpand(pref,TRUE);preferenceLabel=label(pref,"","muted");profileFault=gtk_button_new_with_label("我的故障记录");gtk_box_pack_start(GTK_BOX(profileBox),profileFault,FALSE,FALSE,0);g_signal_connect(profileFault,"clicked",G_CALLBACK(navigate),GINT_TO_POINTER(2));
 agentBox=card(content,"智能助手");label(agentBox,"根据设备、日期和时段获取建议；确认后再预约。","muted");agentReply=label(agentBox,"告诉我你的预约需求。","body");gtk_widget_set_size_request(agentReply,-1,200);
 commonActions=gtk_box_new(GTK_ORIENTATION_VERTICAL,8);gtk_box_pack_start(GTK_BOX(content),commonActions,FALSE,FALSE,0);noteEdit=entry(commonActions,"故障 / 维修说明或智能助手问题；修改资料：姓名|学号或邮箱",0);
 actionBox=gtk_box_new(GTK_ORIENTATION_VERTICAL,6);gtk_box_pack_start(GTK_BOX(commonActions),actionBox,FALSE,FALSE,0);for(int i=0;i<15;i++){actions[i]=button(actionIds[i]==ID_REFRESH||actionIds[i]==ID_LOGOUT||actionIds[i]==ID_UPDATE?sidebarActions:actionBox,actionNames[i],actionIds[i]);if(actionIds[i]==ID_BOOK||actionIds[i]==ID_AGENT)gtk_style_context_add_class(gtk_widget_get_style_context(actions[i]),"primary");}
 tableBox=card(content,"预约与使用记录");label(tableBox,"选中预约后，可手动签到、签退或取消。实际时间按北京时间显示。","muted");rows=gtk_list_store_new(7,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_INT);list=gtk_tree_view_new_with_model(GTK_TREE_MODEL(rows));const char *columns[]={"设备 / 姓名","预约时段 / 说明 / 账号","状态","实际签到","实际签退","实际使用"};for(int i=0;i<6;i++){GtkCellRenderer *cell=gtk_cell_renderer_text_new();GtkTreeViewColumn *col=gtk_tree_view_column_new_with_attributes(columns[i],cell,"text",i,NULL);gtk_tree_view_column_set_resizable(col,TRUE);gtk_tree_view_append_column(GTK_TREE_VIEW(list),col);}GtkWidget *scroll=gtk_scrolled_window_new(NULL,NULL);gtk_widget_set_size_request(scroll,-1,230);gtk_container_add(GTK_CONTAINER(scroll),list);gtk_box_pack_start(GTK_BOX(tableBox),scroll,TRUE,TRUE,0);
 statusText=gtk_label_new("所有预约时段按北京时间计算。");gtk_box_pack_start(GTK_BOX(root),statusText,FALSE,FALSE,8);
 GtkCssProvider *css=gtk_css_provider_new();gtk_css_provider_load_from_data(css,
 "* { font-family: 'Noto Sans CJK SC', 'PingFang SC', sans-serif; font-size: 13px; color: " UI_INK "; } window { background: " UI_BG "; } .sidebar { padding:20px; background: white; border-right: 1px solid " UI_BORDER "; } .brand { font-size:16px; font-weight:700; margin-bottom:24px; } .card { padding:22px; background: " UI_SURFACE "; border:1px solid " UI_BORDER "; border-radius:16px; } .page-title { font-size:28px; font-weight:700; } .hero-title { font-size:30px; font-weight:700; } .card-title { font-size:16px; font-weight:700; } .eyebrow,.muted { color: " UI_MUTED "; } .eyebrow { font-size:11px; } button { background:white; border:1px solid " UI_BORDER "; border-radius:8px; padding:9px 12px; box-shadow:none; } button:hover { background:#f2f2ef; } .nav { border:0; background:transparent; min-height:28px; } .nav.active { background:#f2f2ef; border-left:3px solid " UI_ACCENT "; } .primary { background: " UI_ACCENT "; color:white; } .primary label { color:white; } entry,combobox button { background:white; border:1px solid " UI_BORDER "; border-radius:8px; padding:9px; } .selection { background: " UI_SOFT "; padding:12px; border-radius:8px; } .seat { background:#fcfcfb; padding:6px; } .seat:checked { background: " UI_SOFT "; border:2px solid " UI_ACCENT "; } .seat.faulty { background:#fcf0ef; border-color:#e9c2bf; } .seat.faulty label { color: " UI_DANGER "; } .seat.occupied { background:#f2f2ef; } .seat.live { background:#e8f6fa; } treeview { background:white; } treeview:selected { background:" UI_SOFT "; color:" UI_INK "; } header button { background:#fcfcfb; border-radius:0; }",-1,NULL);gtk_style_context_add_provider_for_screen(gdk_screen_get_default(),GTK_STYLE_PROVIDER(css),GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);g_object_unref(css);
 g_signal_connect(dateEdit,"changed",G_CALLBACK(changed),NULL);g_signal_connect(startEdit,"changed",G_CALLBACK(changed),NULL);g_signal_connect(endEdit,"changed",G_CALLBACK(changed),NULL);
 gtk_widget_show_all(root);
 if(preview){ui_preview_model(&client,&model);logged=strcmp(previewPage,"login")!=0;page=!strcmp(previewPage,"profile")?1:!strcmp(previewPage,"agent")?4:0;gtk_label_set_text(GTK_LABEL(statusText),"界面预览 · 示例数据 · 未连接真实账号");}
 render();gtk_widget_show(window);if(preview&&previewOutput)g_timeout_add(2000,preview_capture,NULL);if(!preview){g_timeout_add_seconds(30,refresh_timer,NULL);g_idle_add(update_start,NULL);}
 if(argc>1&&!strcmp(argv[1],"--smoke-test"))g_timeout_add_seconds(5,smoke_quit,NULL);
 gtk_main();lab_model_free(&model);lab_session_clear(&client.session);g_object_unref(rows);curl_global_cleanup();return previewFailed?2:0;
}
