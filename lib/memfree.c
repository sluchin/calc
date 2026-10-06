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
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdlib.h> /* free */
#include <stdarg.h> /* va_list va_arg va_end */
#include <string.h> /* memcpy */

#include "log.h"
#include "memfree.h"

static void free_pointer(void *addr);

/**
 * @brief メモリ解放
 *
 * freeした後, NULLを代入する.
 * 例: memfree(&pointer, NULL);
 * 引数はポインタ変数のアドレス (どの型のポインタでもキャストは要らない).
 *
 * @param[in,out] ptr freeするポインタ変数のアドレス
 * @param[in,out] ... 可変引数 (ポインタ変数のアドレス)
 * @attention 最後の引数はNULLにすること.
 */
void
memfree(void *ptr, ...)
{
    void *mem = NULL; /* ポインタ変数のアドレス */
    va_list ap;       /* va_list */

    dbglog("start: ptr=%p", ptr);
    dbgtrace();

    /* 最初のポインタ */
    free_pointer(ptr);

    va_start(ap, ptr);

    /* 続くポインタ (最後は NULL) */
    mem = va_arg(ap, void *);
    while (mem != NULL) {
        free_pointer(mem);
        mem = va_arg(ap, void *);
    }

    va_end(ap);
}

/**
 * ポインタを解放して, NULL を代入する
 *
 * ポインタ変数の型 (char * や struct xxx * など) は, 呼び出し側ごとに違う.
 * void ** にキャストして読み書きすると, 別の型で参照することになる (厳密なエイリアス規則
 * に反する) ので memcpy() で読み書きする.
 *
 * @param[in,out] addr ポインタ変数のアドレス
 */
static void
free_pointer(void *addr)
{
    void *mem = NULL; /* ポインタ */

    (void)memcpy(&mem, addr, sizeof(mem));
    dbglog("mem=%p", mem);
    if (mem != NULL)
        free(mem);
    mem = NULL;
    (void)memcpy(addr, &mem, sizeof(mem));
}
