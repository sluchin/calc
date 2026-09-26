/**
 * @file tests/calcd/test_main.c
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
#include <unistd.h> /* environ */

#include "test_helper.h"
#include "test_process.h"
#include "def.h"
#include "log.h"
#include "net.h"
#include "server.h"
#include "option.h"

DEFINE_FFF_GLOBALS;

/* main.c が呼び出す, option.c と server.c と net.c の関数は, モックにする */
FAKE_VOID_FUNC(parse_args, int, char **);

/* main.c が呼び出す関数の失敗は, 子プロセスの中で注入する (本物を呼ぶ素通し) */
FAKE_VALUE_FUNC(int, sigaction, int, const struct sigaction *,
                struct sigaction *);
TEST_PASSTHROUGH(int, sigaction,
                 (int signo, const struct sigaction *act,
                  struct sigaction *oldact),
                 (signo, act, oldact))
FAKE_VALUE_FUNC(int, sigemptyset, sigset_t *);
TEST_PASSTHROUGH(int, sigemptyset, (sigset_t *set), (set))
FAKE_VALUE_FUNC(int, sigfillset, sigset_t *);
TEST_PASSTHROUGH(int, sigfillset, (sigset_t *set), (set))
FAKE_VALUE_FUNC(int, server_sock);
FAKE_VOID_FUNC(server_loop, int);
FAKE_VALUE_FUNC(int, close_sock, int *);
#ifndef _DEBUG
FAKE_VALUE_FUNC(int, daemon, int, int);
#endif

#define SOCKFD 7 /**< server_sock() が返すソケット */

/** 親子プロセスで共有する情報 */
struct shared {
    int parse_count; /**< parse_args() の呼び出し回数 */
    int loop_count;  /**< server_loop() の呼び出し回数 */
    int loop_sock;   /**< server_loop() のソケット */
    int loop_signal; /**< server_loop() で g_sig_handled */
    int close_count; /**< close_sock() の呼び出し回数 */
    int daemon_count; /**< daemon() の呼び出し回数 */
    int close_sock;  /**< close_sock() のソケット */
};

extern char **environ; /**< 環境変数 */

/** main() (main.c) */
int calcd_main(int argc, char *argv[], char *envp[]);

/* 内部変数 */
static struct shared *shm = NULL; /**< 共有メモリ */
static int raise_signo = 0;       /**< server_loop() で発生させるシグナル */

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
}

/**
 * server_loop() のモック動作
 *
 * @param[in] sock ソケット
 * @return なし
 */
