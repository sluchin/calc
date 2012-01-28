/**
 * @file lib/tests/test_memfree.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-22 higashi 新規作成
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

#include <stdlib.h> /* malloc */
#include <cutter.h> /* cutter library */

#include "def.h"
#include "log.h"
#include "memfree.h"

/* プロトタイプ */
/** memfree() 関数テスト */
void test_memfree(void);

/**
 * set_memfree() 関数テスト
 *
 * @return なし
 */
void
test_memfree(void)
{
    char *mem[] = { NULL, NULL, NULL }; /* ポインタ値 */
    enum { MEM1, MEM2, MEM3, MAX };     /* 配列要素 */

    int i;
    for (i = 0; i < MAX; i++) {
        mem[i] = (char *)malloc(5 * sizeof(char));
        if (!mem[i]) {
            cut_error("malloc");
            return;
        }
    }
    memfree((void **)&mem[MEM1],
            (void **)&mem[MEM2], (void **)&mem[MEM3], NULL);
    cut_assert_null(mem[MEM1]);
    cut_assert_null(mem[MEM2]);
    cut_assert_null(mem[MEM3]);

    /* 第二引数がNULLの場合 */
    mem[MEM1] = (char *)malloc(5 * sizeof(char));
    if (!mem[MEM1]) {
        cut_error("malloc");
        return;
    }
    mem[MEM3] = (char *)malloc(5 * sizeof(char));
    if (!mem[MEM3]) {
        cut_error("malloc");
        return;
    }
    memfree((void **)&mem[MEM1],
            (void **)&mem[MEM2], (void **)&mem[MEM3], NULL);
    cut_assert_null(mem[MEM1]);
    cut_assert_null(mem[MEM3]);
}

