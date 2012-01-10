/**
 * @file  calc/tests/test_func.c
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
#include <math.h>   /* abs sqrt sin cos tan etc... */
#include <fcntl.h>  /* open */
#include <unistd.h> /* dup2 close */
#include <errno.h>  /* errno */
#include <cutter.h> /* cutter library */

#include "def.h"
#include "log.h"
#include "error.h"
#include "calc.h"
#include "func.h"
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
static testcalc calc; /**< calc関数構造体 */
static testfunc func; /**< func関数構造体 */

/* 内部関数 */

/** テストデータ構造体 */
struct test_data {
    char expr[MAX_STRING];
    dbl answer;
    dbl x;
    dbl y;
    ER errorcode;
    dbl error;
};

/** テストデータ構造体(check_math) */
struct test_data_math {
    char expr[MAX_STRING];
    dbl answer;
    dbl x;
    dbl (*callback)(dbl);
    ER errorcode;
    dbl error;
};

/** exec_func() 関数テスト用データ */
static const struct test_data func_data[] = {
    { "pi",          3.14159265359,   0, 0, E_NONE,   0.00000000001   },
    { "e",           2.71828182846,   0, 0, E_NONE,   0.00000000001   },
    { "abs(-2)",     2,               0, 0, E_NONE,   0.0             },
    { "sqrt(2)",     1.41421356237,   0, 0, E_NONE,   0.00000000001   },
    { "sin(2)" ,     0.909297426826,  0, 0, E_NONE,   0.000000000001  },
    { "cos(2)",     -0.416146836547,  0, 0, E_NONE,   0.000000000001  },
    { "tan(2)",     -2.18503986326,   0, 0, E_NONE,   0.00000000001   },
    { "asin(0.5)",   0.523598775598,  0, 0, E_NONE,   0.000000000001  },
    { "acos(0.5)",   1.0471975512,    0, 0, E_NONE,   0.0000000001    },
    { "atan(0.5)",   0.463647609001,  0, 0, E_NONE,   0.000000000001  },
    { "exp(2)" ,     7.38905609893,   0, 0, E_NONE,   0.00000000001   },
    { "ln(2)",       0.69314718056,   0, 0, E_NONE,   0.00000000001   },
    { "log(2)",      0.301029995664,  0, 0, E_NONE,   0.000000000001  },
    { "deg(2)",    114.591559026,     0, 0, E_NONE,   0.000000001     },
    { "rad(2)",      0.0349065850399, 0, 0, E_NONE,   0.0000000000001 },
    { "n(10)",       3628800,         0, 0, E_NONE,   0.0             },
    { "nPr(5,2)",    20,              0, 0, E_NONE,   0.0             },
    { "nCr(5,2)",    10,              0, 0, E_NONE,   0.0             },
    { "nofunc(5)",    0.0,            0, 0, E_NOFUNC, 0.0             }
};

/** get_pow() 関数テスト用データ */
static const struct test_data pow_data[] = {
    { "2^3",   8,        2,  3, E_NONE, 0 },
    { "0^0",   1,        0,  0, E_NONE, 0 },
    { "0^2",   0,        0,  2, E_NONE, 0 },
    { "2^0",   1,        2,  0, E_NONE, 0 },
    { "-1^3", -1,       -1,  3, E_NONE, 0 },
    { "0^-1",  0.0,      0, -1, E_NAN,  0 },
};

/** get_rad() 関数テスト用データ */
static const struct test_data rad_data[] = {
    { "rad(2)", 0.0349065850399, 2, 0, E_NONE, 0.0000000000001 }
};

/** get_deg() 関数テスト用データ */
static const struct test_data deg_data[] = {
    { "deg(2)", 114.591559026, 2, 0, E_NONE, 0.000000001 }
};

/** get_sqrt() 関数テスト用データ */
static const struct test_data sqrt_data[] = {
    { "sqrt(2)",  1.41421356237,  2, 0, E_NONE, 0.00000000001 },
    { "sqrt(-1)", 0.0,           -1, 0, E_NAN,  0             }
};

/** check_math() 関数テスト用データ */
static const struct test_data_math math_data[] = {
    { "abs(-2)",    2,              -2,   fabs,  E_NONE, 0.0            },
    { "sin(2)" ,    0.909297426826,  2,   sin,   E_NONE, 0.000000000001 },
    { "cos(2)",    -0.416146836547,  2,   cos,   E_NONE, 0.000000000001 },
    { "tan(2)",    -2.18503986326,   2,   tan,   E_NONE, 0.00000000001  },
    { "asin(0.5)",  0.523598775598,  0.5, asin,  E_NONE, 0.000000000001 },
    { "acos(0.5)",  1.0471975512,    0.5, acos,  E_NONE, 0.0000000001   },
    { "atan(0.5)",  0.463647609001,  0.5, atan,  E_NONE, 0.000000000001 },
    { "exp(2)" ,    7.38905609893,   2,   exp,   E_NONE, 0.00000000001  },
    { "ln(2)",      0.69314718056,   2,   log,   E_NONE, 0.00000000001  },
    { "log(2)",     0.301029995664,  2,   log10, E_NONE, 0.000000000001 },
};

