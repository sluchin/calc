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
static testerror error; /**< 関数構造体 */

/**
 * 初期化処理
 *
 * @return なし
 */
void cut_startup(void)
{
    (void)memset(&error, 0, sizeof(testerror));
    test_init_error(&error);
}

/**
 * get_errormsg() 関数テスト
 *
 * @return なし
 */
void
test_get_errormsg(void)
{
    calcinfo tsd; /* calcinfo構造体 */

    int i;
    for (i = 0; i < MAXERROR; i++) {
        (void)memset(&tsd, 0, sizeof(calcinfo));
        set_string(&tsd, "dammy");
        tsd.errorcode = (ER)i;
        tsd.answer = get_errormsg(&tsd);
        cut_assert_equal_string(error.errormsg[i],
                                (char *)tsd.answer,
                                cut_message("%s==%s",
                                            (char *)tsd.answer,
                                            (char *)error.errormsg));
        clear_error(&tsd);
        destroy_answer(&tsd);
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
    calcinfo tsd; /* calcinfo構造体 */

    int i;
    for (i = 0; i < MAXERROR; i++) {
        (void)memset(&tsd, 0, sizeof(calcinfo));
        set_string(&tsd, "dammy");

        set_errorcode(&tsd, (ER)i);
        cut_assert_equal_int(i,
                             (int)tsd.errorcode,
                             cut_message("%d==%d",
                                         (int)tsd.errorcode,
                                         i));
        clear_error(&tsd);
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
    calcinfo tsd; /* calcinfo構造体 */

    (void)memset(&tsd, 0, sizeof(calcinfo));
    set_string(&tsd, "dammy");

    tsd.answer = (uchar *)strdup("dammy");
    clear_error(&tsd);

    cut_assert_equal_int((int)E_NONE, (int)tsd.errorcode);
    destroy_answer(&tsd);
}

/**
 * is_error() 関数テスト
 *
 * @return なし
 */
void
test_is_error(void)
{
    calcinfo tsd; /* calcinfo構造体 */

    (void)memset(&tsd, 0, sizeof(calcinfo));
    set_string(&tsd, "dammy");

    /* エラー時, trueを返す */
    set_errorcode(&tsd, E_SYNTAX);
    cut_assert_true((cut_boolean)is_error(&tsd));

    /* 正常時, falseを返す */
    clear_error(&tsd);
    cut_assert_false((cut_boolean)is_error(&tsd));
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
    calcinfo tsd;     /* calcinfo構造体 */

    (void)memset(&tsd, 0, sizeof(calcinfo));
    set_string(&tsd, "dammy");

    result = sqrt(-1);
    dbglog("result=%g", result);
    check_validate(&tsd, result);
    cut_assert_equal_int((int)E_NAN,
                         (int)tsd.errorcode,
                         cut_message("E_NAN"));
    clear_error(&tsd);

    (void)memset(&tsd, 0, sizeof(calcinfo));
    set_string(&tsd, "dammy");

    result = pow(10, 10000);
    dbglog("result=%g", result);
    check_validate(&tsd, result);
    cut_assert_equal_int((int)E_INFINITY,
                         (int)tsd.errorcode,
                         cut_message("E_INFINITY"));
    clear_error(&tsd);
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
    calcinfo tsd;     /* calcinfo構造体 */

    (void)memset(&tsd, 0, sizeof(calcinfo));
    set_string(&tsd, "dammy");

    clear_math_feexcept();
    result = log(-1);
    dbglog("result=%g", result);
    check_math_feexcept(&tsd);
    cut_assert_equal_int((int)E_NAN,
                         (int)tsd.errorcode,
                         cut_message("NaN: log(-1)=%g", result));
    clear_error(&tsd);

    (void)memset(&tsd, 0, sizeof(calcinfo));
    set_string(&tsd, "dammy");

    clear_math_feexcept();
    result = log(0);
    dbglog("result=%g", result);
    check_math_feexcept(&tsd);
    cut_assert_equal_int((int)E_INFINITY,
                         (int)tsd.errorcode,
                         cut_message("Infinity: log(0)=%g", result));
    clear_error(&tsd);
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
    calcinfo tsd;     /* calcinfo構造体 */

    (void)memset(&tsd, 0, sizeof(calcinfo));
    set_string(&tsd, "dammy");

    result = log(-1);
    dbglog("result=%g", result);
    clear_math_feexcept();
    check_math_feexcept(&tsd);
    cut_assert_equal_int((int)E_NONE,
                         (int)tsd.errorcode,
                         cut_message("None: log(-1)=%g clear", result));
    clear_error(&tsd);
}

