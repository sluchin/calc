/**
 * @file tests/lib/test_timer.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-23 higashi 新規作成
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

#include <unistd.h> /* read STDERR_FILENO */
#include <errno.h>  /* errno */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "fileio.h"
#include "timer.h"

#define BUF_SIZE 256

DEFINE_FFF_GLOBALS;

/* gettimeofday() は, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(int, gettimeofday, struct timeval *, void *);
TEST_PASSTHROUGH(int, gettimeofday, (struct timeval *tv, void *tz), (tv, tz))

/* プロトタイプ */
/** print_timer() 関数テスト */
TEST test_print_timer(void);
/** start_timer() 関数テスト */
TEST test_start_timer(void);
/** stop_timer() 関数テスト */
TEST test_stop_timer(void);
/** get_time() 関数テスト */
TEST test_get_time(void);
/** get_time() 関数テスト (失敗) */
TEST test_get_time_failure(void);
/** stop_timer() 関数テスト (時刻が一周する) */
TEST test_stop_timer_wrap(void);

/**
 * print_timer() 関数テスト
 *
 * @return なし
 */
TEST
test_print_timer(void)
{
    unsigned int t = 0, time = 0; /* タイマ用変数 */
    int fd = -1;                  /* ファイルディスクリプタ */
    int retval = 0;               /* 戻り値 */
    char actual[BUF_SIZE] = {0};  /* 実際の文字列 */
    const char expected[] =       /* 期待する文字列 */
        "time of time: [0-9]+\\.[0-9]+\\[msec\\]";

    start_timer(&t);
    time = stop_timer(&t);
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        TEST_FAIL("pipe_fd=%d(%d)", fd, errno);
    }

    print_timer(time);

    retval = read(fd, actual, sizeof(actual));
    if (retval < 0) {
        TEST_FAIL("read=%d(%d)", fd, errno);
        goto error_handler;
    }
    dbglog("actual=%s", actual);

    TEST_ASSERT_MATCH_MSG(expected, actual, "expected=%s actual=%s",
                                 expected, actual);

error_handler:
    close_fd(&fd, NULL);
    PASS();
}

/**
 * start_timer() 関数テスト
 *
 * @return なし
 */
TEST
test_start_timer(void)
{
    unsigned int t = 0; /* タイマ用変数 */

    start_timer(&t);
    TEST_ASSERT_NOT_INT(0, t);
    PASS();
}

/**
 * stop_timer() 関数テスト
 *
 * @return なし
 */
TEST
test_stop_timer(void)
{
    unsigned int t = 0, time = 0; /* タイマ用変数 */

    start_timer(&t);
    dbglog("t=%u", t);
    (void)usleep(1000); /* 最適化されると, 経過時間が 0 になることがあるので待つ */
    time = stop_timer(&t);
    dbglog("time=%u", time);
    ASSERT((time) > (0));
    PASS();
}

/**
 * get_time() 関数テスト
 *
 * @return なし
 */
TEST
test_get_time(void)
{
    unsigned long long t = 0; /* 戻り値 */

    t = get_time();
    TEST_ASSERT_NOT_INT(0, (unsigned int)t);
    PASS();
}


/**
 * 初期化処理
 *
 * @return なし
 */
static void
setup(void *data)
{
    (void)data;
    TEST_PASSTHROUGH_RESET(gettimeofday);
}

/**
 * get_time() 関数テスト (失敗)
 *
 * @return なし
 */
TEST
test_get_time_failure(void)
{
    /* gettimeofday() に失敗すると, 0 を返す */
    TEST_INJECT(gettimeofday, 0, 1, -1, EFAULT);
    TEST_ASSERT_INT(0, get_time());
    TEST_ASSERT_INJECTED(gettimeofday);
    PASS();
}

static long long fake_time = 0; /**< gettimeofday() のモックが返す時刻 (マイクロ秒) */

/**
 * gettimeofday() のモック動作 (fake_time を返す)
 *
 * @param[out] tv timeval構造体
 * @param[in] tz 使用しない
 * @return 0
 */
static int
fake_gettimeofday(struct timeval *tv, void *tz)
{
    (void)tz;
    tv->tv_sec = (time_t)(fake_time / 1000000);
    tv->tv_usec = (suseconds_t)(fake_time % 1000000);
    return 0;
}

/**
 * stop_timer() 関数テスト (32 ビットの時刻が一周する)
 *
 * @return なし
 */
TEST
test_stop_timer_wrap(void)
{
    unsigned int t = 0; /* タイマ用変数 */

    gettimeofday_fake.custom_fake = fake_gettimeofday;

    /* 通常 */
    fake_time = 5000000000LL + 1000;
    start_timer(&t);
    fake_time += 500;
    TEST_ASSERT_INT(500, stop_timer(&t));

    /* 開始が, 一周する直前 (0xFFFFFF00) で, 終了が, 一周したあと (0x100) */
    t = 0xFFFFFF00U;
    fake_time = 0x100000100LL;
    TEST_ASSERT_INT(0x200, stop_timer(&t));
    PASS();
}

GREATEST_MAIN_DEFS();

int
main(int argc, char **argv)
{
    TEST_MAIN_BEGIN();
    SET_SETUP(setup, NULL);
    RUN_TEST(test_print_timer);
    RUN_TEST(test_start_timer);
    RUN_TEST(test_stop_timer);
    RUN_TEST(test_get_time);
    RUN_TEST(test_get_time_failure);
    RUN_TEST(test_stop_timer_wrap);
    TEST_MAIN_END();
}
