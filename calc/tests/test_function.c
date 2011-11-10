/**
 * @file  calc/tests/test_function.c
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

#include <ctype.h>  /* isalpha */
#include <math.h>   /* abs sqrt sin cos tan etc...*/
#include <cutter.h> /* cutter library */

#include "def.h"
#include "util.h"
#include "log.h"
#include "calc.h"
#include "function.h"
#include "test_common.h"


/* プロトタイプ */
/** exec_func() 関数テスト */
void test_exec_func(void);
/** get_pi() 関数テスト */
void test_get_pi(void);
/** get_e() 関数テスト */
void test_get_e(void);
/** get_rad() 関数テスト */
void test_get_rad(void);
/** get_deg() 関数テスト */
void test_get_deg(void);
/** get_pow() 関数テスト */
void test_get_pow(void);
/** get_sqrt() 関数テスト */
void test_get_sqrt(void);
/** check_math() 関数テスト */
void test_check_math(void);
/** factorial() 関数テスト */
void test_factorial(void);
/** get_factorial() 関数テスト */
void test_get_factorial(void);
/** get_permutation() 関数テスト */
void test_get_permutation(void);
/** get_combination() 関数テスト */
void test_get_combination(void);

/* 内部変数 */
static testcalc calc;         /**< calc関数構造体 */
static testfunction function; /**< function関数構造体 */

/* 内部関数 */

/** テストデータ構造体 */
struct test_data {
    char expr[MAX_STRING];
    dbl answer;
    dbl x;
    dbl y;
    dbl error;
};

/** テストデータ構造体(check_math) */
struct test_data_math {
    char expr[MAX_STRING];
    dbl answer;
    dbl x;
    dbl (*callback)(dbl);
    dbl error;
};

static const struct test_data func_data[] = {
    { "pi",           3.14159265359,   0, 0, 0.00000000001   },
    { "e",            2.71828182846,   0, 0, 0.00000000001   },
    { "abs(-2)",      2,               0, 0, 0.0             },
    { "sqrt(2)",      1.41421356237,   0, 0, 0.00000000001   },
    { "sin(2)" ,      0.909297426826,  0, 0, 0.000000000001  },
    { "cos(2)",      -0.416146836547,  0, 0, 0.000000000001  },
    { "tan(2)",      -2.18503986326,   0, 0, 0.00000000001   },
    { "asin(0.5)",    0.523598775598,  0, 0, 0.000000000001  },
    { "acos(0.5)",    1.0471975512,    0, 0, 0.0000000001    },
    { "atan(0.5)",    0.463647609001,  0, 0, 0.000000000001  },
    { "exp(2)" ,      7.38905609893,   0, 0, 0.00000000001   },
    { "ln(2)",        0.69314718056,   0, 0, 0.00000000001   },
    { "log(2)",       0.301029995664,  0, 0, 0.000000000001  },
    { "deg(2)",     114.591559026,     0, 0, 0.000000001     },
    { "rad(2)",       0.0349065850399, 0, 0, 0.0000000000001 },
    { "n(10)",        3628800,         0, 0, 0.0             },
    { "nPr(5,2)",     20,              0, 0, 0.0             },
    { "nCr(5,2)",     10,              0, 0, 0.0             },
    { "nofunc(5)",    0,               0, 0, 0.0             }
};

static const struct test_data pow_data[] = {
    { "2^3",   8,  2,  3, 0 },
    { "0^0",   1,  0,  0, 0 },
    { "0^2",   0,  0,  2, 0 },
    { "2^0",   1,  2,  0, 0 },
    { "-1^3", -1, -1,  3, 0 },
    { "0^-1",  0,  0, -1, 0 },
};

static const struct test_data rad_data[] = {
    { "rad(2)", 0.0349065850399, 2, 0, 0.0000000000001 }
};

static const struct test_data deg_data[] = {
    { "deg(2)", 114.591559026, 2, 0, 0.000000001 }
};

