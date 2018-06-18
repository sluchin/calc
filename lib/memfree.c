/**
 * @file  lib/memfree.c
 * @brief メモリ解放
 *
 * @author higashi
 * @date 2010-06-24 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
 */
/* This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by * the Free Software Foundation; either version 2 of the License, or * (at your option) any later version.
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
#include <stdarg.h> /* va_list va_arg va_end */

#include "log.h"
#include "memfree.h"

/**
 * メモリ解放
 *
 * freeした後, NULLを代入する.
 * 例: memfree((void **)pointer, NULL);
 * void **にキャストしなければ警告が出る.
 *
 * @param[in,out] ptr freeするポインタ
 * @param[in,out] ... 可変引数
 * @return なし
 * @attention 最後の引数はNULLにすること.
 */
void
memfree(void** ptr, ...)
{
    void **mem = NULL; /* ポインタ */
    va_list ap;        /* va_list */

    dbglog("start: ptr=%p", *ptr);
    dbgtrace();

    if (*ptr)
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

