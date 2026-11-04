#ifndef LAB_SITE_POLICY_H
#define LAB_SITE_POLICY_H
#include <wchar.h>
#define LAB_SITE_URL L"https://alexis-zhang0503.github.io/intelligent-lab-booking-system/"
#define LAB_APP_VERSION L"2.0.0"
/* Do not attach a native bridge or let a remote redirect take over the app. */
static int lab_is_site_url(const wchar_t *url) {
    const size_t n = wcslen(LAB_SITE_URL);
    return url && wcsncmp(url, LAB_SITE_URL, n) == 0;
}
static int lab_is_https(const wchar_t *url) {
    return url && wcsncmp(url, L"https://", 8) == 0;
}
#endif