/** factorial() 関数テスト用データ */
static const struct test_data fact_data[] = {
    { "",       1,  1, 0, E_NONE, 0.0 },
    { "",       1,  1, 0, E_NONE, 0.0 },
    { "",       2,  2, 0, E_NONE, 0.0 },
    { "",       6,  3, 0, E_NONE, 0.0 },
    { "",      24,  4, 0, E_NONE, 0.0 },
    { "",     120,  5, 0, E_NONE, 0.0 },
    { "",     720,  6, 0, E_NONE, 0.0 },
    { "",    5040,  7, 0, E_NONE, 0.0 },
    { "",   40320,  8, 0, E_NONE, 0.0 },
    { "",  362880,  9, 0, E_NONE, 0.0 },
    { "", 3628800, 10, 0, E_NONE, 0.0 }
};

/** get_factorial() 関数テスト用データ */
static const struct test_data factorial_data[] = {
    { "n(0)",         1,    0,   0, E_NONE, 0.0 },
    { "n(1)",         1,    1,   0, E_NONE, 0.0 },
    { "n(2)",         2,    2,   0, E_NONE, 0.0 },
    { "n(3)",         6,    3,   0, E_NONE, 0.0 },
    { "n(9)",    362880,    9,   0, E_NONE, 0.0 },
    { "n(-3)",       -6,   -3,   0, E_NONE, 0.0 },
    { "n(-9)",  -362880,   -9,   0, E_NONE, 0.0 },
    { "n(0.5)",       0.0,  0.5, 0, E_NAN,  0.0 }
};

/** get_permutation() 関数テスト用データ */
static const struct test_data permutation_data[] = {
    { "nPr(5,2)",  20,    5,  2, E_NONE, 0.0 },
    { "nPr(-5,2)",  0.0, -5,  2, E_NAN,  0.0 },
    { "nPr(5,-2)",  0.0,  5, -2, E_NAN,  0.0 },
    { "nPr(2,5)",   0.0,  2,  5, E_NAN,  0.0 }
};

/** get_combination() 関数テスト用データ */
static const struct test_data combination_data[] = {
    { "nCr(5,2)",  10,    5,  2, E_NONE, 0.0 },
    { "nCr(-5,2)",  0.0, -5,  2, E_NAN,  0.0 },
    { "nCr(5,-2)",  0.0,  5, -2, E_NAN,  0.0 },
    { "nCr(2,5)",   0.0,  2,  5, E_NAN,  0.0 }
};

/**
 * 初期化処理
 *
 * @return なし
 */
