/**
 * @file  tests/calcp/test_error.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-11-14 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011-2018 Tetsuya Higashi. All Rights Reserved.
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
#include <fenv.h>   /* feclearexcept */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "calc.h"
#include "error.h"
#include "helper.h"

DEFINE_FFF_GLOBALS;

/* strdup() は, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(char *, strdup, const char *);
TEST_PASSTHROUGH(char *, strdup, (const char *str), (str))

/* feclearexcept() は, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(int, feclearexcept, int);
TEST_PASSTHROUGH(int, feclearexcept, (int excepts), (excepts))

/* プロトタイプ */
/* get_errormsg() 関数テスト */
TEST test_get_errormsg(void);
/* set_errormsg() 関数テスト */
TEST test_set_errorcode(void);
/* clear_error() 関数テスト */
TEST test_clear_error(void);
/* is_error() 関数テスト */
TEST test_is_error(void);
/* check_validate() 関数テスト */
TEST test_check_validate(void);
/* clear_math_feexcept() 関数テスト */
TEST test_clear_math_feexcept(void);
/* check_math_feexcept() 関数テスト */
TEST test_check_math_feexcept(void);

/* 内部変数 */
static testcalc st_calc;   /**< calc関数構造体 */
static testerror st_error; /**< error関数構造体 */

/**
 * 初期化処理
 *
 * @return なし
 */
static void
startup(void)
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
TEST
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
        TEST_ASSERT_STR_MSG(st_error.errormsg[i], (char *)calc.answer, "%s==%s",
                                            (char *)calc.answer,
                                            (char *)st_error.errormsg);
        clear_error(&calc);
        destroy_answer(&calc);
    }
    PASS();
}

/**
 * set_errorcode() 関数テスト
 *
 * @return なし
 */
TEST
test_set_errorcode(void)
{
    calcinfo calc; /* calcinfo構造体 */

    int i;
    for (i = 0; i < MAXERROR; i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, "dammy");
        st_calc.readch(&calc);

        set_errorcode(&calc, (ER)i);
        TEST_ASSERT_INT_MSG(i, (int)calc.errorcode, "%d==%d",
                                         (int)calc.errorcode,
                                         i);
        clear_error(&calc);
    }
    PASS();
}

/**
 * clear_error() 関数テスト
 *
 * @return なし
 */
TEST
test_clear_error(void)
{
    calcinfo calc; /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    calc.answer = (unsigned char *)strdup("dammy");
    clear_error(&calc);

    TEST_ASSERT_INT((int)E_NONE, (int)calc.errorcode);
    destroy_answer(&calc);
    PASS();
}

/**
 * is_error() 関数テスト
 *
 * @return なし
 */
TEST
test_is_error(void)
{
    calcinfo calc; /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    /* エラー時, trueを返す */
    set_errorcode(&calc, E_SYNTAX);
    ASSERT(is_error(&calc));

    /* 正常時, falseを返す */
    clear_error(&calc);
    ASSERT_FALSE(is_error(&calc));
    PASS();
}

/**
 * check_validate() 関数テスト
 *
 * @return なし
 */
TEST
test_check_validate(void)
{
    double result = 0.0; /* 結果 */
    calcinfo calc;       /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    result = sqrt(-1);
    dbglog("result=%g", result);
    check_validate(&calc, result);
    TEST_ASSERT_INT_MSG((int)E_NAN, (int)calc.errorcode, "E_NAN");
    clear_error(&calc);

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    result = pow(10, 10000);
    dbglog("result=%g", result);
    check_validate(&calc, result);
    TEST_ASSERT_INT_MSG((int)E_INFINITY, (int)calc.errorcode, "E_INFINITY");
    clear_error(&calc);
    PASS();
}

/**
 * get_check_math_feexcept() 関数テスト
 *
 * @return なし
 */
TEST
test_check_math_feexcept(void)
{
    double result = 0.0; /* 結果 */
    calcinfo calc;       /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    clear_math_feexcept();
    result = log(-1);
    dbglog("result=%g", result);
    check_math_feexcept(&calc);
    TEST_ASSERT_INT_MSG((int)E_NAN, (int)calc.errorcode, "NaN: log(-1)=%g", result);
    clear_error(&calc);

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    clear_math_feexcept();
    result = log(0);
    dbglog("result=%g", result);
    check_math_feexcept(&calc);
    TEST_ASSERT_INT_MSG((int)E_INFINITY, (int)calc.errorcode, "Infinity: log(0)=%g", result);
    clear_error(&calc);

    /* アンダーフローは, エラーではない */
    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    clear_math_feexcept();
    result = exp(-1000);
    dbglog("result=%g", result);
    check_math_feexcept(&calc);
    TEST_ASSERT_INT_MSG((int)E_NONE, (int)calc.errorcode, "None: exp(-1000)=%g", result);
    clear_error(&calc);
    PASS();
}

/**
 * get_clear_math_feexcept() 関数テスト
 *
 * @return なし
 */
TEST
test_clear_math_feexcept(void)
{
    double result = 0.0; /* 結果 */
    calcinfo calc;       /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    result = log(-1);
    dbglog("result=%g", result);
    clear_math_feexcept();
    check_math_feexcept(&calc);
    TEST_ASSERT_INT_MSG((int)E_NONE, (int)calc.errorcode, "None: log(-1)=%g clear", result);
    clear_error(&calc);
    PASS();
}


/**
 * 初期化処理
 *
 * @return なし
 */
static void
setup(void *data)
{
    (void)data;
    TEST_PASSTHROUGH_RESET(strdup);
    TEST_PASSTHROUGH_RESET(feclearexcept);
}

/**
 * 終了処理
 *
 * @return なし
 */
static void
teardown(void *data)
{
    (void)data;
    free_strings();
}

/**
 * get_errormsg() 関数テスト (失敗)
 *
 * @return なし
 */
TEST
test_get_errormsg_failure(void)
{
    calcinfo calc; /* calcinfo構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "dammy");
    st_calc.readch(&calc);

    /* エラーコードが範囲外 */
    calc.errorcode = E_NONE;
    TEST_ASSERT_NULL(get_errormsg(&calc));
    calc.errorcode = (ER)MAXERROR;
    TEST_ASSERT_NULL(get_errormsg(&calc));

    /* strdup() に失敗 */
    calc.errorcode = E_SYNTAX;
    TEST_INJECT(strdup, 0, 1, NULL, ENOMEM);
    TEST_ASSERT_NULL(get_errormsg(&calc));
    TEST_ASSERT_INJECTED(strdup);
    PASS();
}

/**
 * clear_math_feexcept() 関数テスト (失敗)
 *
 * @return なし
 */
TEST
test_clear_math_feexcept_failure(void)
{
    /* 失敗しても, ログを出力するだけ */
    TEST_INJECT(feclearexcept, 0, 1, -1, EINVAL);
    clear_math_feexcept();
    TEST_ASSERT_INJECTED(feclearexcept);
    PASS();
}

GREATEST_MAIN_DEFS();

int
main(int argc, char **argv)
{
    TEST_MAIN_BEGIN();
    startup();
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(test_get_errormsg);
    RUN_TEST(test_get_errormsg_failure);
    RUN_TEST(test_set_errorcode);
    RUN_TEST(test_clear_error);
    RUN_TEST(test_is_error);
    RUN_TEST(test_check_validate);
    RUN_TEST(test_check_math_feexcept);
    RUN_TEST(test_clear_math_feexcept);
    RUN_TEST(test_clear_math_feexcept_failure);
    TEST_MAIN_END();
}
