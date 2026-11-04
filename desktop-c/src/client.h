#ifndef LAB_NATIVE_CLIENT_H
#define LAB_NATIVE_CLIENT_H
#include "../vendor/cJSON.h"
#include <stddef.h>
#include <stdint.h>
#define LAB_NATIVE_VERSION "3.1.0"
#define LAB_API_HOST "ikfqcappastopzjbncea.supabase.co"
#define LAB_PUBLIC_KEY "sb_publishable_5Q5xIYHsAeEfO195kJ7jNg_Z5uBwxkV"
#define LAB_SITE_HOST "alexis-zhang0503.github.io"
#define LAB_SITE_BASE "/intelligent-lab-booking-system/"
typedef char *(*LabTransport)(void *, const char *, const char *, const char *, const char *, const char *,
                              const char *, int *, char *, size_t);
typedef struct {
    char access[8192], refresh[4096], uid[64], name[128], login[128], role[16];
} LabSession;
typedef struct {
    cJSON *devices, *bookings, *profiles, *issues, *usage;
} LabModel;
typedef struct {
    LabTransport transport;
    void *context;
    LabSession session;
    char error[1024];
    int status;
} LabClient;
const char *lab_str(const cJSON *, const char *);
int lab_int(const cJSON *, const char *);
void lab_model_free(LabModel *);
void lab_session_clear(LabSession *);
cJSON *lab_request(LabClient *, const char *, const char *, const cJSON *);
cJSON *lab_rpc(LabClient *, const char *, const cJSON *);
int lab_login(LabClient *, const char *, const char *);
int lab_register(LabClient *, const char *, const char *, const char *, const char *);
int lab_load_model(LabClient *, LabModel *);
int64_t lab_booking_start(const cJSON *);
int lab_attendance_action(const cJSON *, int64_t);
int lab_attend(LabClient *, const cJSON *, int, int64_t);
int lab_reserve_range(LabClient *, const char *, int, int, const int *, size_t, const char *keys[9], int *);
int lab_semver_newer(const char *, const char *);
#endif
