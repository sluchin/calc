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
#include "calc.h"
#include "function.h"

/** pi(4*atan(1)) */
#define DEF_PI       3.14159265358979323846264338327950288
/** ネイピア数(オイラー数) */
#define DEF_E        2.71828182845904523536028747135266249

/** 配列要素数 */
#define arraysize(a)    sizeof(a) / sizeof(a[0])

/* 内部関数 */
/** 関数情報構造体初期化 */
static void init_func(void) __attribute__((constructor));
/** Pi取得 */
static dbl get_pi(void);
/** ネイピア数(オイラー数)取得 */
static dbl get_e(void);
/** 関数エラーチェック */
static dbl check_math(dbl x, dbl (*callback)(dbl));
/** 角度をラジアンに変換 */
static dbl get_rad(dbl x);
/** ラジアンを角度に変換 */
static dbl get_deg(dbl x);
/** 階乗取得 */
static dbl get_factorial(dbl n);
/** 順列(nPr) */
static dbl get_permutation(dbl n, dbl r);
/** 組み合わせ(nCr) */
static dbl get_combination(dbl n, dbl r);

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
    char funcname[MAX_FUNC_STRING + 1];
};

struct funcstring fstring[] = {
    { "pi"   }, /**< pi */
    { "e"    }, /**< ネイピア数(オイラー数) */
    { "abs"  }, /**< 絶対値 */
    { "sqrt" }, /**< 平方根 */
    { "sin"  }, /**< 三角関数(sin) */
    { "cos"  }, /**< 三角関数(cosin) */
    { "tan"  }, /**< 三角関数(tangent) */
    { "asin" }, /**< 逆三角関数(arcsin) */
    { "acos" }, /**< 逆三角関数(arccosin) */
    { "atan" }, /**< 逆三角関数(arctangent) */
    { "exp"  }, /**< 指数関数 */
    { "ln"   }, /**< 自然対数 */
    { "log"  }, /**< 常用対数 */
    { "rad"  }, /**< 角度をラジアンに変換 */
    { "deg"  }, /**< ラジアンを角度に変換 */
    { "n"    }, /**< 階乗 */
    { "nPr"  }, /**< 順列 */
    { "nCr"  }  /**< 組み合わせ */
};

/** 関数共用体 */
union func {
    dbl (*func0)(void);
    dbl (*func1)(dbl x);
    dbl (*func2)(dbl x, dbl y);
};

/** 関数情報構造体 */
struct funcinfo {
    enum functype type;
    union func func;
};

/** 関数情報構造体配列 */
static struct funcinfo finfo[MAXFUNC];

/**
 * 関数情報構造体初期化
 *
 * @return なし
 */
static void
init_func(void)
{
    dbglog("start");

    assert(arraysize(fstring) == MAXFUNC);
    assert(arraysize(finfo) == MAXFUNC);

    memset(finfo, 0, sizeof(finfo));

    /* pi */
    finfo[FN_PI].type = ARG_0;
    finfo[FN_PI].func.func0 = get_pi;
    /* ネイピア数(オイラー数) */
    finfo[FN_E].type = ARG_0;
    finfo[FN_E].func.func0 = get_e;
    /* 絶対値 */
    finfo[FN_ABS].type = ARG_1;
    finfo[FN_ABS].func.func1 = fabs;
    /* 平方根 */
    finfo[FN_SQRT].type = ARG_1;
    finfo[FN_SQRT].func.func1 = sqrt;
    /* 三角関数(sin) */
    finfo[FN_SIN].type = ARG_1;
    finfo[FN_SIN].func.func1 = sin;
    /* 三角関数(cosin) */
    finfo[FN_COS].type = ARG_1;
    finfo[FN_COS].func.func1 = cos;
    /* 三角関数(tangent) */
    finfo[FN_TAN].type = ARG_1;
    finfo[FN_TAN].func.func1 = tan;
    /* 逆三角関数(arcsin) */
    finfo[FN_ASIN].type = ARG_1;
    finfo[FN_ASIN].func.func1 = asin;
    /* 逆三角関数(arccosin) */
    finfo[FN_ACOS].type = ARG_1;
    finfo[FN_ACOS].func.func1 = acos;
    /* 逆三角関数(arccosin) */
    finfo[FN_ATAN].type = ARG_1;
    finfo[FN_ATAN].func.func1 = atan;
    /* 指数関数 */
    finfo[FN_EXP].type = ARG_1;
    finfo[FN_EXP].func.func1 = exp;
    /* 自然対数 */
    finfo[FN_LN].type = ARG_1;
    finfo[FN_LN].func.func1 = log;
    /* 常用対数 */
    finfo[FN_LOG].type = ARG_1;
    finfo[FN_LOG].func.func1 = log10;
    /* 角度をラジアンに変換 */
    finfo[FN_RAD].type = ARG_1;
    finfo[FN_RAD].func.func1 = get_rad;
    /* ラジアンを角度に変換 */
    finfo[FN_DEG].type = ARG_1;
    finfo[FN_DEG].func.func1 = get_deg;
    /* 階乗 */
    finfo[FN_FACT].type = ARG_1;
    finfo[FN_FACT].func.func1 = get_factorial;
    /* 順列 */
    finfo[FN_PERM].type = ARG_2;
    finfo[FN_PERM].func.func2 = get_permutation;
    /* 組み合わせ */
    finfo[FN_COMB].type = ARG_2;
    finfo[FN_COMB].func.func2 = get_combination;

}

