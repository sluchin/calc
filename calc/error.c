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
#include <string.h> /* strlen */
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
 * @attention 呼び出し元で, clear_error()すること.
 */
uchar *
get_errormsg(calcinfo *tsd)
{
    size_t slen = 0;   /* 文字列長 */
    uchar *msg = NULL; /* エラーメッセージ */

    dbglog("start: errorcode=%d", tsd->errorcode);
    assert(MAXERROR == NELEMS(errormsg));

    if (tsd->errorcode == E_NONE)
        return NULL;

    slen = strlen(errormsg[tsd->errorcode]) + 1;
    dbglog("slen=%zu", slen);
    msg = (uchar *)malloc(slen * sizeof(uchar));
    if (!msg) {
        outlog("malloc: slen=%zu", slen);
        return NULL;
    }
    (void)memset(msg, 0, slen);
    (void)strncpy((char *)msg, errormsg[tsd->errorcode], slen);
    dbglog("errormsg=%s, errorcode=%d",
           errormsg[tsd->errorcode], (int)tsd->errorcode);

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

    memfree((void **)&tsd->errormsg, NULL);
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

#ifdef _DEBUG
    int retval = 0;
    if ((retval = isnan(val)) != 0)
        dbglog("isnan(%g)=%d", val, retval);
    if ((retval = isinf(val)) != 0)
        dbglog("isinf(%g)=%d", val, retval);
    if ((retval = fpclassify(val)) == FP_SUBNORMAL)
        dbglog("fpclassify(%g)=%d", val, retval);
#endif /* _DEBUG */
}

/**
 * 浮動小数点例外チェック
 *
 * math.h で宣言されている数学関数使用時,\n
 * 浮動小数点例外をチェックする.
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
#ifdef _DEBUG
    int retval = 0;
    if ((retval = fetestexcept(FE_INVALID)) != 0)
        dbglog("fetestexcept(FE_INVALID)=%d)", retval);
    if ((retval = fetestexcept(FE_DIVBYZERO)) != 0)
        dbglog("fetestexcept(FE_DIVBYZERO)=%d)", retval);
    if ((retval = fetestexcept(FE_OVERFLOW)) != 0)
        dbglog("fetestexcept(FE_OVERFLOW)=%d)", retval);
    if ((retval = fetestexcept(FE_UNDERFLOW)) != 0)
        dbglog("fetestexcept(FE_UNDERFLOW)=%d)", retval);
    if ((retval = fetestexcept(FE_INEXACT)) != 0)
        dbglog("fetestexcept(FE_INEXACT)=%d)", retval);
#endif /* _DEBUG */
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
    feclearexcept(FE_ALL_EXCEPT);
    errno = 0;
}

#ifdef UNITTEST
void
test_init_error(testerror *error)
{
    error->errormsg = errormsg;
}
#endif /* UNITTEST */

