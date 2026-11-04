#include "../src/client.h"
#include <curl/curl.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
typedef struct { char *data; size_t size; } Buffer;
static size_t receive(char *p,size_t s,size_t n,void *ctx) {
    Buffer *b=ctx; size_t bytes=s*n;
    if (bytes>16*1024*1024-b->size) return 0;
    char *q=realloc(b->data,b->size+bytes+1); if(!q)return 0;
    b->data=q; memcpy(q+b->size,p,bytes); b->size+=bytes; q[b->size]=0; return bytes;
}
char *lab_mac_http(void *ctx,const char *host,const char *method,const char *path,const char *key,const char *token,const char *body,int *status,char *error,size_t cap) {
    (void)ctx; *status=0; error[0]=0;
    CURL *c=curl_easy_init(); if(!c){snprintf(error,cap,"无法初始化 HTTPS");return NULL;}
    char url[2048], header[8400]; Buffer b={calloc(1,1),0}; struct curl_slist *headers=NULL;
    snprintf(url,sizeof(url),"https://%s%s",host,path);
    headers=curl_slist_append(headers,"Content-Type: application/json");
    if(*key){snprintf(header,sizeof(header),"apikey: %s",key);headers=curl_slist_append(headers,header);}
    if(*token){snprintf(header,sizeof(header),"Authorization: Bearer %s",token);headers=curl_slist_append(headers,header);}
    curl_easy_setopt(c,CURLOPT_URL,url); curl_easy_setopt(c,CURLOPT_HTTPHEADER,headers);
    curl_easy_setopt(c,CURLOPT_CUSTOMREQUEST,method);
    if(strcmp(method,"GET"))curl_easy_setopt(c,CURLOPT_POSTFIELDS,body);
    curl_easy_setopt(c,CURLOPT_CONNECTTIMEOUT,15L);curl_easy_setopt(c,CURLOPT_TIMEOUT,60L);
    curl_easy_setopt(c,CURLOPT_NOSIGNAL,1L);curl_easy_setopt(c,CURLOPT_WRITEFUNCTION,receive);curl_easy_setopt(c,CURLOPT_WRITEDATA,&b);
    CURLcode result=curl_easy_perform(c);long code=0;curl_easy_getinfo(c,CURLINFO_RESPONSE_CODE,&code);*status=(int)code;
    if(result!=CURLE_OK){snprintf(error,cap,"HTTPS: %s",curl_easy_strerror(result));free(b.data);b.data=NULL;}
    curl_slist_free_all(headers);curl_easy_cleanup(c);return b.data;
}
