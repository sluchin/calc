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

void test_answer_four(void);
void test_answer_func(void);
void test_answer_four_func(void);
void test_answer_error(void);
void test_get_strlen(void);

/* 内部変数 */
static struct test_calc_func func; /**< 関数構造体 */

/* 内部関数 */
/** バッファセット */
static char *exec_calc(char *str);

void
cut_setup(void)
{
    test_init_calc(&func);
}

void
cut_teardown(void)
{

}

/**
 * 四則演算テスト
 */
void test_answer_four(void)
{
    cut_assert_equal_string("421", exec_calc("(105+312)+2*(5-3)"));
    destroy_calc();
    cut_assert_equal_string("418", exec_calc("(105+312)+2/(5-3)"));
    destroy_calc();
    cut_assert_equal_string("5", exec_calc("1+2*(5-3)"));
    destroy_calc();
    cut_assert_equal_string("2", exec_calc("1+2/(5-3)"));
    destroy_calc();
}

/**
 * 関数テスト
 */
void test_answer_func(void)
{
    cut_assert_equal_string("3.14159265359", exec_calc("pi"));
    destroy_calc();
    cut_assert_equal_string("2.71828182846", exec_calc("e"));
    destroy_calc();
    cut_assert_equal_string("2", exec_calc("abs(-2)"));
    destroy_calc();
    cut_assert_equal_string("1.41421356237", exec_calc("sqrt(2)"));
    destroy_calc();
    cut_assert_equal_string("0.909297426826", exec_calc("sin(2)"));
    destroy_calc();
    cut_assert_equal_string("-0.416146836547", exec_calc("cos(2)"));
    destroy_calc();
    cut_assert_equal_string("-2.18503986326", exec_calc("tan(2)"));
    destroy_calc();
    cut_assert_equal_string("0.523598775598", exec_calc("asin(0.5)"));
    destroy_calc();
    cut_assert_equal_string("1.0471975512", exec_calc("acos(0.5)"));
    destroy_calc();
    cut_assert_equal_string("0.463647609001", exec_calc("atan(0.5)"));
    destroy_calc();
    cut_assert_equal_string("7.38905609893", exec_calc("exp(2)"));
    destroy_calc();
    cut_assert_equal_string("0.69314718056", exec_calc("ln(2)"));
    destroy_calc();
    cut_assert_equal_string("0.301029995664", exec_calc("log(2)"));
    destroy_calc();
    cut_assert_equal_string("114.591559026", exec_calc("deg(2)"));
    destroy_calc();
    cut_assert_equal_string("0.0349065850399", exec_calc("rad(2)"));
    destroy_calc();
    cut_assert_equal_string("3628800", exec_calc("n(10)"));
    destroy_calc();
    cut_assert_equal_string("20", exec_calc("nPr(5,2)"));
    destroy_calc();
    cut_assert_equal_string("10", exec_calc("nCr(5,2)"));
    destroy_calc();
}

/**
 * 四則演算と関数の組み合わせテスト
 */
