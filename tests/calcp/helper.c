/**
 * @file  tests/calcp/helper.c
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
#include <stdlib.h> /* free exit */
#include <string.h> /* strndup strlen */

#include "test_helper.h"
#include "log.h"
#include "def.h"
#include "calc.h"
#include "helper.h"

#define MAX_STRINGS 256 /**< 保持できる文字列数 */

/* 内部変数 */
static char *strings[MAX_STRINGS]; /**< 確保した文字列 */
static size_t nstrings = 0;        /**< 確保した文字列数 */

/**
 * 文字列設定
 *
 * @param[in] calc calcinfo構造体
 * @param[in] str 文字列
 */
void
set_string(calcinfo *calc, const char *str)
{
    size_t length = 0;          /* 文字列長 */
    unsigned char *expr = NULL; /* 式 */
    int retval = 0;             /* 戻り値 */

    length = strlen(str);
    if (nstrings >= MAX_STRINGS) {
        (void)printf("set_string: too many strings\n");
        exit(EXIT_FAILURE);
    }
    expr = (unsigned char *)strndup(str, length);
    if (!expr) {
        (void)printf("set_string: strndup=%p\n", (void *)expr);
        exit(EXIT_FAILURE);
    }
    strings[nstrings++] = (char *)expr; /* free_strings() で解放する */

    calc->ptr = expr;

    /* フォーマット設定 */
    retval = snprintf(calc->fmt, sizeof(calc->fmt),
                      "%s%ld%s", "%.", 12L, "g");
    if (retval < 0) {
        (void)printf("set_string: snprintf\n");
        exit(EXIT_FAILURE);
    }

    dbglog("fmt=%s", calc->fmt);
    dbglog("calc=%p", (void *)calc);
    dbglog("%p expr=%s, length=%zu", (void *)expr, expr, length);
}

/**
 * set_string() で確保した文字列の解放
 */
void
free_strings(void)
{
    size_t i;

    for (i = 0; i < nstrings; i++) {
        free(strings[i]);
        strings[i] = NULL;
    }
    nstrings = 0;
}
