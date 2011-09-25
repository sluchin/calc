/**
 * @file  calc/tests/test_calc.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-09-24 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011 Tetsuya Higashi. All Rights Reserved.
 */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 */

#include <stdio.h>  /* snprintf */
#include <string.h> /* strdup */
#include <cutter.h> /* cutter library */

#include "def.h"
#include "util.h"
#include "log.h"
#include "calc.h"

struct test_calc_func func;

/* 内部変数 */
static uchar *expr = NULL;   /* 式 */
static uchar *result = NULL; /* 結果 */

#define CALCFREE destroy_calc();                \
    memfree(1, &expr);                          \
    expr = result = NULL

/* 内部関数 */
/** バッファセット */
static uchar *set_buf(char *str);

void
cut_setup(void)
{
    test_init_calc(&func);
}

void
cut_teardown(void)
{

}

void test_answer()
{
    cut_assert_equal_string("421", (char *)set_buf("(105+312)+2*(5-3)"));
    CALCFREE;
    cut_assert_equal_string("418", (char *)set_buf("(105+312)+2/(5-3)"));
    CALCFREE;
    cut_assert_equal_string("5", (char *)set_buf("1+2*(5-3)"));
    CALCFREE;
    cut_assert_equal_string("2", (char *)set_buf("1+2/(5-3)"));
    CALCFREE;
    cut_assert_equal_string("3.14159265359", (char *)set_buf("pi"));
    CALCFREE;
    cut_assert_equal_string("2.71828182846", (char *)set_buf("e"));
    CALCFREE;
    cut_assert_equal_string("2", (char *)set_buf("abs(-2)"));
    CALCFREE;
    cut_assert_equal_string("1.41421356237", (char *)set_buf("sqrt(2)"));
    CALCFREE;
    cut_assert_equal_string("0.909297426826", (char *)set_buf("sin(2)"));
    CALCFREE;
    cut_assert_equal_string("-0.416146836547", (char *)set_buf("cos(2)"));
    CALCFREE;
    cut_assert_equal_string("-2.18503986326", (char *)set_buf("tan(2)"));
    CALCFREE;
    cut_assert_equal_string("0.523598775598", (char *)set_buf("asin(0.5)"));
    CALCFREE;
    cut_assert_equal_string("1.0471975512", (char *)set_buf("acos(0.5)"));
    CALCFREE;
    cut_assert_equal_string("0.463647609001", (char *)set_buf("atan(0.5)"));
    CALCFREE;
    cut_assert_equal_string("7.38905609893", (char *)set_buf("exp(2)"));
    CALCFREE;
    cut_assert_equal_string("0.69314718056", (char *)set_buf("ln(2)"));
    CALCFREE;
    cut_assert_equal_string("0.301029995664", (char *)set_buf("log(2)"));
    CALCFREE;
    cut_assert_equal_string("114.591559026", (char *)set_buf("deg(2)"));
    CALCFREE;
    cut_assert_equal_string("0.0349065850399", (char *)set_buf("rad(2)"));
    CALCFREE;
    cut_assert_equal_string("3628800", (char *)set_buf("n(10)"));
    CALCFREE;
    cut_assert_equal_string("20", (char *)set_buf("nPr(5,2)"));
    CALCFREE;
    cut_assert_equal_string("10", (char *)set_buf("nCr(5,2)"));
    CALCFREE;
    cut_assert_equal_string("15.7079632679", (char *)set_buf("5*pi"));
    CALCFREE;
    cut_assert_equal_string("15.7079632679", (char *)set_buf("pi*5"));
    CALCFREE;
    cut_assert_equal_string("13.5914091423", (char *)set_buf("5*e"));
    CALCFREE;
    cut_assert_equal_string("13.5914091423", (char *)set_buf("e*5"));
    CALCFREE;
    cut_assert_equal_string("10", (char *)set_buf("5*abs(-2)"));
    CALCFREE;
    cut_assert_equal_string("10", (char *)set_buf("abs(-2)*5"));
    CALCFREE;
    cut_assert_equal_string("7.07106781187", (char *)set_buf("5*sqrt(2)"));
    CALCFREE;
    cut_assert_equal_string("7.07106781187", (char *)set_buf("sqrt(2)*5"));
    CALCFREE;
    cut_assert_equal_string("4.54648713413", (char *)set_buf("5*sin(2)"));
    CALCFREE;
    cut_assert_equal_string("4.54648713413", (char *)set_buf("sin(2)*5"));
    CALCFREE;
    cut_assert_equal_string("-2.08073418274", (char *)set_buf("5*cos(2)"));
    CALCFREE;
    cut_assert_equal_string("-2.08073418274", (char *)set_buf("cos(2)*5"));
    CALCFREE;
    cut_assert_equal_string("-10.9251993163", (char *)set_buf("5*tan(2)"));
    CALCFREE;
    cut_assert_equal_string("-10.9251993163", (char *)set_buf("tan(2)*5"));
    CALCFREE;
    cut_assert_equal_string("1.0471975512", (char *)set_buf("2*asin(0.5)"));
    CALCFREE;
    cut_assert_equal_string("1.0471975512", (char *)set_buf("asin(0.5)*2"));
    CALCFREE;
    cut_assert_equal_string("2.09439510239", (char *)set_buf("2*acos(0.5)"));
    CALCFREE;
    cut_assert_equal_string("2.09439510239", (char *)set_buf("acos(0.5)*2"));
    CALCFREE;
    cut_assert_equal_string("2.318238045", (char *)set_buf("5*atan(0.5)"));
    CALCFREE;
    cut_assert_equal_string("2.318238045", (char *)set_buf("atan(0.5)*5"));
    CALCFREE;
    cut_assert_equal_string("36.9452804947", (char *)set_buf("5*exp(2)"));
    CALCFREE;
    cut_assert_equal_string("36.9452804947", (char *)set_buf("exp(2)*5"));
    CALCFREE;
    cut_assert_equal_string("3.4657359028", (char *)set_buf("5*ln(2)"));
    CALCFREE;
    cut_assert_equal_string("3.4657359028", (char *)set_buf("ln(2)*5"));
    CALCFREE;
    cut_assert_equal_string("1.50514997832", (char *)set_buf("5*log(2)"));
    CALCFREE;
    cut_assert_equal_string("1.50514997832", (char *)set_buf("log(2)*5"));
    CALCFREE;
    cut_assert_equal_string("572.957795131", (char *)set_buf("5*deg(2)"));
    CALCFREE;
    cut_assert_equal_string("572.957795131", (char *)set_buf("deg(2)*5"));
    CALCFREE;
    cut_assert_equal_string("0.174532925199", (char *)set_buf("5*rad(2)"));
    CALCFREE;
    cut_assert_equal_string("0.174532925199", (char *)set_buf("rad(2)*5"));
    CALCFREE;
    cut_assert_equal_string("18144000", (char *)set_buf("5*n(10)"));
    CALCFREE;
    cut_assert_equal_string("18144000", (char *)set_buf("n(10)*5"));
    CALCFREE;
    cut_assert_equal_string("100", (char *)set_buf("5*nPr(5,2)"));
    CALCFREE;
    cut_assert_equal_string("100", (char *)set_buf("nPr(5,2)*5"));
    CALCFREE;
    cut_assert_equal_string("50", (char *)set_buf("5*nCr(5,2)"));
    CALCFREE;
    cut_assert_equal_string("4.43749076323e+14", (char *)set_buf("nCr(50,22)*5"));
    CALCFREE;
}

