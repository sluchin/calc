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

#include <stdio.h>  /* snprintf */
#include <stdlib.h> /* exit EXIT_SUCCESS */
#include <signal.h> /* raise sigaction */
#include <unistd.h> /* alarm fork usleep */

#include "test_helper.h"
#include "test_process.h"
#include "def.h"
#include "log.h"
#include "option.h"
#include "calc.h"

DEFINE_FFF_GLOBALS

/* main.c が呼び出す, option.c の関数は, モックにする */
FAKE_VOID_FUNC(parse_args, int, char **)

/* main.c が呼び出す関数の失敗は, 子プロセスの中で注入する (本物を呼ぶ素通し) */
FAKE_VALUE_FUNC(int, sigaction, int, const struct sigaction *, struct sigaction *)
TEST_PASSTHROUGH(int,
                 sigaction,
                 (int signo, const struct sigaction *act, struct sigaction *oldact),
                 (signo, act, oldact))
FAKE_VALUE_FUNC(int, sigemptyset, sigset_t *)
TEST_PASSTHROUGH(int, sigemptyset, (sigset_t * set), (set))
FAKE_VALUE_FUNC(int, sigfillset, sigset_t *)
TEST_PASSTHROUGH(int, sigfillset, (sigset_t * set), (set))
FAKE_VALUE_FUNC(int, setvbuf, FILE *, char *, int, size_t)
TEST_PASSTHROUGH(int, setvbuf, (FILE * fp, char *buf, int mode, size_t size), (fp, buf, mode, size))
FAKE_VALUE_FUNC(int, fflush, FILE *)
TEST_PASSTHROUGH(int, fflush, (FILE * fp), (fp))
FAKE_VALUE_FUNC(unsigned char *, create_answer, calcinfo *, const unsigned char *)
TEST_PASSTHROUGH(unsigned char *,
                 create_answer,
                 (calcinfo * calc, const unsigned char *expr),
                 (calc, expr))

#define BUF_SIZE 1024u /**< バッファサイズ */

/** 親子プロセスで共有する情報 */
struct shared {
    int parse_count;      /**< parse_args() の呼び出し回数 */
    int parse_argc;       /**< parse_args() の argc */
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
 */
static void
fake_parse_args(int argc, char **argv)
{
    shm->parse_count++;
    shm->parse_argc = argc;
    (void)snprintf(shm->parse_argv0, sizeof(shm->parse_argv0), "%s", argv[0]);
    if (raise_signo != 0)
        (void)raise(raise_signo);
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
    TEST_PASSTHROUGH_RESET(fflush);
    TEST_PASSTHROUGH_RESET(create_answer);
    FFF_RESET_HISTORY();
    parse_args_fake.custom_fake = fake_parse_args;
    raise_signo = 0;
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
    char *argv[] = {"calcp", NULL}; /* 引数 */

    (void)arg;
    (void)calcp_main(1, argv);
}

/**
 * main() 関数テスト (計算)
 */
TEST
test_main_calc(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */
    int status = 0;           /* 終了ステータス */

    /* quit で終了 */
    status = test_run_child(run_main, NULL, "100*3\n2+3*4\nquit\n", out, sizeof(out));
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
 */
TEST
test_main_exit(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    /* exit で終了 (以降の式は実行されない) */
    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child(run_main, NULL, "100*3\nexit\n200*3\n", out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "300") != NULL, "out=%s", out);
    TEST_ASSERT_MSG(strstr(out, "600") == NULL, "out=%s", out);

    /* 空行は無視する */
    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child(run_main, NULL, "\n100*3\nquit\n", out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "300") != NULL, "out=%s", out);
    PASS();
}

/**
 * main() 関数テスト (計算できない式)
 */
TEST
test_main_error(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    /* エラーになっても, 次の式を実行する */
    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child(run_main, NULL, "1/0\n100*3\nquit\n", out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "Divide by zero") != NULL, "out=%s", out);
    TEST_ASSERT_MSG(strstr(out, "300") != NULL, "out=%s", out);
    PASS();
}

/**
 * main() 関数テスト (シグナル)
 */
TEST
test_main_signal(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    /* シグナルを受け取ると, ループを終了する */
    raise_signo = SIGINT;
    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child(run_main, NULL, "100*3\n200*3\n", out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "600") == NULL, "out=%s", out);
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
    /* シグナルハンドラの設定 (get 側と set 側で, 3 つのシグナル分) */
    TEST_INJECT(sigemptyset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigfillset, 0, 1, -1, EINVAL);
    TEST_INJECT(sigaction, 0, 6, -1, EINVAL);
    /* バッファリングの設定 (標準入力と標準出力) */
    TEST_INJECT(setvbuf, 0, 2, -1, EBADF);
    /* main_loop() の最初の fflush(NULL) */
    TEST_INJECT(fflush, 0, 1, EOF, EIO);
    /* 最初の計算 */
    TEST_INJECT(create_answer, 0, 1, NULL, ENOMEM);
    run_main(arg);
}

