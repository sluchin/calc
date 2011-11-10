/**
 * @file  calc/function.c
 * @brief 関数
 *
 * @author higashi
 * @date 2011-08-15 higashi 新規作成
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

#include <string.h>  /* strcmp */
#include <stdbool.h> /* bool */
#include <errno.h>   /* errno */
#include <math.h>    /* sinl cosl tanl logl log10l */
#include <fenv.h>    /* FE_ALL_EXCEPT */
#include <assert.h>  /* assert */

#include "log.h"
#include "error.h"
#include "function.h"

/** pi(4*atan(1)) */
#define DEF_PI       3.14159265358979323846264338327950288
/** ネイピア数(オイラー数) */
#define DEF_E        2.71828182845904523536028747135266249

/* 内部関数 */
/** 関数情報構造体初期化 */
static void init_func(void) __attribute__((constructor));
/** Pi取得 */
static dbl get_pi(calcinfo *tsd);
/** ネイピア数(オイラー数)取得 */
static dbl get_e(calcinfo *tsd);
/** 角度をラジアンに変換 */
static dbl get_rad(calcinfo *tsd, dbl x);
/** ラジアンを角度に変換 */
static dbl get_deg(calcinfo *tsd, dbl x);
/** 平方根 */
static dbl get_sqrt(calcinfo *tsd, dbl x);
/** 関数エラーチェック */
static dbl check_math(calcinfo *tsd, dbl x, dbl (*callback)(dbl));
/** 階乗 */
static void factorial(dbl *x, dbl n);
/** 階乗取得 */
static dbl get_factorial(calcinfo *tsd, dbl n);
/** 順列(nPr) */
static dbl get_permutation(calcinfo *tsd, dbl n, dbl r);
/** 組み合わせ(nCr) */
static dbl get_combination(calcinfo *tsd, dbl n, dbl r);

/** 関数種別 */
enum functype {
    FN_PI = 0, FN_E, FN_ABS, FN_SQRT, FN_SIN,
    FN_COS, FN_TAN,  FN_ASIN, FN_ACOS, FN_ATAN,
    FN_EXP, FN_LN, FN_LOG, FN_RAD, FN_DEG,
    FN_FACT, FN_PERM, FN_COMB,
    MAXFUNC
};

/** 関数文字列構造体 */
struct funcstring {
    enum functype type;
    char funcname[MAX_FUNC_STRING + 1];
};

/** 関数文字列構造体初期化 */
static struct funcstring fstring[] = {
    { FN_PI,   "pi"   }, /**< pi */
    { FN_E,    "e"    }, /**< ネイピア数(オイラー数) */
    { FN_ABS,  "abs"  }, /**< 絶対値 */
    { FN_SQRT, "sqrt" }, /**< 平方根 */
    { FN_SIN,  "sin"  }, /**< 三角関数(sin) */
    { FN_COS,  "cos"  }, /**< 三角関数(cosin) */
    { FN_TAN,  "tan"  }, /**< 三角関数(tangent) */
    { FN_ASIN, "asin" }, /**< 逆三角関数(arcsin) */
    { FN_ACOS, "acos" }, /**< 逆三角関数(arccosin) */
    { FN_ATAN, "atan" }, /**< 逆三角関数(arctangent) */
    { FN_EXP,  "exp"  }, /**< 指数関数 */
    { FN_LN,   "ln"   }, /**< 自然対数 */
    { FN_LOG,  "log"  }, /**< 常用対数 */
    { FN_RAD,  "rad"  }, /**< 角度をラジアンに変換 */
    { FN_DEG,  "deg"  }, /**< ラジアンを角度に変換 */
    { FN_FACT, "n"    }, /**< 階乗 */
    { FN_PERM, "nPr"  }, /**< 順列 */
    { FN_COMB, "nCr"  }  /**< 組み合わせ */
};

