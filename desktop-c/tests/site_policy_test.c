#include "../src/site_policy.h"
#include <assert.h>
int main(void) {
    assert(lab_is_site_url(LAB_SITE_URL));
    assert(lab_is_site_url(LAB_SITE_URL L"?desktop=1&launch=123"));
    assert(!lab_is_site_url(L"https://alexis-zhang0503.github.io.evil.test/intelligent-lab-booking-system/"));
    assert(!lab_is_site_url(L"https://evil.test/"));
    assert(!lab_is_site_url(L"https://alexis-zhang0503.github.io/other-project/"));
    assert(!lab_is_site_url(L"file:///C:/Windows/System32/"));
    assert(!lab_is_site_url(NULL));
    assert(!lab_is_https(L"javascript:alert(1)"));
    assert(lab_is_https(L"https://learn.microsoft.com/"));
    return 0;
}
