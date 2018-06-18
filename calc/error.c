/**
 * @file  calc/error.c
 * @brief エラー設定取得
 *
 * @author higashi
 * @date 2011-08-15 higashi 新規作成
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

#include <stdio.h>  /* NULL */
#include <string.h> /* strdup */
#include <math.h>   /* isnan isinf fpclassify */
#include <fenv.h>   /* fetestexcept FE_INVALID */
#include <errno.h>  /* errno */
#include <assert.h> /* assert */

#include "log.h"
#include "error.h"

/** エラーメッセージ文字列構造体 */
static const char *errormsg[] = {
    NULL,
    "Divide by zero.",
    "Syntax error.",
    "Function not defined.",
    "NaN.",
    "Infinity."
};

/**
 * エラーメッセージ取得
 *
 * @param[in] calc calcinfo構造体
 * @return エラーメッセージ
 * @attention 呼び出し元で, clear_error()すること.\n
 *            戻り値ポインタは解放しなければならない.
 */
unsigned char *
get_errormsg(calcinfo *calc)
{
    unsigned char *msg = NULL; /* エラーメッセージ */

    dbglog("start: errorcode=%d", (int)calc->errorcode);
    assert(MAXERROR == NELEMS(errormsg));

    if (calc->errorcode <= E_NONE ||
        MAXERROR <= calc->errorcode)
        return NULL;

    dbglog("errormsg=%s, errorcode=%d",
           errormsg[calc->errorcode], (int)calc->errorcode);

    msg = (unsigned char *)strdup(errormsg[calc->errorcode]);
    if (!msg) {
        outlog("strdup");
        return NULL;
    }

    return msg;
}

/**
 * エラーコード設定
 *
 * @param[in] calc calcinfo構造体
 * @param[in] error エラー種別
 * @return なし
 */
void
set_errorcode(calcinfo *calc, ER error)
{
    dbglog("start: %d", (int)error);

    if (calc->errorcode == E_NONE)
        calc->errorcode = error;
}

/**
 * エラークリア
 *
 * @param[in] calc calcinfo構造体
 * @return なし
 */
void
clear_error(calcinfo *calc)
{
    dbglog("start");
    calc->errorcode = E_NONE;
}

/**
 * エラー判定
 *
 * @param[in] calc calcinfo構造体
 * @retval true エラー
 */
bool
is_error(calcinfo *calc)
{
    dbglog("start: errorcode=%d", (int)calc->errorcode);

    if (calc->errorcode)
        return true;

    return false;
}

/**
 * 数値の妥当性チェック
 *
 * @param[in] calc calcinfo構造体
 * @param[in] val 値
 * @return なし
 */
void
check_validate(calcinfo *calc, double val)
{
    dbglog("start");

    if (isnan(val))
        set_errorcode(calc, E_NAN);
    else if (isinf(val) != 0)
        set_errorcode(calc, E_INFINITY);

    dbglog("isnan=%d, isinf=%d, fpclassify=%d",
           isnan(val), isinf(val), fpclassify(val));
}

/**
 * 浮動小数点例外チェック
 *
 * @param[in] calc calcinfo構造体
 * @return なし
 */
void
check_math_feexcept(calcinfo *calc)
{
    dbglog("start");

    if (fetestexcept(FE_DIVBYZERO |
                     FE_OVERFLOW  |
                     FE_UNDERFLOW)) {
        set_errorcode(calc, E_INFINITY);
    } else {
        if (fetestexcept(FE_INVALID))
            set_errorcode(calc, E_NAN);
    }
    dbglog("FE_INVALID=%d, FE_DIVBYZERO=%d, FE_OVERFLOW=%d, " \
           "FE_UNDERFLOW=%d, FE_INEXACT=%d",
           fetestexcept(FE_INVALID),
           fetestexcept(FE_DIVBYZERO),
           fetestexcept(FE_OVERFLOW),
           fetestexcept(FE_UNDERFLOW),
           fetestexcept(FE_INEXACT));
}

/**
 * 浮動小数点例外チェッククリア
 *
 * 浮動小数点例外をチェックする前に必ずクリアする.
 *
 * @return なし
 */
void
clear_math_feexcept(void)
{
    if (feclearexcept(FE_ALL_EXCEPT)) /* エラー(非0) */
        outlog("feclearexcept");
    errno = 0;
}

#ifdef UNITTEST
void
test_init_error(testerror *error)
{
    error->errormsg = errormsg;
}
#endif /* UNITTEST */