/** 関数共用体 */
union func {
    dbl (*func0)(calcinfo *tsd);
    dbl (*func1)(calcinfo *tsd, dbl x);
    dbl (*func2)(calcinfo *tsd, dbl x, dbl y);
    dbl (*math)(dbl x);
};

/** 関数種別列挙体 */
enum uniontype {
    FUNC0,
    FUNC1,
    FUNC2,
    MATH
};

/** 関数情報構造体 */
struct funcinfo {
    enum uniontype type;
    union func func;
};

/** 関数情報構造体配列 */
static struct funcinfo finfo[MAXFUNC];

/**
 * 関数実行
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] func 関数名
 * return なし
 */
dbl
exec_func(calcinfo *tsd, const char *func)
{
    dbl result = 0;      /* 戻り値 */
    dbl x = 0, y = 0;    /* 値 */
    int i;               /* 変数 */
    bool exec;           /* 関数実行フラグ */
    enum functype ftype; /* 関数種別 */

    dbglog("start: func=%s", func);

    for (i = 0, exec = false; i < MAXFUNC && !exec; i++) {
        if (!strcmp(fstring[i].funcname, func)) {
            ftype = fstring[i].type;
            dbglog("i=%d, ftype=%d", i, ftype);
            switch (finfo[ftype].type) {
                dbglog("type=%d", finfo[ftype].type);
            case FUNC0:
                result = finfo[ftype].func.func0(tsd);
                break;
            case FUNC1:
                parse_func_args(tsd, ARG_1, &x, &y);
                result = finfo[ftype].func.func1(tsd, x);
                break;
            case FUNC2:
                parse_func_args(tsd, ARG_2, &x, &y);
                result = finfo[ftype].func.func2(tsd, x, y);
                break;
            case MATH:
                parse_func_args(tsd, ARG_1, &x, &y);
                result = check_math(tsd, x, finfo[ftype].func.math);
                break;
            default:
                break;
            }
            exec = true;
        }
    }
    if (!exec) /* エラー */
        set_errorcode(tsd, E_NOFUNC);

    dbglog("x=%.12g, y=%.12g", x, y);
    dbglog(tsd->fmt, result);
    return result;
}

/**
 * 指数取得
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] x 値
 * @param[in] y 値
 * @return 指数
 */
dbl
get_pow(calcinfo *tsd, dbl x, dbl y)
{
    dbl result = 0; /* 計算結果 */

    dbglog("start");

    if (is_error(tsd))
        return EX_ERROR;

    if ((fpclassify(x) == FP_ZERO) && isless(y, 0)) {
        /* 定義域エラー */
        set_errorcode(tsd, E_NAN);
        return EX_ERROR;
    }

    clear_math_feexcept();

    result = pow(x, y);

    check_math_feexcept(tsd, result);

    return result;
}

/**
 * 関数情報構造体初期化
 *
 * @return なし
 */
