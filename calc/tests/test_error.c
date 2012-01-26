/**
 * @file  calc/tests/test_error.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-11-14 higashi 新規作成
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

#include <math.h>   /* sqrt log */
#include <fcntl.h>  /* open */
#include <unistd.h> /* dup2 close */
#include <errno.h>  /* errno */
#include <cutter.h> /* cutter library */

#include "def.h"
#include "log.h"
#include "calc.h"
#include "error.h"
#include "test_common.h"

/* プロトタイプ */
/* get_errormsg() 関数テスト */
void test_get_errormsg(void);
/* set_errormsg() 関数テスト */
void test_set_errorcode(void);
/* clear_error() 関数テスト */
void test_clear_error(void);
/* is_error() 関数テスト */
void test_is_error(void);
/* check_validate() 関数テスト */
void test_check_validate(void);
/* clear_math_feexcept() 関数テスト */
void test_clear_math_feexcept(void);
/* check_math_feexcept() 関数テスト */
void test_check_math_feexcept(void);

/* 内部変数 */
static testcalc st_calc;   /**< calc関数構造体 */
static testerror st_error; /**< error関数構造体 */

/**
 * 初期化処理
 *
 * @return なし
 */
void cut_startup(void)
{
    (void)memset(&st_calc, 0, sizeof(testcalc));
    (void)memset(&st_error, 0, sizeof(testerror));
    test_init_calc(&st_calc);
    test_init_error(&st_error);
}

/**
 * get_errormsg() 関数テスト
 *
 * @return なし
 */
void
test_get_errormsg(void)
{
    calcinfo calc; /* calcinfo構造体 */

    int i;
    for (i = 0; i < MAXERROR; i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, "dammy");
        st_calc.readch(&calc);
        calc.errorcode = (ER)i;
        calc.answer = get_errormsg(&calc);
        cut_assert_equal_string(st_error.errormsg[i],
                                (char *)calc.answer,
                                cut_message("%s==%s",
                                            (char *)calc.answer,
                                            (char *)st_error.errormsg));
        clear_error(&calc);
        destroy_answer(&calc);
    }
}

/**
 * set_errorcode() 関数テスト
 *
 * @return なし
 */
void
test_set_errorcode(void)
{
    calcinfo calc; /* calcinfo構造体 */

    int i;
    for (i = 0; i < MAXERROR; i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, "dammy");
        st_calc.readch(&calc);

        set_errorcode(&calc, (ER)i);
        cut_assert_equal_int(i,
                             (int)calc.errorcode,
                             cut_message("%d==%d",
                                         (int)calc.errorcode,
                                         i));
        clear_error(&calc);
    }
}

/**
 * clear_error() 関数テスト
 *
 * @return なし
 */
void
test_clear_error(void)
{
    calcinfo calc; /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    calc.answer = (uchar *)strdup("dammy");
    clear_error(&calc);

    cut_assert_equal_int((int)E_NONE, (int)calc.errorcode);
    destroy_answer(&calc);
}

/**
 * is_error() 関数テスト
 *
 * @return なし
 */
void
test_is_error(void)
{
    calcinfo calc; /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    /* エラー時, trueを返す */
    set_errorcode(&calc, E_SYNTAX);
    cut_assert_true((cut_boolean)is_error(&calc));

    /* 正常時, falseを返す */
    clear_error(&calc);
    cut_assert_false((cut_boolean)is_error(&calc));
}

/**
 * check_validate() 関数テスト
 *
 * @return なし
 */
void
test_check_validate(void)
{
    dbl result = 0.0; /* 結果 */
    calcinfo calc;    /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    result = sqrt(-1);
    dbglog("result=%g", result);
    check_validate(&calc, result);
    cut_assert_equal_int((int)E_NAN,
                         (int)calc.errorcode,
                         cut_message("E_NAN"));
    clear_error(&calc);

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    result = pow(10, 10000);
    dbglog("result=%g", result);
    check_validate(&calc, result);
    cut_assert_equal_int((int)E_INFINITY,
                         (int)calc.errorcode,
                         cut_message("E_INFINITY"));
    clear_error(&calc);
}

/**
 * get_check_math_feexcept() 関数テスト
 *
 * @return なし
 */
void
test_check_math_feexcept(void)
{
    dbl result = 0.0; /* 結果 */
    calcinfo calc;    /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    clear_math_feexcept();
    result = log(-1);
    dbglog("result=%g", result);
    check_math_feexcept(&calc);
    cut_assert_equal_int((int)E_NAN,
                         (int)calc.errorcode,
                         cut_message("NaN: log(-1)=%g", result));
    clear_error(&calc);

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    clear_math_feexcept();
    result = log(0);
    dbglog("result=%g", result);
    check_math_feexcept(&calc);
    cut_assert_equal_int((int)E_INFINITY,
                         (int)calc.errorcode,
                         cut_message("Infinity: log(0)=%g", result));
    clear_error(&calc);
}

/**
 * get_clear_math_feexcept() 関数テスト
 *
 * @return なし
 */
void
test_clear_math_feexcept(void)
{
    dbl result = 0.0; /* 結果 */
    calcinfo calc;    /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    result = log(-1);
    dbglog("result=%g", result);
    clear_math_feexcept();
    check_math_feexcept(&calc);
    cut_assert_equal_int((int)E_NONE,
                         (int)calc.errorcode,
                         cut_message("None: log(-1)=%g clear", result));
    clear_error(&calc);
}