static const struct test_data sqrt_data[] = {
    { "sqrt(2)",  1.41421356237,  2, 0, 0.00000000001 },
    { "sqrt(-1)",             0, -1, 0, 0             }
};

static const struct test_data fact_data[] = {
    { "", 1, 0, 0, 0.0 },
    { "", 1, 1, 0, 0.0 },
    { "", 2, 2, 0, 0.0 },
    { "", 6, 3, 0, 0.0 }
};

static const struct test_data_math math_data[] = {
    { "abs(-2)",    2,              -2,   fabs,  0.0            },
    { "sin(2)" ,    0.909297426826,  2,   sin,   0.000000000001 },
    { "cos(2)",    -0.416146836547,  2,   cos,   0.000000000001 },
    { "tan(2)",    -2.18503986326,   2,   tan,   0.00000000001  },
    { "asin(0.5)",  0.523598775598,  0.5, asin,  0.000000000001 },
    { "acos(0.5)",  1.0471975512,    0.5, acos,  0.0000000001   },
    { "atan(0.5)",  0.463647609001,  0.5, atan,  0.000000000001 },
    { "exp(2)" ,    7.38905609893,   2,   exp,   0.00000000001  },
    { "ln(2)",      0.69314718056,   2,   log,   0.00000000001  },
    { "log(2)",     0.301029995664,  2,   log10, 0.000000000001 },
};
/**
 * 初期化処理
 *
 * @return なし
 */
void cut_startup(void)
{
    test_init_calc(&calc);
    test_init_function(&function);
}

/**
 * exec_func() 関数テスト
 *
 * @return なし
 */
void
test_exec_func(void)
{
    dbl result = 0;                 /* 結果 */
    calcinfo *tsd = NULL;           /* calcinfo構造体 */
    char func[MAX_FUNC_STRING + 1]; /* 関数文字列 */
    int pos = 0;                    /* 配列位置 */

    int i;
    for (i = 0; i < arraysize(func_data); i++) {
        tsd = set_string(func_data[i].expr);
        if (!tsd)
            cut_error("tsd=%p", tsd);

        pos = 0;
        (void)memset(func, 0, sizeof(func));
        while (isalpha(tsd->ch) && tsd->ch != '\0' &&
               pos <= MAX_FUNC_STRING) {
               func[pos++] = tsd->ch;
               calc.readch(tsd);
        }
        dbglog("func=%s", func);

        result = exec_func(tsd, func);
        dbglog(tsd->fmt, result);
        cut_assert_equal_double(func_data[i].answer,
                                func_data[i].error,
                                result,
                                cut_message("%s = %.12g",
                                            func_data[i].expr,
                                            func_data[i].answer));
        destroy_calc(tsd);
    }
}

/**
 * get_pow() 関数テスト
 *
 * @return なし
 */