static void
fake_server_loop(int sock)
{
    shm->loop_count++;
    shm->loop_sock = sock;
    if (raise_signo)
        (void)raise(raise_signo);
    shm->loop_signal = g_sig_handled;
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

#ifndef _DEBUG
/**
 * daemon() のモック動作 (デーモン化はしない)
 *
 * @param[in] nochdir 使用しない
 * @param[in] noclose 使用しない
 * @return 0
 */
static int
fake_daemon(int nochdir, int noclose)
{
    (void)nochdir;
    (void)noclose;
    shm->daemon_count++;
    return 0;
}
#endif

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
    TEST_PASSTHROUGH_RESET(sigaction);
    TEST_PASSTHROUGH_RESET(sigemptyset);
    TEST_PASSTHROUGH_RESET(sigfillset);
    RESET_FAKE(server_sock);
    RESET_FAKE(server_loop);
    RESET_FAKE(close_sock);
#ifndef _DEBUG
    RESET_FAKE(daemon);
#endif
    FFF_RESET_HISTORY();
#ifndef _DEBUG
    daemon_fake.custom_fake = fake_daemon;
#endif
    parse_args_fake.custom_fake = fake_parse_args;
    server_sock_fake.return_val = SOCKFD;
    server_loop_fake.custom_fake = fake_server_loop;
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
 * @param[in] arg argv (NULL終端)
 * @return なし
 */
static void
run_main(void *arg)
{
    char **argv = (char **)arg; /* 引数 */
    int argc = 0;               /* 引数の数 */

    while (argv[argc])
        argc++;
    (void)calcd_main(argc, argv, environ);
}

/**
 * main() 関数テスト (正常)
 *
 * @return なし
 */
TEST
test_main_success(void)
{
    char *argv[] = { "calcd", NULL };

    TEST_ASSERT_INT(EXIT_SUCCESS, test_run_child(run_main, argv, NULL, NULL, 0));
    TEST_ASSERT_INT(1, shm->parse_count);
    TEST_ASSERT_INT(1, shm->loop_count);
    TEST_ASSERT_INT(SOCKFD, shm->loop_sock);
    /* ソケットをクローズしている */
    TEST_ASSERT_INT(1, shm->close_count);
#ifndef _DEBUG
    /* デーモン化している */
    TEST_ASSERT_INT(1, shm->daemon_count);
#endif
    PASS();
}

/**
 * main() 関数テスト (ソケットを作れない)
 *
 * @return なし
 */
TEST
test_main_server_sock_failure(void)
{
    char *argv[] = { "calcd", NULL };

    server_sock_fake.return_val = EX_NG;
    TEST_ASSERT_INT(EXIT_FAILURE,
                    test_run_child(run_main, argv, NULL, NULL, 0));
    TEST_ASSERT_INT(1, shm->parse_count);
    TEST_ASSERT_INT(0, shm->loop_count);
    PASS();
}

#ifndef _DEBUG
/**
 * main() 関数テスト (デーモン化できない)
 *
 * @return なし
 */
TEST
test_main_daemon_failure(void)
{
    char *argv[] = { "calcd", NULL };

    daemon_fake.custom_fake = NULL;
    daemon_fake.return_val = -1;
    TEST_ASSERT_INT(EXIT_FAILURE,
                    test_run_child(run_main, argv, NULL, NULL, 0));
    TEST_ASSERT_INT(0, shm->loop_count);
    PASS();
}
#endif

/**
 * main() 関数テスト (シグナル)
 *
 * @return なし
 */
TEST
test_main_signal(void)
{
    char *argv[] = { "calcd", NULL };

    /* SIGTERM: ループを終了して, 正常終了する */
    raise_signo = SIGTERM;
    TEST_ASSERT_INT(EXIT_SUCCESS, test_run_child(run_main, argv, NULL, NULL, 0));
    TEST_ASSERT_INT(1, shm->loop_signal);
    TEST_ASSERT_INT(1, shm->close_count);
    PASS();
}

/**
 * main() 関数テスト (SIGHUP で再起動)
 *
 * @return なし
 */
TEST
test_main_sighup(void)
{
    /* argv[0] を再実行する. sh に 42 で終了させて, 再実行を確認する */
    char *argv[] = { "/bin/sh", "-c", "exit 42", NULL };

    raise_signo = SIGHUP;
    TEST_ASSERT_INT(42, test_run_child(run_main, argv, NULL, NULL, 0));
    TEST_ASSERT_INT(1, shm->loop_count);
    TEST_ASSERT_INT(1, shm->close_count);
    PASS();
}

/**
 * 失敗を注入して, main() を子プロセスで実行するための関数
 * (注入は, 親プロセスの test_run_child() が消費しないように, 子プロセスで行う)
 *
 * @param[in] arg argv (NULL終端)
 * @return なし
 */
static void
run_main_failure(void *arg)
{
    /* シグナルハンドラの設定 (get 側と set 側で, 11 のシグナル分) */
    TEST_INJECT(sigemptyset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigfillset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigaction, 0, 22, -1, EINVAL);
    run_main(arg);
}

/**
 * main() 関数テスト (シグナルハンドラの設定に失敗)
 *
 * @return なし
 */
TEST
test_main_failure(void)
{
    char *argv[] = { "calcd", NULL };

    /* 失敗しても, 続行する */
    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child(run_main_failure, argv, NULL, NULL, 0));
    TEST_ASSERT_INT(1, shm->loop_count);
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
    RUN_TEST(test_main_server_sock_failure);
#ifndef _DEBUG
    RUN_TEST(test_main_daemon_failure);
#endif
    RUN_TEST(test_main_signal);
    RUN_TEST(test_main_sighup);
    RUN_TEST(test_main_failure);
    TEST_MAIN_END();
}
