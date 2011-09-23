
#include <stdio.h>  /* snprintf */
#include <cutter.h> /* cutter library */

#include "log.h"
#include "calc.h"

struct test_func func;

void
cut_setup(void)
{
    test_initfunc(&func);
}

void
cut_teardown(void)
{

}

void test_get_strlen()
{
    int retval = 0;            /* 戻り値 */
    char fmt[sizeof("%.15g")]; /* フォーマット */

    retval = snprintf(fmt, sizeof(fmt), "%s", "%.18g");
    if (retval < 0) {
        outlog("snprintf=%d", retval);
        return;
    }
    dbglog("fmt=%s", fmt);

    double val = 1234567.89012345;
    retval = func.get_strlen(val, fmt);
    dbglog("retval=%d", retval);
    cut_assert_equal_int(5, func.get_strlen(50000, fmt));
    cut_assert_equal_int(15, func.get_strlen(123456789012345, fmt));
    //cut_assert_equal_int(10, func.get_strlen(12345678.9, fmt));
    //cut_assert_equal_int(16, _test_get_strlen(1234567.89012345, fmt));
}

