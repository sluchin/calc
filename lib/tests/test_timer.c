/**
 * @file lib/tests/test_timer.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-23 higashi 新規作成
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

#include <unistd.h> /* pipe fork */
#include <errno.h>  /* errno */
#include <cutter.h> /* cutter library */

#include "def.h"
#include "log.h"
#include "timer.h"
#include "test_common.h"

#define BUF_SIZE 256

/* プロトタイプ */
/** print_timer() 関数テスト */
void test_print_timer(void);
/** start_timer() 関数テスト */
void test_start_timer(void);
/** stop_timer() 関数テスト */
void test_stop_timer(void);
/** get_time() 関数テスト */
void test_get_time(void);

/**
 * print_timer() 関数テスト
 *
 * @return なし
 */
void
test_print_timer(void)
{
    uint t = 0, time = 0;        /* タイマ用変数 */
    int fd = -1;                 /* ファイルディスクリプタ */
    int retval = 0;              /* 戻り値 */
    char actual[BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =      /* 期待する文字列 */ 
        "time of time: [0-9]+\\.[0-9]+\\[msec\\]";

    start_timer(&t);
    time = stop_timer(&t);
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        cut_error("pipe_fd=%d(%d)", fd, errno);
        return;
    }

    print_timer(time);

    retval = read(fd, actual, sizeof(actual));
    if (retval < 0) {
        cut_fail("read=%d(%d)", fd, errno);
        goto error_handler;
    }
    dbglog("actual=%s", actual);

    cut_assert_match(expected, actual,
                     cut_message("expected=%s actual=%s",
                                 expected, actual));

error_handler:
    close_fd(&fd, NULL);
}

/**
 * start_timer() 関数テスト
 *
 * @return なし
 */
void
test_start_timer(void)
{
    uint t = 0; /* タイマ用変数 */

    start_timer(&t);
    cut_assert_not_equal_uint(0, t);
}

/**
 * stop_timer() 関数テスト
 *
 * @return なし
 */
void
test_stop_timer(void)
{
    uint t = 0, time = 0; /* タイマ用変数 */

    start_timer(&t);
    time = stop_timer(&t);
    cut_assert_operator(time, >, 0);
}

/**
 * get_time() 関数テスト
 *
 * @return なし
 */
void
test_get_time(void)
{
    unsigned long long t = 0; /* 戻り値 */

    t = get_time();
    cut_assert_not_equal_uint(0, (uint)t);
}