/**
 * 標準出力を閉じて, main() を子プロセスで実行するための関数
 *
 * @param[in] arg 使用しない
 */
static void
run_main_closed_stdout(void *arg)
{
    (void)close(STDOUT_FILENO); /* fprintf() が失敗する */
    run_main(arg);
}

/**
 * 一定時間後に SIGINT を受け取る, main() を子プロセスで実行するための関数
 *
 * @param[in] arg 使用しない
 */
static void
run_main_sigint(void *arg)
{
    pid_t ppid = getpid(); /* main() を実行するプロセス */

    (void)alarm(10u); /* 終了しなかったときの保険 */
    if (fork() == 0) {
        (void)usleep(300000u);
        (void)kill(ppid, SIGINT);
        _exit(EXIT_SUCCESS);
    }
    run_main(arg);
}

/**
 * main() 関数テスト (システムコールなどの失敗)
 */
TEST
test_main_failure(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    /* 失敗しても, 最初の計算に失敗した以外は, 続行する */
    TEST_ASSERT_INT(EXIT_SUCCESS, test_run_child(run_main_failure, NULL, "100*3\n200*3\nquit\n",
                                                 out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "600") != NULL, "out=%s", out);

    /* 標準出力に書き込めなくても, 続行する */
    TEST_ASSERT_INT(EXIT_SUCCESS, test_run_child(run_main_closed_stdout, NULL, "100*3\nquit\n", out,
                                                 sizeof(out)));
    PASS();
}

/**
 * 標準入力が端末のとき (readline) の main() を, 子プロセスで実行するための関数
 *
 * @param[in] arg 使用しない
 */
static void
run_main_tty(void *arg)
{
    (void)alarm(10u); /* 終了しなかったときの保険 */
    run_main(arg);
}

/**
 * main() 関数テスト (標準入力が端末のとき, readline を使う)
 */
TEST
test_main_readline(void)
{
    char out[BUF_SIZE * 4] = {0}; /* 出力 */

    TEST_ASSERT_INT(EXIT_SUCCESS,
                    test_run_child_pty(run_main_tty, NULL, "100*3\nquit\n", out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "300") != NULL, "out=%s", out);
    PASS();
}

/**
 * main() 関数テスト (標準入力の終わり)
 * 端末でなければ, 入力の終わり (EOF) で終了する.
 */
TEST
test_main_eof(void)
{
    char out[BUF_SIZE] = {0}; /* 出力 */

    TEST_ASSERT_INT(EXIT_SUCCESS, test_run_child(run_main_tty, NULL, "100*3\n", out, sizeof(out)));
    TEST_ASSERT_MSG(strstr(out, "300") != NULL, "out=%s", out);

    /* 何も入力しない */
    TEST_ASSERT_INT(EXIT_SUCCESS, test_run_child(run_main_tty, NULL, NULL, out, sizeof(out)));
    PASS();
}

/**
 * main() 関数テスト (履歴が上限に達する)
 */
TEST
test_main_history(void)
{
    char out[BUF_SIZE * 32] = {0}; /* 出力 (入力のエコーも含む) */
    char inbuf[1024] = {0};        /* 入力 */
    unsigned int i;

    /* テストデータごとに, 実行して, 結果を確認する */
    for (i = 0u; i < 101u; i++)
        (void)strcat(inbuf, "1+2\n");
    (void)strcat(inbuf, "quit\n");
    TEST_ASSERT_INT(EXIT_SUCCESS, test_run_child_pty(run_main_tty, NULL, inbuf, out, sizeof(out)));
    PASS();
}

/**
 * main() 関数テスト (入力待ちのときに, シグナルを受け取る)
 * 端末の入力を待つ readline は, イベントフックを呼び続けるので, SIGINT で,
 * フックが readline を終了させる.
 */
TEST
test_main_event_hook(void)
{
    TEST_ASSERT_INT(EXIT_SUCCESS, test_run_child_pty(run_main_sigint, NULL, NULL, NULL, 0));
    PASS();
}

/* greatest の定義 (main() を含む, 実行ファイルごとに 1 か所) */
GREATEST_MAIN_DEFS();

/**
 * テストの実行
 *
 * @param[in] argc 引数の数
 * @param[in] argv 引数 (greatest のオプション. -t の後にテスト名を指定すると, 1 つのテストだけ実行できる)
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
    RUN_TEST(test_main_calc);
    RUN_TEST(test_main_exit);
    RUN_TEST(test_main_error);
    RUN_TEST(test_main_signal);
    RUN_TEST(test_main_failure);
    RUN_TEST(test_main_readline);
    RUN_TEST(test_main_eof);
    RUN_TEST(test_main_history);
    RUN_TEST(test_main_event_hook);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}