void test_answer_four_func(void)
{
    cut_assert_equal_string("15.7079632679", exec_calc("5*pi"));
    destroy_calc();
    cut_assert_equal_string("15.7079632679", exec_calc("pi*5"));
    destroy_calc();
    cut_assert_equal_string("13.5914091423", exec_calc("5*e"));
    destroy_calc();
    cut_assert_equal_string("13.5914091423", exec_calc("e*5"));
    destroy_calc();
    cut_assert_equal_string("10", exec_calc("5*abs(-2)"));
    destroy_calc();
    cut_assert_equal_string("10", exec_calc("abs(-2)*5"));
    destroy_calc();
    cut_assert_equal_string("7.07106781187", exec_calc("5*sqrt(2)"));
    destroy_calc();
    cut_assert_equal_string("7.07106781187", exec_calc("sqrt(2)*5"));
    destroy_calc();
    cut_assert_equal_string("4.54648713413", exec_calc("5*sin(2)"));
    destroy_calc();
    cut_assert_equal_string("4.54648713413", exec_calc("sin(2)*5"));
    destroy_calc();
    cut_assert_equal_string("-2.08073418274", exec_calc("5*cos(2)"));
    destroy_calc();
    cut_assert_equal_string("-2.08073418274", exec_calc("cos(2)*5"));
    destroy_calc();
    cut_assert_equal_string("-10.9251993163", exec_calc("5*tan(2)"));
    destroy_calc();
    cut_assert_equal_string("-10.9251993163", exec_calc("tan(2)*5"));
    destroy_calc();
    cut_assert_equal_string("1.0471975512", exec_calc("2*asin(0.5)"));
    destroy_calc();
    cut_assert_equal_string("1.0471975512", exec_calc("asin(0.5)*2"));
    destroy_calc();
    cut_assert_equal_string("2.09439510239", exec_calc("2*acos(0.5)"));
    destroy_calc();
    cut_assert_equal_string("2.09439510239", exec_calc("acos(0.5)*2"));
    destroy_calc();
    cut_assert_equal_string("2.318238045", exec_calc("5*atan(0.5)"));
    destroy_calc();
    cut_assert_equal_string("2.318238045", exec_calc("atan(0.5)*5"));
    destroy_calc();
    cut_assert_equal_string("36.9452804947", exec_calc("5*exp(2)"));
    destroy_calc();
    cut_assert_equal_string("36.9452804947", exec_calc("exp(2)*5"));
    destroy_calc();
    cut_assert_equal_string("3.4657359028", exec_calc("5*ln(2)"));
    destroy_calc();
    cut_assert_equal_string("3.4657359028", exec_calc("ln(2)*5"));
    destroy_calc();
    cut_assert_equal_string("1.50514997832", exec_calc("5*log(2)"));
    destroy_calc();
    cut_assert_equal_string("1.50514997832", exec_calc("log(2)*5"));
    destroy_calc();
    cut_assert_equal_string("572.957795131", exec_calc("5*deg(2)"));
    destroy_calc();
    cut_assert_equal_string("572.957795131", exec_calc("deg(2)*5"));
    destroy_calc();
    cut_assert_equal_string("0.174532925199", exec_calc("5*rad(2)"));
    destroy_calc();
    cut_assert_equal_string("0.174532925199", exec_calc("rad(2)*5"));
    destroy_calc();
    cut_assert_equal_string("18144000", exec_calc("5*n(10)"));
    destroy_calc();
    cut_assert_equal_string("18144000", exec_calc("n(10)*5"));
    destroy_calc();
    cut_assert_equal_string("100", exec_calc("5*nPr(5,2)"));
    destroy_calc();
    cut_assert_equal_string("100", exec_calc("nPr(5,2)*5"));
    destroy_calc();
    cut_assert_equal_string("50", exec_calc("5*nCr(5,2)"));
    destroy_calc();
    cut_assert_equal_string("4.43749076323e+14", exec_calc("nCr(50,22)*5"));
    destroy_calc();
}

/**
 * 関数エラー時
 */
void
test_answer_error(void)
{
    cut_assert_equal_string("Divide by zero.", exec_calc("5/0"));
    cut_assert_equal_string("Syntax error.", exec_calc("sin(5"));
    destroy_calc();
    cut_assert_equal_string("Syntax error.", exec_calc("nCr(5)"));
    destroy_calc();
    cut_assert_equal_string("Function not defined.", exec_calc("nofunc(5)"));
    destroy_calc();
    cut_assert_equal_string("Nan.", exec_calc("n(0.5)"));
    destroy_calc();
    cut_assert_equal_string("Nan.", exec_calc("nPr(-1,2)"));
    destroy_calc();
    cut_assert_equal_string("Nan.", exec_calc("nPr(3,5)"));
    destroy_calc();
    cut_assert_equal_string("Nan.", exec_calc("nCr(-1,2)"));
    destroy_calc();
    cut_assert_equal_string("Nan.", exec_calc("nCr(3,5)"));
    destroy_calc();
    cut_assert_equal_string("Nan.", exec_calc("sqrt(-5)"));
    destroy_calc();
    cut_assert_equal_string("Infinity.", exec_calc("10^1000000"));
    destroy_calc();
    cut_assert_equal_string("Infinity.", exec_calc("n(5000)"));
    destroy_calc();
    cut_assert_equal_string("Infinity.", exec_calc("n(-5000)"));
    destroy_calc();

}

/**
 * get_strlen() 関数テスト
 */
void test_get_strlen(void)
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
 * 計算実行
 *
 * @param[in] str 文字列
 * @return なし
 */
static char *
exec_calc(char *str)
{
    size_t length = 0;    /* 文字列長 */
    uchar *expr = NULL;   /**< 式 */
    uchar *result = NULL; /**< 結果 */

    length = strlen(str);
    expr = (uchar *)cut_take_strndup(str, length);
    if (!expr) {
        outlog("cut_take_strndup=%p", expr);
        return NULL;
    }

    init_calc(expr, 12L);
    result = answer();
    dbglog("%s=%s", expr, result);
    dbglog("length=%u, expr=%p, result=%p", length,  expr, result);

    return (char *)result;
}