/**
 * pi取得
 *
 * @return pi
 */
static dbl
get_pi(void)
{
    dbglog("start");
    return DEF_PI;
}

/**
 * ネイピア数(オイラー数)を取得
 *
 * @return ネイピア数(オイラー数)
 */
static dbl
get_e(void)
{
    dbglog("start");
    return DEF_E;
}

/**
 * 角度をラジアンに変換
 *
 * @param[in] x 値
 * @return ラジアン
 */
static dbl
get_rad(dbl x)
{
    dbglog("start");
    return (x * DEF_PI / 180);
}

/**
 * ラジアンを角度に変換
 *
 * @param[in] x 値
 * @return 角度
 */
static dbl
get_deg(dbl x)
{
    dbglog("start");
    return (x * 180 / DEF_PI);
}

/**
 * 関数エラーチェック
 *
 * @param[in] x 値
 * @param[in] callback コールバック関数
 * @return 絶対値
 */
static dbl
check_math(dbl x, dbl (*callback)(dbl))
{
    dbl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(1, x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = callback(x);

    check_math_feexcept(result);

    return result;
}

/**
 * 階乗取得
 *
 * @param[in] val arg_value構造体
 * @return 階乗
 */
static dbl
get_factorial(dbl n)
{
    dbl result = 0;   /* 計算結果 */

    dbglog("start");

    check_validate(1, n);
    if (is_error())
        return result;

    dbglog("n=%.18g", n);
    if (n == 1 || n == 0)
        result = 1;
    else
        result = n * get_factorial(n - 1);
    dbglog("result=%.18g", result);

    check_validate(1, result);

    return result;
}

/**
 * 順列
 *
 * 異なる n 個の整数から r 個の整数を取り出す\n
 * 順列 nPr を求める関数.\n
 * nPr = n! / (n - r)！
 *
 * @param[in] val arg_value構造体
 * return nPr
 */
static dbl
get_permutation(dbl n, dbl r)
{
    dbl result = 0;   /* 計算結果 */
    dbl x = 0, y = 1; /* 値 */

    dbglog("start");

    check_validate(2, n, r);
    if (is_error())
        return result;

    if (isless(n, 0) || isless(r, 0) ||
        isless(n, r)) {
        /* 計算不能 */
        set_errorcode(E_MATH);
        return 0;
    }

    x = get_factorial(n);
    dbglog("x=%.18g", x);
    if (isgreater((n - r), 0))
        y = get_factorial(n - r);

    dbglog("x=%.18g, y=%.18g", x, y);

    result = x / y;
    dbglog("result=%.18g", result);

    check_validate(1, result);

    return result;
}

/**
 * 組み合わせ
 *
 * 異なる n 個の整数から r 個の整数を取り出す\n
 * 組み合わせ数 nCr を求める関数.\n
 * nCr = n! / r! (n - r)！
 *
 * @param[in] val arg_value構造体
 * return nCr
 */
static dbl
get_combination(dbl n, dbl r)
{
    dbl result = 0;          /* 計算結果 */
    dbl x = 0, y = 0, z = 1; /* 値 */

    dbglog("start");

    check_validate(2, n, r);
    if (is_error())
        return result;

    if (isless(n, 0) || isless(r, 0) ||
        isless(n, r)) {
        /* 計算不能 */
        set_errorcode(E_MATH);
        return 0;
    }

    x = get_factorial(n);
    y = get_factorial(r);
    if (isgreater((n - r), 0))
        z = get_factorial(n - r);
    dbglog("x=%.18g, y=%.18g, z=%.18g", x, y, z);

    result = x / (y * z);
    dbglog("result=%.18g", result);

    check_validate(1, result);

    return result;
}

/**
 * 指数取得
 *
 * @param[in] x 値
 * @param[in] y 値
 * @return 指数
 */
dbl
get_pow(dbl x, dbl y)
{
    dbl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(2, x, y);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = pow(x, y);

    check_math_feexcept(result);

    return result;
}

/**
 * 関数実行
 *
 * @param[in] func 関数名
 * return なし
 */
dbl
exec_func(const char *func)
{
    dbl result = 0;   /* 戻り値 */
    dbl x = 0, y = 0; /* 値 */
    int i;            /* 変数 */
    bool exec;        /* 関数実行フラグ */

    dbglog("start");

    for (i = 0, exec = false; i < MAXFUNC && !exec; i++) {
        if (!strcmp(fstring[i].funcname, func)) {
            dbglog("finfo[%d]=%p", finfo[i]);
            switch (finfo[i].type) {
            dbglog("type=%d", finfo[i].type);
            case ARG_0:
                result = finfo[i].func.func0();
                break;
            case ARG_1:
                parse_func_args(ARG_1, &x, &y);
                dbglog("x=%.18g, y=%.18g", x, y);
                result = check_math(x, finfo[i].func.func1);
                break;
            case ARG_2:
                parse_func_args(ARG_2, &x, &y);
                dbglog("x=%.18g, y=%.18g", x, y);
                result = finfo[i].func.func2(x, y);
                break;
            default:
                break;
            }
            exec = true;
        }
    }
    if (!exec) /* エラー */
        set_errorcode(E_NOFUNC);

    dbglog("result=%.18g", result);
    return result;
}

