/**
 * @file lib/tests/test_util.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-22 higashi 新規作成
 * @version \Id
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

#include <stdlib.h>    /* malloc */
#include <arpa/inet.h> /* htons */
#include <cutter.h>    /* cutter library */

#include "def.h"
#include "log.h"
#include "memfree.h"

/* プロトタイプ */
/** memfree() 関数テスト */
void test_memfree(void);

/* 内部関数 */

/**
 * set_memfree() 関数テスト
 *
 * @return なし
 */
void
test_memfree(void)
{
    char *mem1 = NULL; /* ポインタ値 1 */
    char *mem2 = NULL; /* ポインタ値 2 */
    char *mem3 = NULL; /* ポインタ値 3 */

    mem1 = malloc(5);
    if (!mem1)
        cut_error("malloc=%p", mem1);
    mem2 = malloc(5);
    if (!mem2)
        cut_error("malloc=%p", mem2);
    mem3 = malloc(5);
    if (!mem3)
        cut_error("malloc=%p", mem3);
    memfree((void **)&mem1, (void **)&mem2, (void **)&mem3, NULL);
    cut_assert_null(mem1);
    cut_assert_null(mem2);
    cut_assert_null(mem3);

    /* mem2がNULLの場合 */
    mem1 = malloc(5);
    if (!mem1)
        cut_error("malloc=%p", mem1);
    mem3 = malloc(5);
    if (!mem3)
        cut_error("malloc=%p", mem3);
    memfree((void **)&mem1, (void **)&mem2, (void **)&mem3, NULL);
    cut_assert_null(mem1);
    cut_assert_null(mem3);
}