void test_get_strlen()
{
    cut_assert_equal_int(5, func.get_strlen(50000, "%.18g"));
    cut_assert_equal_int(15, func.get_strlen(123456789012345, "%.18g"));
    // 12345678.9000000004
    cut_assert_equal_int(19, func.get_strlen(12345678.9, "%.18g"));
    // 1234567.89012344996
    cut_assert_equal_int(19, func.get_strlen(1234567.89012345, "%.18g"));

    cut_assert_equal_int(5, func.get_strlen(50000, "%.15g"));
    cut_assert_equal_int(15, func.get_strlen(123456789012345, "%.15g"));
    // 12345678.9
    cut_assert_equal_int(10, func.get_strlen(12345678.9, "%.15g"));
    // 1234567.89012345
    cut_assert_equal_int(16, func.get_strlen(1234567.89012345, "%.15g"));

    // 5e+04
    cut_assert_equal_int(5, func.get_strlen(50000, "%.1g"));
    // 1e+14
    cut_assert_equal_int(5, func.get_strlen(123456789012345, "%.1g"));
    // 1e+07
    cut_assert_equal_int(5, func.get_strlen(12345678.9, "%.1g"));
    // 1e+06
    cut_assert_equal_int(5, func.get_strlen(1234567.89012345, "%.1g"));

    cut_assert_equal_int(5, func.get_strlen(50000, "%.30g"));
    cut_assert_equal_int(15, func.get_strlen(123456789012345, "%.30g"));
    // 12345678.9000000003725290298462
    cut_assert_equal_int(31, func.get_strlen(12345678.9, "%.30g"));
    // 1234567.89012344996444880962372
    cut_assert_equal_int(31, func.get_strlen(1234567.89012345, "%.30g"));
}

/**
 * バッファセット
 *
 * @param[in] str 文字列
 * @return なし
 */
static uchar *
set_buf(char *str)
{
    size_t length = 0; /* 文字列長 */

    length = strlen(str);
    expr = (uchar *)strndup(str, length);
    if (!expr) {
        outlog("strndup=%p", expr);
        return NULL;
    }
    dbglog("length=%d, expr=%p: %s", length, expr, expr);

    init_calc(expr, 12L);
    result = answer();
    dbglog("%s=%s", expr, result);
    dbglog("expr=%p, result=%p", expr, result);

    return result;
}

