/**
 * @file  error.c
 * @brief エラー設定取得
 *
 * @sa error.h
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
#include <math.h>   /* isnan isinf fpclassify */
#include <fenv.h>   /* FE_INVALID */

#include "log.h"
#include "error.h"

/** エラーメッセージ */
static const char *errormsg[] = {
    NULL,
    "Divide by zero.",
    "Syntax error.",
    "Function not defined.",
    "Math error.",
    "Not a Number"
    "Overflow.",
    "Underflow."
};

/** エラーコード */
static ER errorcode = E_NONE;

/**
 * エラーメッセージ取得
 *
 * @return エラーメッセージ
 * @attention 呼び出し元で, clear_error()すること.
 */
uchar *
get_errormsg(void)
{
    dbglog("start");

    size_t slen = 0;   /* 文字列長 */
    uchar *msg = NULL; /* エラーメッセージ */

    if (errorcode != E_NONE) {
        slen = strlen(errormsg[errorcode]) + 1;
        msg = (uchar *)malloc(slen * sizeof(uchar));
        if (!msg) {
            outlog("malloc[%p]", msg);
            return NULL;
        }
        (void)memset(msg, 0, slen);
        (void)strncpy((char *)msg, errormsg[errorcode], slen + 1);
    }
    return msg;
}

/**
 * エラーコード設定
 *
 * param[in] error エラー種別
 * @return なし
 */
void
set_errorcode(ER error)
{
    dbglog("start");

    if (errorcode == E_NONE)
        errorcode = error;
    dbglog("errorcode[%d]", errorcode);
}

/**
 * エラークリア
 *
 * @param[in] msg 領域解放するポインタ
 * @return なし
 */
void
clear_error(uchar *msg)
{
    dbglog("start");

    if (msg)
        free(msg);
    msg = NULL;
    errorcode = E_NONE;
}

/**
 * エラー判定
 *
 * @retval true エラー
 */
bool
is_error(void)
{
    dbglog("start");

    if (errorcode) {
        dbglog("errorcode[%d]", errorcode);
        return true;
    }
    return false;
}

/**
 * 数値の妥当性チェック
 *
 * @param[in] val 数値
 * @return なし
 */
void
check_validate(ldfl val)
{
    dbglog("start");

    if (isnan(val))
        set_errorcode(E_NAN);
    else if (isinf(val))
        set_errorcode(E_OVERFLOW);
    else if (fpclassify(val) == FP_SUBNORMAL)
        set_errorcode(E_UNDERFLOW);
}

/**
 * 浮動小数点例外チェック
 *
 * math.h で宣言されている数学関数使用時,\n
 * 浮動小数点例外をチェックする.
 *
 * @return なし
 */
void
check_math_feexcept(void)
{
    dbglog("start");

    if (fetestexcept(FE_INVALID))
        set_errorcode(E_NAN);
    else if (fetestexcept(FE_DIVBYZERO))
        set_errorcode(E_ZERO);
    else if (fetestexcept(FE_OVERFLOW))
        set_errorcode(E_OVERFLOW);
    else if (fetestexcept(FE_UNDERFLOW))
        set_errorcode(E_UNDERFLOW);
}

