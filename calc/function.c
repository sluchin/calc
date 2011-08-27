/**
 * @file  function.c
 * @brief 関数
 *
 * @sa function.h
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

#include "log.h"
#include "error.h"
#include "function.h"

/** pi(4*atan(1)) */
#define DEF_PI       (ldfl)3.14159265358979323846264338327950288
/** ネイピア数(オイラー数) */
#define DEF_E        (ldfl)2.71828182845904523536028747135266249

/** Pi取得 */
static ldfl get_pi(const struct arg_value *val);
/** ネイピア数(オイラー数)取得 */
static ldfl get_e(const struct arg_value *val);
/** 絶対値取得 */
static ldfl get_abs(const struct arg_value *val);
/** 平方根取得 */
static ldfl get_sqrt(const struct arg_value *val);
/** 三角関数取得 */
static ldfl get_sin(const struct arg_value *val);
/** 三角関数取得 */
static ldfl get_cos(const struct arg_value *val);
/** 三角関数取得 */
static ldfl get_tan(const struct arg_value *val);
/** 逆三角関数取得 */
static ldfl get_asin(const struct arg_value *val);
/** 逆三角関数取得 */
static ldfl get_acos(const struct arg_value *val);
/** 逆三角関数取得 */
static ldfl get_atan(const struct arg_value *val);
/** 指数関数取得 */
static ldfl get_exp(const struct arg_value *val);
/** 自然対数取得 */
static ldfl get_ln(const struct arg_value *val);
/** 常用対数取得 */
static ldfl get_log(const struct arg_value *val);
/** 角度をラジアンに変換 */
static ldfl get_rad(const struct arg_value *val);
/** ラジアンを角度に変換 */
static ldfl get_deg(const struct arg_value *val);
/** 階乗 */
static ldfl factorial(ldfl n);
/** 階乗取得 */
static ldfl get_factorial(const struct arg_value *val);
/** 順列(nPr) */
static ldfl get_permutation(const struct arg_value *val);
/** 組み合わせ(nCr) */
static ldfl get_combination(const struct arg_value *val);

static const struct _st_func {
    int argnum;
    char funcname[MAX_FUNC_STRING + 1];
    ldfl (*func)(const struct arg_value *);
} st_func[] = {
    { ARG_0, "pi",   &get_pi },          /**< pi */
    { ARG_0, "e",    &get_e },           /**< ネイピア数(オイラー数) */
    { ARG_1, "abs",  &get_abs },         /**< 絶対値 */
    { ARG_1, "sqrt", &get_sqrt },        /**< 平方根 */
    { ARG_1, "sin",  &get_sin },         /**< 三角関数(sin) */
    { ARG_1, "cos",  &get_cos },         /**< 三角関数(cosin) */
    { ARG_1, "tan",  &get_tan },         /**< 三角関数(tangent) */
    { ARG_1, "asin", &get_asin },        /**< 逆三角関数(arcsin) */
    { ARG_1, "acos", &get_acos },        /**< 逆三角関数(arccosin) */
    { ARG_1, "atan", &get_atan },        /**< 逆三角関数(arctangent) */
    { ARG_1, "exp",  &get_exp },         /**< 指数関数 */
    { ARG_1, "ln",   &get_ln },          /**< 自然対数 */
    { ARG_1, "log",  &get_log },         /**< 常用対数 */
    { ARG_1, "deg",  &get_deg },         /**< ラジアンを角度に変換 */
    { ARG_1, "rad",  &get_rad },         /**< 角度をラジアンに変換 */
    { ARG_1, "n",    &get_factorial },   /**< 階乗 */
    { ARG_2, "nPr",  &get_permutation }, /**< 順列 */
    { ARG_2, "nCr",  &get_combination }  /**< 組み合わせ */
};

/** 構造体配列要素数 */
static const int st_func_num = sizeof(st_func) / sizeof(st_func[0]);

/**
 * pi取得
 *
 * @param[in] val 値
 * @return pi
 */
static ldfl
get_pi(const struct arg_value *val)
{
    dbglog("start");
    return DEF_PI;
}

/**
 * ネイピア数(オイラー数)を取得
 *
 * @param[in] val 値
 * @return ネイピア数(オイラー数)
 */
static ldfl
get_e(const struct arg_value *val)
{
    dbglog("start");
    return DEF_E;
}

/**
 * 絶対値取得
 *
 * @param[in] val 値
 * @return 絶対値
 */
