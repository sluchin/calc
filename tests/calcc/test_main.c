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
FAKE_VALUE_FUNC(int, setvbuf, FILE *, char *, int, size_t);
TEST_PASSTHROUGH(int, setvbuf,
                 (FILE *fp, char *buf, int mode, size_t size),
                 (fp, buf, mode, size))

/*
 * atexit() は, libc の共有ライブラリには無い (静的ライブラリの関数) ので,
 * 本物は, __cxa_atexit() で呼ぶ. テストの実行ファイルにある main.c の呼び出しだけが,
 * このモックになる (共有ライブラリの中の呼び出しは, 置き換えられない).
 */
typedef void (*atexit_func_t)(void);
FAKE_VALUE_FUNC(int, atexit, atexit_func_t);
extern int __cxa_atexit(void (*func)(void *), void *arg, void *dso);
extern void *__dso_handle;
static struct test_inject inject_atexit; /**< atexit() に注入する失敗 */
/**
 * atexit() の素通し (失敗を注入できる)
 *
 * @param[in] func 終了時に呼ぶ関数
 * @return 0, または注入した失敗
 */
static int
pass_atexit(atexit_func_t func)
{
    if (inject_atexit.count > 0) {
        inject_atexit.count--;
        errno = inject_atexit.err;
        return (int)inject_atexit.value;
    }
    return __cxa_atexit((void (*)(void *))func, NULL, __dso_handle);
}
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
    int pipe_ignored; /**< client_loop() で SIGPIPE を無視している */
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
    struct sigaction old; /* 現在の SIGPIPE の設定 */

    shm->loop_count++;
    shm->loop_sock = sock;
    shm->loop_signal = g_sig_handled;
    (void)sigaction(SIGPIPE, NULL, &old);
    shm->pipe_ignored = (old.sa_handler == SIG_IGN);
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
 * @param[in] data 使用しない
 */
static void
setup(void *data)
{
    /* モックと状態を, 初期状態 (素通し) に戻す */
    (void)data;
    RESET_FAKE(parse_args);
    TEST_PASSTHROUGH_RESET(sigaction);
    TEST_PASSTHROUGH_RESET(sigemptyset);
    TEST_PASSTHROUGH_RESET(sigfillset);
    TEST_PASSTHROUGH_RESET(setvbuf);
    RESET_FAKE(atexit);
    (void)memset(&inject_atexit, 0, sizeof(inject_atexit));
    atexit_fake.custom_fake = pass_atexit;
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
 * @param[in] data 使用しない
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
 */
static void
run_main(void *arg)
{
    char *argv[] = { "calcc", NULL }; /* 引数 */

    (void)arg;
    (void)calcc_main(1, argv);
}

/**
 * main() 関数テスト (正常)
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
    /* SIGPIPE を無視している (送信エラーとして処理できる) */
    TEST_ASSERT_INT(1, shm->pipe_ignored);
    PASS();
}

/**
 * main() 関数テスト (client_loop() のステータスで終了する)
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

/**
 * 失敗を注入して, main() を子プロセスで実行するための関数
 * (注入は, 親プロセスの test_run_child() が消費しないように, 子プロセスで行う)
 *
 * @param[in] arg 使用しない
 */
static void
run_main_failure(void *arg)
{
    /* シグナルハンドラの設定 (get 側と set 側で, 4 つのシグナル分) */
    TEST_INJECT(sigemptyset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigfillset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigaction, 0, 8, -1, EINVAL);
    /* バッファリングの設定 (標準入力と標準出力) */
    TEST_INJECT(setvbuf, 0, 2, -1, EBADF);
    run_main(arg);
}

/**
 * atexit() に失敗する main() を, 子プロセスで実行するための関数
 *
 * @param[in] arg 使用しない
 */
static void
run_main_atexit_failure(void *arg)
{
    inject_atexit.count = 1;
    inject_atexit.value = -1;
    inject_atexit.err = ENOMEM;
    run_main(arg);
}

/**
 * main() 関数テスト (システムコールなどの失敗)
 */
TEST
test_main_failure(void)
{
    /* シグナルハンドラの設定と, バッファリングの設定に失敗しても, 続行する */
    TEST_ASSERT_INT(EX_SUCCESS,
                    test_run_child(run_main_failure, NULL, NULL, NULL, 0));
    TEST_ASSERT_INT(1, shm->loop_count);

    /* atexit() に失敗すると, 終了する */
    TEST_ASSERT_INT(EX_FAILURE,
                    test_run_child(run_main_atexit_failure, NULL, NULL, NULL,
                                   0));
    PASS();
}

/* greatest の定義 (main() を含む, 実行ファイルごとに 1 か所) */
GREATEST_MAIN_DEFS();

/**
 * テストの実行
 *
 * @param[in] argc 引数の数
 * @param[in] argv 引数 (greatest のオプション. -t <名前> で 1 つのテストだけ実行できる)
 * @return 全てのテストが成功なら EXIT_SUCCESS, 失敗があれば EXIT_FAILURE
 */
int
main(int argc, char **argv)
{
    /* greatest の初期化 (オプションの解析. 標準出力のバッファリングは行わない) */
    TEST_MAIN_BEGIN();
    /* 各テストの前後に行う処理 */
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    /* テストの実行 */
    RUN_TEST(test_main_success);
    RUN_TEST(test_main_status);
    RUN_TEST(test_main_connect_failure);
    RUN_TEST(test_main_signal);
    RUN_TEST(test_main_failure);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}