static void
init_func(void)
{
    dbglog("start");

    assert(MAXFUNC == arraysize(fstring));
    assert(MAXFUNC == arraysize(finfo));

    memset(finfo, 0, sizeof(finfo));

    /* pi */
    finfo[FN_PI].type = FUNC0;
    finfo[FN_PI].func.func0 = get_pi;
    /* ネイピア数(オイラー数) */
    finfo[FN_E].type = FUNC0;
    finfo[FN_E].func.func0 = get_e;
    /* 絶対値 */
    finfo[FN_ABS].type = MATH;
    finfo[FN_ABS].func.math = fabs;
    /* 平方根 */
    finfo[FN_SQRT].type = FUNC1;
    finfo[FN_SQRT].func.func1 = get_sqrt;
    /* 三角関数(sin) */
    finfo[FN_SIN].type = MATH;
    finfo[FN_SIN].func.math = sin;
    /* 三角関数(cosin) */
    finfo[FN_COS].type = MATH;
    finfo[FN_COS].func.math = cos;
    /* 三角関数(tangent) */
    finfo[FN_TAN].type = MATH;
    finfo[FN_TAN].func.math = tan;
    /* 逆三角関数(arcsin) */
    finfo[FN_ASIN].type = MATH;
    finfo[FN_ASIN].func.math = asin;
    /* 逆三角関数(arccosin) */
    finfo[FN_ACOS].type = MATH;
    finfo[FN_ACOS].func.math = acos;
    /* 逆三角関数(arccosin) */
    finfo[FN_ATAN].type = MATH;
    finfo[FN_ATAN].func.math = atan;
    /* 指数関数 */
    finfo[FN_EXP].type = MATH;
    finfo[FN_EXP].func.math = exp;
    /* 自然対数 */
    finfo[FN_LN].type = MATH;
    finfo[FN_LN].func.math = log;
    /* 常用対数 */
    finfo[FN_LOG].type = MATH;
    finfo[FN_LOG].func.math = log10;
    /* 角度をラジアンに変換 */
    finfo[FN_RAD].type = FUNC1;
    finfo[FN_RAD].func.func1 = get_rad;
    /* ラジアンを角度に変換 */
    finfo[FN_DEG].type = FUNC1;
    finfo[FN_DEG].func.func1 = get_deg;
    /* 階乗 */
    finfo[FN_FACT].type = FUNC1;
    finfo[FN_FACT].func.func1 = get_factorial;
    /* 順列 */
    finfo[FN_PERM].type = FUNC2;
    finfo[FN_PERM].func.func2 = get_permutation;
    /* 組み合わせ */
    finfo[FN_COMB].type = FUNC2;
    finfo[FN_COMB].func.func2 = get_combination;
}

/**
 * pi取得
 *
 * @param[in] tsd calcinfo構造体
 * @return pi
 */
static dbl
get_pi(calcinfo *tsd)
{
    dbglog("start");
    if (is_error(tsd))
        return EX_ERROR;
    return DEF_PI;
}

/**
 * ネイピア数(オイラー数)を取得
 *
 * @param[in] tsd calcinfo構造体
 * @return ネイピア数(オイラー数)
 */
static dbl
get_e(calcinfo *tsd)
{
    dbglog("start");
    if (is_error(tsd))
        return EX_ERROR;
    return DEF_E;
}

/**
 * 角度をラジアンに変換
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] x 値
 * @return ラジアン
 */
static dbl
get_rad(calcinfo *tsd, dbl x)
{
    dbglog("start");
    if (is_error(tsd))
        return EX_ERROR;
    return (x * DEF_PI / 180);
}

/**
 * ラジアンを角度に変換
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] x 値
 * @return 角度
 */
static dbl
get_deg(calcinfo *tsd, dbl x)
{
    dbglog("start");
    if (is_error(tsd))
        return EX_ERROR;
    return (x * 180 / DEF_PI);
}

/**
 * 平方根
 *
 * 平方根には, 値域エラーは存在しない.
 * @param[in] tsd calcinfo構造体
 * @param[in] x 値
 * @return 平方根
 */
static dbl
get_sqrt(calcinfo *tsd, dbl x)
{
    dbl result = 0; /* 計算結果 */

    dbglog("start: x=%g", x);

    if (is_error(tsd))
        return EX_ERROR;

    if (isless(x, 0)) { /* 定義域エラー */
        set_errorcode(tsd, E_NAN);
        return EX_ERROR;
    }

    result = sqrt(x);
    return result;
}

/**
 * 関数エラーチェック
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] x 値
 * @param[in] callback コールバック関数
 * @return 絶対値
 */
static dbl
check_math(calcinfo *tsd, dbl x, dbl (*callback)(dbl))
{
    dbl result = 0; /* 計算結果 */

    dbglog("start");

    if (is_error(tsd))
        return EX_ERROR;

    clear_math_feexcept();

    result = callback(x);

    check_math_feexcept(tsd, result);

    return result;
}

/**
 * 階乗
 *
 * n! = n * (n - 1)!
 *
 * @param[in] x 値
 * @param[in] n 値
 * @return なし
 */
