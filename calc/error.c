/**
 * @file  calc/error.c
 * @brief エラー設定取得
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

#include <stdio.h>  /* NULL */
#include <stdlib.h> /* malloc free */
#include <string.h> /* strlen strdup */
#include <errno.h>  /* errno */
#include <math.h>   /* isnan isinf fpclassify */
#include <fenv.h>   /* fetestexcept FE_INVALID */
#include <assert.h> /* assert */

#include "log.h"
#include "memfree.h"
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
 * @param[in] tsd calcinfo構造体
 * @return エラーメッセージ
 * @attention 呼び出し元で, clear_error()すること.\n
 *            戻り値ポインタは解放しなければならない.
 */
uchar *
get_errormsg(calcinfo *tsd)
{
    uchar *msg = NULL; /* エラーメッセージ */

    dbglog("start: errorcode=%d", tsd->errorcode);
    assert(MAXERROR == NELEMS(errormsg));

    if (tsd->errorcode == E_NONE)
        return NULL;

    dbglog("errormsg=%s, errorcode=%d",
           errormsg[tsd->errorcode], (int)tsd->errorcode);

    msg = (uchar *)strdup(errormsg[tsd->errorcode]);
    if (!msg) {
        outlog("strdup");
        return NULL;
    }

    return msg;
}

/**
 * エラーコード設定
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] error エラー種別
 * @return なし
 */
void
set_errorcode(calcinfo *tsd, ER error)
{
    dbglog("start: %d", (int)error);

    if (tsd->errorcode == E_NONE)
        tsd->errorcode = error;
}

/**
 * エラークリア
 *
 * @param[in] tsd calcinfo構造体
 * @return なし
 */
void
clear_error(calcinfo *tsd)
{
    dbglog("start");
    tsd->errorcode = E_NONE;
}

/**
 * エラー判定
 *
 * @param[in] tsd calcinfo構造体
 * @retval true エラー
 */
bool
is_error(calcinfo *tsd)
{
    dbglog("start: errorcode=%d", (int)tsd->errorcode);

    if (tsd->errorcode)
        return true;

    return false;
}

/**
 * 数値の妥当性チェック
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] val 値
 * @return なし
 */
void
check_validate(calcinfo *tsd, dbl val)
{
    dbglog("start");

    if (isnan(val))
        set_errorcode(tsd, E_NAN);
    else if (isinf(val) != 0)
        set_errorcode(tsd, E_INFINITY);

    dbglog("isnan=%d, isinf=%d, fpclassify=%d",
           isnan(val), isinf(val), fpclassify(val));
}

/**
 * 浮動小数点例外チェック
 *
 * @param[in] tsd calcinfo構造体
 * @return なし
 */
void
check_math_feexcept(calcinfo *tsd)
{
    dbglog("start");

    if (fetestexcept(FE_DIVBYZERO |
                     FE_OVERFLOW  |
                     FE_UNDERFLOW)) {
        set_errorcode(tsd, E_INFINITY);
    } else {
        if (fetestexcept(FE_INVALID))
            set_errorcode(tsd, E_NAN);
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