void cut_startup(void)
{
    (void)memset(&calc, 0, sizeof(testcalc));
    (void)memset(&func, 0, sizeof(testfunc));
    test_init_calc(&calc);
    test_init_func(&func);
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
    for (i = 0; i < NELEMS(func_data); i++) {
        tsd = set_string(func_data[i].expr);
        if (!tsd)
            cut_error("i=%d, tsd=%p, expr=%s(%d)",
                      i, tsd, func_data[i].expr, errno);

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
                                cut_message("%s=%.12g",
                                            func_data[i].expr,
                                            func_data[i].answer));
        cut_assert_equal_int((int)func_data[i].errorcode,
                             (int)tsd->errorcode,
                             cut_message("%s error",
                                         func_data[i].expr));
        clear_error(tsd);
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
    for (i = 0; i < NELEMS(pow_data); i++) {
        tsd = set_string(pow_data[i].expr);
        if (!tsd)
            cut_error("i=%d, tsd=%p, expr=%s(%d)",
                      i, tsd, pow_data[i].expr, errno);

        result = get_pow(tsd, pow_data[i].x, pow_data[i].y);
        cut_assert_equal_double(pow_data[i].answer,
                                pow_data[i].error,
                                result,
                                cut_message("%s=%.12g",
                                            pow_data[i].expr,
                                            pow_data[i].answer));
        cut_assert_equal_int((int)pow_data[i].errorcode,
                             (int)tsd->errorcode,
                             cut_message("%s error",
                                         pow_data[i].expr));
        clear_error(tsd);
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
        cut_error("tsd=%p, expr=%s(%d)", tsd, "pi", errno);

    result = func.get_pi(tsd);
    cut_assert_equal_double(pi,
                            0.00000000001,
                            result,
                            cut_message("%s=%.12g",
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
        cut_error("tsd=%p, expr=%s(%d)", tsd, "e", errno);

    result = func.get_e(tsd);
    cut_assert_equal_double(e,
                            0.00000000001,
                            result,
                            cut_message("%s=%.12g",
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
    for (i = 0; i < NELEMS(rad_data); i++) {
        tsd = set_string(rad_data[i].expr);
        if (!tsd)
            cut_error("i=%d, tsd=%p, expr=%s(%d)",
                      i, tsd, rad_data[i].expr, errno);

        result = func.get_rad(tsd, rad_data[i].x);
        cut_assert_equal_double(rad_data[i].answer,
                                rad_data[i].error,
                                result,
                                cut_message("%s=%.12g",
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
    for (i = 0; i < NELEMS(deg_data); i++) {
        tsd = set_string(deg_data[i].expr);
        if (!tsd)
            cut_error("i=%d, tsd=%p, expr=%s(%d)",
                      i, tsd, deg_data[i].expr, errno);

        result = func.get_deg(tsd, deg_data[i].x);
        cut_assert_equal_double(deg_data[i].answer,
                                deg_data[i].error,
                                result,
                                cut_message("%s=%.12g",
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
    for (i = 0; i < NELEMS(sqrt_data); i++) {
        tsd = set_string(sqrt_data[i].expr);
        if (!tsd)
            cut_error("i=%d, tsd=%p, expr=%s(%d)",
                      i, tsd, sqrt_data[i].expr, errno);

        result = func.get_sqrt(tsd, sqrt_data[i].x);
        cut_assert_equal_double(sqrt_data[i].answer,
                                sqrt_data[i].error,
                                result,
                                cut_message("%s=%.12g",
                                            sqrt_data[i].expr,
                                            sqrt_data[i].answer));
        cut_assert_equal_int((int)sqrt_data[i].errorcode,
                             (int)tsd->errorcode,
                             cut_message("%s error",
                                         sqrt_data[i].expr));
        clear_error(tsd);
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
    for (i = 0; i < NELEMS(math_data); i++) {
        tsd = set_string(math_data[i].expr);
        if (!tsd)
            cut_error("i=%d, tsd=%p, expr=%s(%d)",
                      i, tsd, math_data[i].expr, errno);

        result = func.check_math(tsd, math_data[i].x,
                                 math_data[i].callback);
        cut_assert_equal_double(math_data[i].answer,
                                math_data[i].error,
                                result,
                                cut_message("%s=%.12g",
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
    for (i = 0; i < NELEMS(fact_data); i++) {
        result = 1;
        func.factorial(&result, fact_data[i].x);
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
    dbl result = 0;       /* 結果 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < NELEMS(factorial_data); i++) {
        tsd = set_string(factorial_data[i].expr);
        if (!tsd)
            cut_error("i=%d, tsd=%p, expr=%s(%d)",
                      i, tsd, factorial_data[i].expr, errno);

        result = func.get_factorial(tsd, factorial_data[i].x);
        cut_assert_equal_double(factorial_data[i].answer,
                                factorial_data[i].error,
                                result,
                                cut_message("%s=%.12g",
                                            factorial_data[i].expr,
                                            factorial_data[i].answer));
        cut_assert_equal_int((int)factorial_data[i].errorcode,
                             (int)tsd->errorcode,
                             cut_message("%s error",
                                         factorial_data[i].expr));
        clear_error(tsd);
        destroy_calc(tsd);
    }
}

/**
 * get_permutation() 関数テスト
 *
 * @return なし
 */
void
test_get_permutation(void)
{
    dbl result = 0;       /* 結果 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < NELEMS(permutation_data); i++) {
        tsd = set_string(permutation_data[i].expr);
        if (!tsd)
            cut_error("i=%d, tsd=%p, expr=%s(%d)",
                      i, tsd, permutation_data[i].expr, errno);

        result = func.get_permutation(tsd,
                                      permutation_data[i].x,
                                      permutation_data[i].y);
        cut_assert_equal_double(permutation_data[i].answer,
                                permutation_data[i].error,
                                result,
                                cut_message("%s=%.12g",
                                            permutation_data[i].expr,
                                            permutation_data[i].answer));
        cut_assert_equal_int((int)permutation_data[i].errorcode,
                             (int)tsd->errorcode,
                             cut_message("%s error",
                                         permutation_data[i].expr));
        clear_error(tsd);
        destroy_calc(tsd);
    }
}

/**
 * get_combination() 関数テスト
 *
 * @return なし
 */
void
test_get_combination(void)
{
    dbl result = 0;       /* 結果 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < NELEMS(combination_data); i++) {
        tsd = set_string(combination_data[i].expr);
        if (!tsd)
            cut_error("i=%d, tsd=%p, expr=%s(%d)",
                      i, tsd, combination_data[i].expr, errno);

        result = func.get_combination(tsd,
                                      combination_data[i].x,
                                      combination_data[i].y);
        cut_assert_equal_double(combination_data[i].answer,
                                combination_data[i].error,
                                result,
                                cut_message("%s=%.12g",
                                            combination_data[i].expr,
                                            combination_data[i].answer));
        cut_assert_equal_int((int)combination_data[i].errorcode,
                             (int)tsd->errorcode,
                             cut_message("%s error",
                                         combination_data[i].expr));
        clear_error(tsd);
        destroy_calc(tsd);
    }
}

