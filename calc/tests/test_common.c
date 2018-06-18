/**
 * @file  calc/tests/test_common.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-09-24 higashi 新規作成
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


#include <stdio.h>  /* snprintf */
#include <cutter.h> /* cutter library */

#include "log.h"
#include "def.h"
#include "calc.h"
#include "test_common.h"

/**
 * 文字列設定
 *
 * @param[in] calc calcinfo構造体
 * @param[in] str 文字列
 * @return なし
 */
void
set_string(calcinfo *calc, const char *str)
{
    size_t length = 0;          /* 文字列長 */
    unsigned char *expr = NULL; /* 式 */
    int retval = 0;             /* 戻り値 */

    length = strlen(str);
    expr = (unsigned char *)cut_take_strndup(str, length);
    if (!expr)
        cut_error("cut_take_strndup=%p", expr);

    calc->ptr = expr;

    /* フォーマット設定 */
    retval = snprintf(calc->fmt, sizeof(calc->fmt),
                      "%s%ld%s", "%.", 12L, "g");
    if (retval < 0)
        cut_error("snprintf");

    dbglog("fmt=%s", calc->fmt);
    dbglog("calc=%p", calc);
    dbglog("%p expr=%s, length=%u", expr, expr, length);
}

