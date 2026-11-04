#ifndef LAB_UI_THEME_H
#define LAB_UI_THEME_H
#include <time.h>
/* Native equivalents of the live web workspace tokens. No HTML renderer. */
#define UI_BG "#f7f7f5"
#define UI_SURFACE "#ffffff"
#define UI_INK "#23252b"
#define UI_MUTED "#686b73"
#define UI_BORDER "#e8e8e3"
#define UI_ACCENT "#5c63d6"
#define UI_SOFT "#eff0ff"
#define UI_SUCCESS "#398563"
#define UI_DANGER "#bd5252"
#define UI_SIDEBAR 228
#define UI_RADIUS 16
#define UI_DEVICE_COLUMNS 8
static const char *ui_page_title(int page) {
    return page == 0 ? "实验室预约" : page == 1 ? "我的" : page == 2 ? "故障记录" : page == 3 ? "用户管理" : "智能助手";
}
static const char *ui_status(const char *s) {
    if (!strcmp(s,"confirmed") || !strcmp(s,"reserved")) return "已预约";
    if (!strcmp(s,"in_use")) return "使用中";
    if (!strcmp(s,"completed")) return "已完成";
    if (!strcmp(s,"cancelled")) return "已取消";
    if (!strcmp(s,"no_show")) return "未到场";
    if (!strcmp(s,"resolved")) return "已修复";
    if (!strcmp(s,"reported") || !strcmp(s,"open")) return "待处理";
    return s;
}
static const cJSON *ui_usage(const LabModel *m,const cJSON *booking) {
    const cJSON *u; cJSON_ArrayForEach(u,m->usage)
        if (!strcmp(lab_str(u,"reservation_id"),lab_str(booking,"id"))) return u;
    return NULL;
}
static void ui_summary(const LabModel *m,const LabSession *s,const char *date,char *today,size_t tn,char *prefs,size_t pn) {
    int count=0,total=0,devices[40]={0},hours[9]={0},bestD=0,bestH=0; const cJSON *b;
    cJSON_ArrayForEach(b,m->bookings) {
        if(strcmp(lab_str(b,"user_id"),s->uid)||!strcmp(lab_str(b,"status"),"cancelled"))continue;
        ++total;if(!strcmp(lab_str(b,"reservation_date"),date))++count;
        int d=lab_int(b,"device_number")-1,h=lab_int(b,"start_hour")-8;
        if(d>=0&&d<40)++devices[d];
        if(h>=0&&h<9)++hours[h];
    }
    for(int i=1;i<40;i++)if(devices[i]>devices[bestD])bestD=i;
    for(int i=1;i<9;i++)if(hours[i]>hours[bestH])bestH=i;
    snprintf(today,tn,"%s · 今天 %d 个预约时段",date,count);
    if(total)snprintf(prefs,pn,"常用设备 #%02d    常用时段 %02d:00    累计预约 %d 小时",bestD+1,bestH+8,total);
    else snprintf(prefs,pn,"暂未形成预约偏好 · 选好设备和时段后，会显示在这里。");
}
static int64_t ui_timestamp(const char *value) {
    int y,mo,d,h,mi,se;
    if(strlen(value)<19)return 0;
    if(sscanf(value,"%d-%d-%dT%d:%d:%d",&y,&mo,&d,&h,&mi,&se)!=6)return 0;
    char date[32];snprintf(date,sizeof(date),"%04d-%02d-%02d",y,mo,d);
    cJSON *b=cJSON_CreateObject();cJSON_AddStringToObject(b,"reservation_date",date);cJSON_AddNumberToObject(b,"start_hour",8);
    int64_t t=lab_booking_start(b)+(int64_t)(h*3600+mi*60+se)*1000;cJSON_Delete(b);
    const char *z=value+19;while(*z=='.'||(*z>='0'&&*z<='9'))++z;
    int zh=0,zm=0;if((*z=='+'||*z=='-')&&sscanf(z+1,"%d:%d",&zh,&zm)>=1)t-=(int64_t)(*z=='+'?1:-1)*(zh*60+zm)*60000;
    return t;
}
static void ui_time(const char *value,char *out,size_t n) {
    int64_t t=ui_timestamp(value);if(!t){snprintf(out,n,"—");return;}
    time_t seconds=(time_t)(t/1000+8*3600);struct tm *tm=gmtime(&seconds);
    if(tm)strftime(out,n,"%Y-%m-%d %H:%M",tm);else snprintf(out,n,"—");
}
static void ui_duration(const cJSON *usage,char *out,size_t n) {
    int64_t in=ui_timestamp(lab_str(usage,"checked_in_at")),end=ui_timestamp(lab_str(usage,"checked_out_at"));
    if(in&&end>=in)snprintf(out,n,"%lld 分钟",(long long)((end-in)/60000));
    else snprintf(out,n,"%s",in?"进行中":"—");
}
static void ui_preview_model(LabClient *c,LabModel *m) {
    snprintf(c->session.uid,sizeof(c->session.uid),"preview-user");
    snprintf(c->session.name,sizeof(c->session.name),"示例用户");
    snprintf(c->session.login,sizeof(c->session.login),"B000001");
    snprintf(c->session.role,sizeof(c->session.role),"student");
    m->devices=cJSON_CreateArray();for(int i=1;i<=40;i++){
        cJSON *d=cJSON_CreateObject();cJSON_AddNumberToObject(d,"number",i);
        cJSON_AddStringToObject(d,"status",i==6?"faulty":"available");cJSON_AddItemToArray(m->devices,d);
    }
    m->bookings=cJSON_Parse("[{\"id\":\"preview-1\",\"user_id\":\"preview-user\",\"device_number\":12,\"reservation_date\":\"2026-11-04\",\"start_hour\":9,\"status\":\"completed\"},{\"id\":\"preview-2\",\"user_id\":\"preview-user\",\"device_number\":18,\"reservation_date\":\"2026-11-05\",\"start_hour\":14,\"status\":\"confirmed\"}]");
    m->usage=cJSON_Parse("[{\"reservation_id\":\"preview-1\",\"checked_in_at\":\"2026-11-04T01:02:00+00:00\",\"checked_out_at\":\"2026-11-04T01:52:00+00:00\"}]");
    m->profiles=cJSON_CreateArray();m->issues=cJSON_CreateArray();
}
#endif
