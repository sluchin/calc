/**
 * @file tests/calcp/test_main.c
 * @brief 単体テスト (main.c)
 *
 * @author higashi
 * @date 2026-09-27 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2026 Tetsuya Higashi. All Rights Reserved.
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
#include <stdlib.h> /* exit EXIT_SUCCESS */
#include <signal.h> /* raise */

#include "test_helper.h"
#include "test_process.h"
#include "def.h"
#include "log.h"
#include "option.h"

DEFINE_FFF_GLOBALS;

/* main.c が呼び出す, option.c の関数は, モックにする */
FAKE_VOID_FUNC(parse_args, int, char **);

#define BUF_SIZE 1024 /**< バッファサイズ */

/** 親子プロセスで共有する情報 */
struct shared {
    int parse_count;   /**< parse_args() の呼び出し回数 */
    int parse_argc;    /**< parse_args() の argc */
    char parse_argv0[32]; /**< parse_args() の argv[0] */
};

/** main() (main.c) */
int calcp_main(int argc, char *argv[]);

/* 内部変数 */
static struct shared *shm = NULL; /**< 共有メモリ */
static int raise_signo = 0;       /**< parse_args() で発生させるシグナル */

/**
 * parse_args() のモック動作
 *
 * @param[in] argc 引数の数
 * @param[in] argv 引数
 * @return なし
 */
static void
fake_parse_args(int argc, char **argv)
{
    shm->parse_count++;
    shm->parse_argc = argc;
    (void)snprintf(shm->parse_argv0, sizeof(shm->parse_argv0), "%s", argv[0]);
    if (raise_signo)
        (void)raise(raise_signo);
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
    RESET_FAKE(parse_args);
    FFF_RESET_HISTORY();
    parse_args_fake.custom_fake = fake_parse_args;
    raise_signo = 0;
    shm = (struct shared *)test_shared_alloc(sizeof(*shm));
}

/**
 * 終了処理
 *
 * @return なし
 */
static void
teardown(void *data)
{
    (void)data;
    test_shared_free(shm, sizeof(*shm));
    shm = NULL;
}

/**
 * main() を子プロセスで実行するための関数
 *
 * @param[in] arg 使用しない
 * @return なし
 */
static void
run_main(void *arg)
{
    char *argv[] = { "calcp", NULL };

    (void)arg;
    (void)calcp_main(1, argv);
}

/**
 * main() 関数テスト (計算)
 *
 * @return なし
 */
TEST
test_main_calc(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */
    int status = 0;           /* 終了ステータス */

    /* quit で終了 */
    status = test_run_child(run_main, NULL, "100*3\n2+3*4\nquit\n",
                            out, sizeof(out));
    TEST_ASSERT_INT(EXIT_SUCCESS, status);
    TEST_ASSERT_MSG(strstr(out, "300") != NULL, "out=%s", out);
    TEST_ASSERT_MSG(strstr(out, "14") != NULL, "out=%s", out);

    /* parse_args() は, argc と argv で 1 回呼ばれる */
    TEST_ASSERT_INT(1, shm->parse_count);
    TEST_ASSERT_INT(1, shm->parse_argc);
    TEST_ASSERT_STR("calcp", shm->parse_argv0);
    PASS();
}

/**
 * main() 関数テスト (exit, 空行)
 *
 * @return なし
 */
TEST
test_main_exit(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    /* exit で終了 (以降の式は実行されない) */
    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child(run_main, NULL, "100*3\nexit\n200*3\n",
                                   out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "300") != NULL, "out=%s", out);
    TEST_ASSERT_MSG(strstr(out, "600") == NULL, "out=%s", out);

    /* 空行は無視する */
    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child(run_main, NULL, "\n100*3\nquit\n",
                                   out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "300") != NULL, "out=%s", out);
    PASS();
}

/**
 * main() 関数テスト (計算できない式)
 *
 * @return なし
 */
TEST
test_main_error(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    /* エラーになっても, 次の式を実行する */
    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child(run_main, NULL, "1/0\n100*3\nquit\n",
                                   out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "Divide by zero") != NULL, "out=%s", out);
    TEST_ASSERT_MSG(strstr(out, "300") != NULL, "out=%s", out);
    PASS();
}

/**
 * main() 関数テスト (シグナル)
 *
 * @return なし
 */
TEST
test_main_signal(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    /* シグナルを受け取ると, ループを終了する */
    raise_signo = SIGINT;
    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child(run_main, NULL, "100*3\n200*3\n",
                                   out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "600") == NULL, "out=%s", out);
    PASS();
}

GREATEST_MAIN_DEFS();

int
main(int argc, char **argv)
{
    TEST_MAIN_BEGIN();
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(test_main_calc);
    RUN_TEST(test_main_exit);
    RUN_TEST(test_main_error);
    RUN_TEST(test_main_signal);
    TEST_MAIN_END();
}