static ldfl
get_abs(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = fabsl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 平方根取得
 *
 * @param[in] val 値
 * @return 平方根
 */
static ldfl
get_sqrt(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = sqrtl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 三角関数取得
 *
 * @param[in] val 値
 * @return 三角関数(sin)
 */
static ldfl
get_sin(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = sinl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 三角関数取得
 *
 * @param[in] val 値
 * @return 三角関数(cos)
 */
static ldfl
get_cos(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = cosl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 三角関数取得
 *
 * @param[in] val 値
 * @return 三角関数(tan)
 */
static ldfl
get_tan(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = tanl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 逆三角関数取得
 *
 * @param[in] val 値
 * @return 逆三角関数(asin)
 */
static ldfl
get_asin(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = asinl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 逆三角関数取得
 *
 * @param[in] val 値
 * @return 逆三角関数(acos)
 */
static ldfl
get_acos(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = acosl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 逆三角関数取得
 *
 * @param[in] val 値
 * @return 逆三角関数(atan)
 */
static ldfl
get_atan(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = atanl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 指数関数取得
 *
 * @param[in] val 値
 * @return 指数関数
 */
static ldfl
get_exp(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = expl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 自然対数取得
 *
 * @param[in] val 値
 * @return 自然対数
 */
static ldfl
get_ln(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = logl(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 常用対数取得
 *
 * @param[in] val 値
 * @return 常用対数
 */
static ldfl
get_log(const struct arg_value *val)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    errno = 0;
    feclearexcept(FE_ALL_EXCEPT);

    result = log10l(val->x);

    check_math_feexcept();

    return result;
}

/**
 * 角度をラジアンに変換
 *
 * @param[in] val 値
 * @return ラジアン
 */
static ldfl
get_rad(const struct arg_value *val)
{
    dbglog("start");
    return (val->x * DEF_PI / 180);
}

/**
 * ラジアンを角度に変換
 *
 * @param[in] val 値
 * @return 角度
 */
static ldfl
get_deg(const struct arg_value *val)
{
    dbglog("start");
    return (val->x * 180 / DEF_PI);
}

/**
 * 階乗
 *
 * @param[in] n 値
 * @return 階乗
 */
static ldfl
factorial(ldfl n)
{
    ldfl result = 0; /* 計算結果 */

    dbglog("start: result[%.18Lg]", result);
    dbglog("n[%.18Lg]", n);

    check_validate(n);
    if (is_error())
        return result;

    if (n == 1 || n == 0)
        result = 1;
    else
        result = n * factorial(n - 1);

    check_validate(result);

    dbglog("result[%.18Lg]", result);
    return result;
}

/**
 * 階乗取得
 *
 * @param[in] val 値
 * @return 階乗
 */
static ldfl
get_factorial(const struct arg_value *val)
{
    ldfl result = 0;   /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    if (is_error())
        return result;

    dbglog("val->x[%.18Lg]", val->x);
    result = val->x * factorial(val->x - 1);
    dbglog("result[%.18Lg]", result);

    check_validate(result);

    return result;
}

/**
 * 順列
 *
 * 異なる n 個の整数から r 個の整数を取り出す\n
 * 順列 nPr を求める関数.\n
 * nPr = n! / (n - r)！
 *
 * @param[in] val 値
 * return nPr
 */
static ldfl
get_permutation(const struct arg_value *val)
{
    ldfl x = 0, y = 1; /* 値 */
    ldfl result = 0;   /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    check_validate(val->y);
    if (is_error())
        return result;

    if (isless(val->x, 0) ||
        isless(val->y, 0) ||
        isless(val->x, val->y)) {
        /* 計算不能 */
        set_errorcode(E_MATH);
        return 0;
    }

    x = factorial(val->x);
    dbglog("x[%.18Lg] y[%.18Lg]", x, y);
    if ((val->x - val->y) > 0)
        y = factorial(val->x - val->y);

    dbglog("x[%.18Lg] y[%.18Lg]", x, y);

    result = x / y;
    dbglog("result[%.18Lg]", result);

    check_validate(result);

    return result;
}

/**
 * 組み合わせ
 *
 * 異なる n 個の整数から r 個の整数を取り出す\n
 * 組み合わせ数 nCr を求める関数.\n
 * nCr = n! / r! (n - r)！
 *
 * @param[in] val 値
 * return nCr
 */
static ldfl
get_combination(const struct arg_value *val)
{
    ldfl x = 0, y = 1, z = 1; /* 値 */
    ldfl result = 0;          /* 計算結果 */

    dbglog("start");

    check_validate(val->x);
    check_validate(val->y);
    if (is_error())
        return result;

    if (isless(val->x, 0) ||
        isless(val->y, 0) ||
        isless(val->x, val->y)) {
        /* 計算不能 */
        set_errorcode(E_MATH);
        return 0;
    }

    x = factorial(val->x);
    y = factorial(val->x);
    if (isgreater((val->x - val->y), 0))
        z = factorial(val->x - val->y);
    dbglog("x[%.18Lg] y[%.18Lg] z[%.18Lg]", x, y, z);

    result = x / (y * z);
    dbglog("result[%.18Lg]", result);

    check_validate(result);

    return result;
}

/**
 * 関数実行
 *
 * @param[in] val  引数
 * @param[in] func 関数名
 * return なし
 */
ldfl
exec_func(struct arg_value *val, const char *func)
{
    ldfl result = 0; /* 戻り値 */
    int i;           /* 変数 */
    bool exec;       /* 関数実行フラグ */

    dbglog("start");

    /* 初期値設定 */
    val->x = val->y = 0;

    for (i = 0, exec = false; i < st_func_num && !exec; i++) {
        if (!strcmp(st_func[i].funcname, func)) {
            if (st_func[i].argnum == ARG_1 ||
                st_func[i].argnum == ARG_2)
                parse_func_args(val, st_func[i].argnum);

            dbglog("i[%d] x[%.18Lg] y[%.18Lg] funcname[%s]",
                   i, val->x, val->y, st_func[i].funcname);
            result = st_func[i].func(val);
            exec = true;
        }
    }
    if (!exec) /* エラー */
        set_errorcode(E_NOFUNC);

    dbglog("result[%.18Lg]", result);
    return result;
}

