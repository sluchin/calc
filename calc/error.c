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
#include <math.h>   /* isnan isinf fpclassify */
#include <fenv.h>   /* FE_INVALID */
#include <stdarg.h> /* va_list va_arg */

#include "log.h"
#include "error.h"

/** エラーメッセージ */
static const char *errormsg[] = {
    NULL,
    "Divide by zero.",
    "Syntax error.",
    "Function not defined.",
    "Math error.",
    "Not a Number.",
    "Overflow.",
    "Underflow.",
    "Positive infinity.",
    "Negative infinity.",
    "Too small to be represented in normalized format."
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
            outlog("malloc=%p", msg);
            return NULL;
        }
        (void)memset(msg, 0, slen);
        (void)strncpy((char *)msg, errormsg[errorcode], slen + 1);
        dbglog("errormsg=%s, errorcode=%d", errormsg[errorcode], errorcode);
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
    dbglog("errorcode=%d", errorcode);
}

/**
 * エラークリア
 *
 * @param[in] msg 領域解放するポインタ
 * @return なし
 */
void
clear_error(uchar **msg)
{
    dbglog("start");

    if (*msg)
        free(*msg);
    *msg = NULL;
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

    if (errorcode)
        return true;

    dbglog("errorcode=%d", errorcode);

    return false;
}

/**
 * 数値の妥当性チェック
 *
 * @param[in] num 引数の数
 * @param[in] ... double型引数
 * @return なし
 */
void
check_validate(int num, ...)
{
    dbl val = 0; /* 値 */
    va_list ap;  /* va_list */
    int i;       /* 汎用変数 */

    dbglog("start");

    va_start(ap, num);

    for (i = 0; i < num; i++) {
        val = va_arg(ap, double);

#ifndef _DEBUG
        if (isnan(val))
            set_errorcode(E_NAN);
        else if (isinf(val) == 1)
            set_errorcode(E_PLUSINF);
        else if (isinf(val) == -1)
            set_errorcode(E_MINUSINF);
        else if (fpclassify(val) == FP_SUBNORMAL)
            set_errorcode(E_NORMSMALL);
#else
        int retval = 0; 
        if ((retval = isnan(val)) != 0) {
            dbglog("isnan(%g)=%d", val, retval);
            set_errorcode(E_NAN);
        }
        if ((retval = isinf(val)) == 1) {
            dbglog("isinf(%g)=%d", val, retval);
            set_errorcode(E_PLUSINF);
        }
        if ((retval = isinf(val)) == -1) {
            dbglog("isinf(%g)=%d", val, retval);
            set_errorcode(E_MINUSINF);
        }
        if ((retval = fpclassify(val)) == FP_SUBNORMAL) {
            dbglog("fpclassify(%g)=%d", val, retval);
            set_errorcode(E_NORMSMALL);
        }
#endif
    }
    va_end(ap);
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
check_math_feexcept(dbl val)
{
    dbglog("start: val=%g", val);

#ifndef _DEBUG
    if (fetestexcept(FE_INVALID)) {
        set_errorcode(E_NAN);
    } else if (fetestexcept(FE_DIVBYZERO)) {
        if (val == HUGE_VALL) {
            set_errorcode(E_PLUSINF);
        } else if (val == -HUGE_VALL) {
            set_errorcode(E_MINUSINF);
        }
    } else if (fetestexcept(FE_OVERFLOW)) {
        set_errorcode(E_OVERFLOW);
    } else if (fetestexcept(FE_UNDERFLOW)) {
        set_errorcode(E_UNDERFLOW);
    }
#else
    int retval = 0;
    if ((retval = fetestexcept(FE_INVALID)) != 0) {
        dbglog("fetestexcept(FE_INVALID)=%d)", retval);
        set_errorcode(E_NAN);
    }
    if ((retval = fetestexcept(FE_DIVBYZERO)) != 0) {
        dbglog("fetestexcept(FE_DIVBYZERO)=%d)", retval);
        if (val == HUGE_VALL) {
            dbglog("%g==HUGE_VALL", val);
            set_errorcode(E_PLUSINF);
        } else if (val == -HUGE_VALL) {
            dbglog("%g==-HUGE_VALL", val);
            set_errorcode(E_MINUSINF);
        }
    }
    if ((retval = fetestexcept(FE_OVERFLOW)) != 0) {
        dbglog("fetestexcept(FE_OVERFLOW)=%d)", retval);
        set_errorcode(E_OVERFLOW);
    }
    if ((retval = fetestexcept(FE_UNDERFLOW)) != 0) {
        dbglog("fetestexcept(FE_UNDERFLOW)=%d)", retval);
        set_errorcode(E_UNDERFLOW);
    }
#endif
}

