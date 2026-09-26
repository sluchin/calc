/**
 * @file tests/calcc/test_main.c
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

#include <stdio.h>  /* fprintf */
#include <stdlib.h> /* exit EXIT_SUCCESS */
#include <signal.h> /* raise */

#include "test_helper.h"
#include "test_process.h"
#include "def.h"
#include "log.h"
#include "net.h"
#include "client.h"
#include "option.h"

DEFINE_FFF_GLOBALS;

/* main.c が呼び出す, option.c と client.c と net.c の関数は, モックにする */
FAKE_VOID_FUNC(parse_args, int, char **);
FAKE_VALUE_FUNC(int, connect_sock);
FAKE_VALUE_FUNC(st_client, client_loop, int);
FAKE_VALUE_FUNC(int, close_sock, int *);

#define SOCKFD 5 /**< connect_sock() が返すソケット */

/** 親子プロセスで共有する情報 */
struct shared {
    int parse_count; /**< parse_args() の呼び出し回数 */
    int loop_count;  /**< client_loop() の呼び出し回数 */
    int loop_sock;   /**< client_loop() のソケット */
    int loop_signal; /**< client_loop() で g_sig_handled */
    int close_count; /**< close_sock() の呼び出し回数 */
    int close_sock;  /**< close_sock() のソケット */
};

/** main() (main.c) */
int calcc_main(int argc, char *argv[]);

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
    (void)argc;
    (void)argv;
    shm->parse_count++;
    if (raise_signo)
        (void)raise(raise_signo);
}

/**
 * client_loop() のモック動作
 *
 * @param[in] sock ソケット
 * @return EX_SUCCESS
 */
static st_client
fake_client_loop(int sock)
{
    shm->loop_count++;
    shm->loop_sock = sock;
    shm->loop_signal = g_sig_handled;
    return EX_SUCCESS;
}

/**
 * close_sock() のモック動作
 *
 * @param[in] sock ソケット
 * @return EX_OK
 */
static int
fake_close_sock(int *sock)
{
    shm->close_count++;
    shm->close_sock = *sock;
    return EX_OK;
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
    RESET_FAKE(connect_sock);
    RESET_FAKE(client_loop);
    RESET_FAKE(close_sock);
    FFF_RESET_HISTORY();
    parse_args_fake.custom_fake = fake_parse_args;
    connect_sock_fake.return_val = SOCKFD;
    client_loop_fake.custom_fake = fake_client_loop;
    close_sock_fake.custom_fake = fake_close_sock;
    raise_signo = 0;
    g_sig_handled = 0;
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
    char *argv[] = { "calcc", NULL };

    (void)arg;
    (void)calcc_main(1, argv);
}

/**
 * main() 関数テスト (正常)
 *
 * @return なし
 */
TEST
test_main_success(void)
{
    TEST_ASSERT_INT(EX_SUCCESS, test_run_child(run_main, NULL, NULL, NULL, 0));
    TEST_ASSERT_INT(1, shm->parse_count);
    TEST_ASSERT_INT(1, shm->loop_count);
    TEST_ASSERT_INT(SOCKFD, shm->loop_sock);
    /* 終了時に, ソケットをクローズしている (atexit) */
    TEST_ASSERT_INT(1, shm->close_count);
    TEST_ASSERT_INT(SOCKFD, shm->close_sock);
    PASS();
}

/**
 * main() 関数テスト (client_loop() のステータスで終了する)
 *
 * @return なし
 */
TEST
test_main_status(void)
{
    client_loop_fake.custom_fake = NULL;
    client_loop_fake.return_val = EX_QUIT;
    TEST_ASSERT_INT(EX_QUIT, test_run_child(run_main, NULL, NULL, NULL, 0));
    client_loop_fake.return_val = EX_RECV_ERR;
    TEST_ASSERT_INT(EX_RECV_ERR, test_run_child(run_main, NULL, NULL, NULL, 0));
    PASS();
}

/**
 * main() 関数テスト (接続できない)
 *
 * @return なし
 */
TEST
test_main_connect_failure(void)
{
    char out[256] = {0}; /* 出力 */

    connect_sock_fake.return_val = EX_NG;
    TEST_ASSERT_INT(EX_CONNECT_ERR,
                    test_run_child(run_main, NULL, NULL, out, sizeof(out)));
    TEST_ASSERT_STR("Connect error\n", out);
    TEST_ASSERT_INT(0, shm->loop_count);
    /* 接続できなくても, atexit で close_sock() が呼ばれる */
    TEST_ASSERT_INT(1, shm->close_count);
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
    /* シグナルを受け取ると, g_sig_handled が設定される */
    raise_signo = SIGINT;
    TEST_ASSERT_INT(EX_SUCCESS, test_run_child(run_main, NULL, NULL, NULL, 0));
    TEST_ASSERT_INT(1, shm->loop_signal);
    PASS();
}

GREATEST_MAIN_DEFS();

int
main(int argc, char **argv)
{
    TEST_MAIN_BEGIN();
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    RUN_TEST(test_main_success);
    RUN_TEST(test_main_status);
    RUN_TEST(test_main_connect_failure);
    RUN_TEST(test_main_signal);
    TEST_MAIN_END();
}