static void
factorial(dbl *x, dbl n)
{
    if (n <= 1)
        return;
    *x = (*x) * n;
    factorial(x, n - 1);
}

/**
 * 階乗取得
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] n 値
 * @return 階乗
 */
static dbl
get_factorial(calcinfo *tsd, dbl n)
{
    dbl result = 0;     /* 計算結果 */
    dbl decimal = 0;    /* 小数 */
    dbl integer = 0;    /* 整数 */
    bool minus = false; /* マイナス */

    dbglog("start");

    if (is_error(tsd))
        return EX_ERROR;

    dbglog("n=%.15g", n);

    /* 自然数かどうかチェック */
    decimal = modf(n, &integer);
    dbglog("decimal=%g, integer=%g", decimal, integer);
    if (decimal) { /* 自然数ではない */
        set_errorcode(tsd, E_NAN);
        return EX_ERROR;
    }

    if (isless(n, 0)) { /* マイナス */
        n = check_math(tsd, n, fabs);
        minus = true;
    }

    result = 1;
    factorial(&result, n);

    dbglog("result=%.15g", result);
    if (minus)
        result *= -1;
    minus = false;

    dbglog("result=%.15g", result);
    check_validate(tsd, result);

    return result;
}

/**
 * 順列
 *
 * 異なる n 個の整数から r 個の整数を取り出す\n
 * 順列 nPr を求める関数.\n
 * nPr = n! / (n - r)！
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] n 値
 * @param[in] r 値
 * return 順列
 */
static dbl
get_permutation(calcinfo *tsd, dbl n, dbl r)
{
    dbl result = 0;   /* 計算結果 */
    dbl x = 0, y = 1; /* 値 */

    dbglog("start");

    if (is_error(tsd))
        return EX_ERROR;

    if (isless(n, 0) || isless(r, 0) ||
        isless(n, r)) { /* 定義域エラー */
        set_errorcode(tsd, E_NAN);
        return EX_ERROR;
    }

    x = get_factorial(tsd, n);
    dbglog("x=%.15g", x);
    if (isgreater((n - r), 0))
        y = get_factorial(tsd, n - r);

    dbglog("x=%.15g, y=%.15g", x, y);

    result = x / y;
    dbglog("result=%.15g", result);

    check_validate(tsd, result);

    return result;
}

/**
 * 組み合わせ
 *
 * 異なる n 個の整数から r 個の整数を取り出す\n
 * 組み合わせ数 nCr を求める関数.\n
 * nCr = n! / r! (n - r)！
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] n 値
 * @param[in] r 値
 * return 組み合わせ
 */
static dbl
get_combination(calcinfo *tsd, dbl n, dbl r)
{
    dbl result = 0;          /* 計算結果 */
    dbl x = 0, y = 0, z = 1; /* 値 */

    dbglog("start");

    if (is_error(tsd))
        return EX_ERROR;

    if (isless(n, 0) || isless(r, 0) ||
        isless(n, r)) { /* 定義域エラー */
        set_errorcode(tsd, E_NAN);
        return EX_ERROR;
    }

    x = get_factorial(tsd, n);
    y = get_factorial(tsd, r);
    if (isgreater((n - r), 0))
        z = get_factorial(tsd, n - r);
    dbglog("x=%.15g, y=%.15g, z=%.15g", x, y, z);

    result = x / (y * z);
    dbglog("result=%.15g", result);

    check_validate(tsd, result);

    return result;
}

#ifdef UNITTEST
void
test_init_function(testfunction *func)
{
    func->get_pi = get_pi;
    func->get_e = get_e;
    func->get_rad = get_rad;
    func->get_deg = get_deg;
    func->get_sqrt = get_sqrt;
    func->check_math = check_math;
    func->factorial = factorial;
    func->get_factorial = get_factorial;
    func->get_permutation = get_permutation;
    func->get_combination = get_combination;
    func->fstring = fstring;
}
#endif /* UNITTEST */
