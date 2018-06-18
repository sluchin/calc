/**
 * @file lib/tests/test_data.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-19 higashi 新規作成
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

#include "def.h"
#include "log.h"
#include "data.h"

#define ALIGNOF(type)  offsetof(struct { char dummy; type var; } , var)
#define ALIGN  8  /**< アライメント */

const char *test_data[] = {
    "a",
    "aa",
    "aaa",
    "aaaa",
    "aaaaa",
    "aaaaaa",
    "aaaaaaa",
    "aaaaaaaa"
};

/* プロトタイプ */
/** set_client_data() 関数テスト */
void test_set_client_data(void);
/** set_server_data() 関数テスト */
void test_set_server_data(void);

/**
 * 初期化処理
 *
 * @return なし
 */
void
cut_startup(void)
{
    dbglog("char=%zu", ALIGNOF(char));
    dbglog("short=%zu", ALIGNOF(short));
    dbglog("int=%zu", ALIGNOF(int));
    dbglog("long=%zu", ALIGNOF(long));
    dbglog("double=%zu", ALIGNOF(double));
}

/**
 * set_client_data() 関数テスト
 *
 * @return なし
 */
void
test_set_client_data(void)
{
    size_t length = 0;
    ssize_t len = 0;
    struct client_data *dt = NULL;

    unsigned int i;
    for (i = 0; i < NELEMS(test_data); i++) {
        length = strlen(test_data[i]) + 1;
        dbglog("length=%zu", length);
        len = set_client_data(&dt, (unsigned char *)test_data[i], length);
        dbglog("len=%zd, %s", len, test_data[i]);
        cut_assert_equal_int(0, len % ALIGN);
        dbglog("dt=%p", dt);
        cut_assert_not_null(dt);
        if (dt)
            free(dt);
        dt = NULL;
    }
}

/**
 * set_server_data() 関数テスト
 *
 * @return なし
 */
void
test_set_server_data(void)
{
    size_t length = 0;
    ssize_t len = 0;
    struct server_data *dt = NULL;

    unsigned int i;
    for (i = 0; i < NELEMS(test_data); i++) {
        length = strlen(test_data[i]) + 1;
        dbglog("length=%zu", length);
        len = set_server_data(&dt, (unsigned char *)test_data[i], length);
        dbglog("len=%zd, %s", len, test_data[i]);
        cut_assert_equal_int(0, len % ALIGN);
        dbglog("dt=%p", dt);
        cut_assert_not_null(dt);
        if (dt)
            free(dt);
        dt = NULL;
    }
}

