/**
 * @file  calc/tests/test_common.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-09-24 higashi 新規作成
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

#include <cutter.h> /* cutter library */

#include "log.h"
#include "calc.h"

/**
 * 文字列設定
 *
 * @param[in] str 文字列
 * @return calcinfo構造体
 */
calcinfo *
set_string(const char *str)
{
    size_t length = 0;    /* 文字列長 */
    uchar *expr = NULL;   /* 式 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    length = strlen(str);
    expr = (uchar *)cut_take_strndup(str, length);
    if (!expr) {
        outlog("cut_take_strndup=%p", expr);
        return NULL;
    }

    tsd = init_calc(expr, 12L);
    if (!tsd) {
        outlog("tsd=%p, %p expr=%s", tsd, expr, expr);
        return NULL;
    }
    dbglog("tsd=%p", tsd);
    dbglog("%p expr=%s, length=%u", expr, expr, length);
    return tsd;
}