void
test_get_pow(void)
{
    dbl result = 0;       /* 結果 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(pow_data); i++) {
        tsd = set_string(pow_data[i].expr);
        if (!tsd)
            cut_error("tsd=%p", tsd);

        result = get_pow(tsd, pow_data[i].x, pow_data[i].y);
        cut_assert_equal_double(pow_data[i].answer,
                                pow_data[i].error,
                                result,
                                cut_message("%s = %.12g",
                                            pow_data[i].expr,
                                            pow_data[i].answer));
        destroy_calc(tsd);
    }
}

/**
 * get_pi() 関数テスト
 *
 * @return なし
 */
void
test_get_pi(void)
{
    dbl result = 0;               /* 結果 */
    calcinfo *tsd = NULL;         /* calcinfo構造体 */
    const dbl pi = 3.14159265359; /* pi */

    tsd = set_string("pi");
    if (!tsd)
        cut_error("tsd=%p", tsd);

    result = function.get_pi(tsd);
    cut_assert_equal_double(pi,
                            0.00000000001,
                            result,
                            cut_message("%s = %.12g",
                                        "pi",
                                        pi));
    destroy_calc(tsd);
}

/**
 * get_e() 関数テスト
 *
 * @return なし
 */
void
test_get_e(void)
{
    dbl result = 0;              /* 結果 */
    calcinfo *tsd = NULL;        /* calcinfo構造体 */
    const dbl e = 2.71828182846; /* e */

    tsd = set_string("e");
    if (!tsd)
        cut_error("tsd=%p", tsd);

    result = function.get_e(tsd);
    cut_assert_equal_double(e,
                            0.00000000001,
                            result,
                            cut_message("%s = %.12g",
                                        "e",
                                        e));
    destroy_calc(tsd);

}

/**
 * get_rad() 関数テスト
 *
 * @return なし
 */
void
test_get_rad(void)
{
    dbl result = 0;       /* 結果 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(rad_data); i++) {
        tsd = set_string(rad_data[i].expr);
        if (!tsd)
            cut_error("tsd=%p", tsd);

        result = function.get_rad(tsd, rad_data[i].x);
        cut_assert_equal_double(rad_data[i].answer,
                                rad_data[i].error,
                                result,
                                cut_message("%s = %.12g",
                                            rad_data[i].expr,
                                            rad_data[i].answer));
        destroy_calc(tsd);
    }
}

/**
 * get_deg() 関数テスト
 *
 * @return なし
 */
void
test_get_deg(void)
{
    dbl result = 0;       /* 結果 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(deg_data); i++) {
        tsd = set_string(deg_data[i].expr);
        if (!tsd)
            cut_error("tsd=%p", tsd);

        result = function.get_deg(tsd, deg_data[i].x);
        cut_assert_equal_double(deg_data[i].answer,
                                deg_data[i].error,
                                result,
                                cut_message("%s = %.12g",
                                            deg_data[i].expr,
                                            deg_data[i].answer));
        destroy_calc(tsd);
    }
}

/**
 * get_sqrt() 関数テスト
 *
 * @return なし
 */
void
test_get_sqrt(void)
{
    dbl result = 0;       /* 結果 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(sqrt_data); i++) {
        tsd = set_string(sqrt_data[i].expr);
        if (!tsd)
            cut_error("tsd=%p", tsd);

        result = function.get_sqrt(tsd, sqrt_data[i].x);
        cut_assert_equal_double(sqrt_data[i].answer,
                                sqrt_data[i].error,
                                result,
                                cut_message("%s = %.12g",
                                            sqrt_data[i].expr,
                                            sqrt_data[i].answer));
        destroy_calc(tsd);
    }
}

/**
 * check_math() 関数テスト
 *
 * @return なし
 */
void
test_check_math(void)
{
    dbl result = 0;       /* 結果 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(math_data); i++) {
        tsd = set_string(math_data[i].expr);
        if (!tsd)
            cut_error("tsd=%p", tsd);

        result = function.check_math(tsd, math_data[i].x,
                                     math_data[i].callback);
        cut_assert_equal_double(math_data[i].answer,
                                math_data[i].error,
                                result,
                                cut_message("%s = %.12g",
                                            math_data[i].expr,
                                            math_data[i].answer));
        destroy_calc(tsd);
    }
}

/**
 * factorial() 関数テスト
 *
 * @return なし
 */
void
test_factorial(void)
{
    dbl result = 0; /* 結果 */

    int i;
    for (i = 0; i < arraysize(fact_data); i++) {
        result = 1;
        function.factorial(&result, fact_data[i].x);
        cut_assert_equal_double(fact_data[i].answer,
                                fact_data[i].error,
                                result);
    }
}

/**
 * get_factorial() 関数テスト
 *
 * @return なし
 */
void
test_get_factorial(void)
{

}

/**
 * get_permutation() 関数テスト
 *
 * @return なし
 */
void
test_get_permutation(void)
{

}

/**
 * get_combination() 関数テスト
 *
 * @return なし
 */
void
test_get_combination(void)
{

}

