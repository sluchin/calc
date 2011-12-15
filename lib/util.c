/**
 * @file  lib/util.c
 * @brief ユーティリティ
 *
 * @author higashi
 * @date 2010-06-24 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
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

#include <stdlib.h> /* free */
#include <stdarg.h> /* va_list va_arg */

#include "log.h"
#include "util.h"

/**
 * メモリ解放
 *
 * freeした後, NULLを代入する.
 * 最後の引数はNULLにする.
 * memfree((void **)pointer, NULL);
 *
 * @param[in,out] ptr freeするポインタ
 * @param[in,out] ... 可変引数
 * @return なし
 */
void
memfree(void** ptr, ...)
{
    va_list ap;        /* va_list */
    void **mem = NULL; /* ポインタ */

    if (!*ptr)
        return;
    else
        free(*ptr);
    *ptr = NULL;

    va_start(ap, ptr);

    while ((mem = va_arg(ap, void **)) != NULL) {
        dbglog("mem=%p", *mem);
        if (*mem)
            free(*mem);
        *mem = NULL;
    }

    va_end(ap);
}

/**
 * チェックサム
 *
 * チェックサムの計算をする.
 *
 * @param[in] addr
 * @param[in] len 長さ
 * @return チェックサム値
 */
ushort
in_cksum(ushort *addr, const size_t len)
{
    int nleft = (int)len;
    int sum = 0;
    ushort *w = addr;
    ushort answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1) {
        *(uchar *)(&answer) = *(uchar *)w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return answer;
}

