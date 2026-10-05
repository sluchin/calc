/**
 * @file tests/lib/test_readline.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-20 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011-2018 Tetsuya Higashi. All Rights Reserved.
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

#include <unistd.h>   /* pipe fork */
#include <sys/wait.h> /* wait waitpid */
#include <errno.h>    /* errno */
#include <signal.h>   /* signal */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "memfree.h"
#include "fileio.h"
#include "readline.h"

/* 端末の入力可能なバイト数(4096)より大きいサイズに設定 */
#define BUF_SIZE 1100u /**< バッファサイズ */

DEFINE_FFF_GLOBALS

/* realloc() は, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(void *, realloc, void *, size_t)
TEST_PASSTHROUGH(void *, realloc, (void *ptr, size_t size), (ptr, size))

/* プロトタイプ */
TEST test_readline(void);
TEST test_readline_failure(void);

/* 内部変数 */
static int pfd[] = {-1, -1};         /**< パイプ */
static char test_data[BUF_SIZE];     /**< テストデータ */
static unsigned char *result = NULL; /**< 結果文字列 */

/* 内部関数 */
static unsigned char *exec_readline(char *data, size_t length);
static void set_sig_handler(void);

/**
 * 初期化処理
 */
static void
startup(void)
{
    set_sig_handler();
}

/**
 * 初期化処理
 *
 * @param[in] data 使用しない
 */
static void
setup(void *data)
{
    (void)data;
    TEST_PASSTHROUGH_RESET(realloc);
    (void)memset(test_data, 0x31, sizeof(test_data));
    test_data[sizeof(test_data) - 1u] = '\0';
    test_data[sizeof(test_data) - 2u] = '\n';
}

/**
 * 終了処理
 *
 * @param[in] data 使用しない
 */
static void
teardown(void *data)
{
    (void)data; /* 使用しない */
    close_fd(&pfd[PIPE_R], &pfd[PIPE_W], NULL);
    memfree(&result, NULL);
}

/**
 * readline() 関数テスト
 */
TEST
test_readline(void)
{
    char nolf_data[] = "test"; /* 改行なし文字列 */

    /* 正常系 */
    result = exec_readline(test_data, sizeof(test_data));

    /* 改行削除 */
    if (test_data[strlen(test_data) - 1u] == '\n')
        test_data[strlen(test_data) - 1u] = '\0';

    TEST_ASSERT_STR(test_data, (char *)result);

    memfree(&result, NULL);

    /* 改行がなく, 入力の終わりで終わる最後の行も, 返す (終端の NUL も送る) */
    result = exec_readline(nolf_data, sizeof(nolf_data));
    dbglog("result=%s", result);
    TEST_ASSERT_STR("test", (char *)result);
    memfree(&result, NULL);

    /* 終端の NUL もない場合 */
    result = exec_readline(nolf_data, strlen(nolf_data));
    TEST_ASSERT_STR("test", (char *)result);
    memfree(&result, NULL);

    /* 何も入力されない場合 (入力の終わり) */
    result = exec_readline(nolf_data, 0);
    TEST_ASSERT_NULL((char *)result);

    /* 異常系 */

    /* ファイルポインタがNULLの場合 */
    result = _readline((FILE *)NULL);
    TEST_ASSERT_NULL((char *)result);
    PASS();
}

/**
 * readline() 実行
 *
 * @param[in] data テストデータ
 * @param[in] length バイト数
 * @return 結果文字列
 */
static unsigned char *
exec_readline(char *data, size_t length)
{
    FILE *fp = NULL;  /* ファイルポインタ */
    int retval = 0;   /* 戻り値 */
    pid_t cpid = 0;   /* プロセスID */
    pid_t w = 0;      /* wait戻り値 */
    int status = 0;   /* ステイタス */
    ssize_t len = 0L; /* writen 戻り値 */

    retval = pipe(pfd);
    if (retval < 0) {
        TEST_ERROR("pipe=%d", retval);
        return NULL;
    }

    fp = fdopen(pfd[PIPE_R], "r");
    if (fp == NULL) {
        TEST_ERROR("fdopen=%p", (void *)fp);
        return NULL;
    }

    cpid = fork();
    if (cpid < 0) {
        TEST_ERROR("fork(%d)", errno);
        return NULL;
    }

    if (cpid == 0) { /* 子プロセス */
        dbglog("child");

        close_fd(&pfd[PIPE_R], NULL);

        /* 送信 */
        len = writen(pfd[PIPE_W], data, length);
        if (len < 0L) {
            outlog("writen");
            close_fd(&pfd[PIPE_W], NULL);
            exit(EXIT_FAILURE);
        }
        close_fd(&pfd[PIPE_W], NULL);
        exit(EXIT_SUCCESS);

    } else { /* 親プロセス */
        dbglog("parent: cpid=%d", (int)cpid);

        close_fd(&pfd[PIPE_W], NULL);

        /* テスト関数の実行 */
        /* 受信待ち */
        result = _readline(fp);
        dbglog("result=%s", result);

        close_fd(&pfd[PIPE_R], NULL);
        w = waitpid(-1, &status, WNOHANG);
        if (w < 0)
            TEST_NOTIFY("wait: status=%d(%d)", status, errno);
        dbglog("w=%d", (int)w);
        if (WEXITSTATUS(status) != 0) {
            TEST_NOTIFY("child error");
            return NULL;
        }
    }
    return result;
}

/**
 * シグナル設定
 */
static void
set_sig_handler(void)
{
    /* シグナル無視 */
    if (signal(SIGINT, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGINT");
    if (signal(SIGTERM, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGTERM");
    if (signal(SIGQUIT, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGQUIT");
    if (signal(SIGHUP, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGHUP");
    if (signal(SIGALRM, SIG_IGN) == SIG_ERR)
        TEST_NOTIFY("SIGALRM");
}

/**
 * readline() 関数テスト (失敗)
 */
TEST
test_readline_failure(void)
{
    int p[2] = {-1, -1}; /* パイプ */
    FILE *fp = NULL;     /* ファイルポインタ */

    /* 読み込みエラー (ディレクトリは, オープンできるが, 読み込めない) */
    fp = fopen("/tmp", "r");
    if (fp == NULL) {
        TEST_FAIL("fopen(%d)", errno);
    }
    result = _readline(fp);
    TEST_ASSERT_NULL((char *)result);
    (void)fclose(fp);

    /* メモリを確保できない */
    if (pipe(p) < 0) {
        TEST_FAIL("pipe(%d)", errno);
    }
    if (write(p[1], "abc\n", 4u) != 4) {
        TEST_FAIL("write(%d)", errno);
    }
    (void)close(p[1]);
    fp = fdopen(p[0], "r");
    if (fp == NULL) {
        TEST_FAIL("fdopen(%d)", errno);
    }
    TEST_INJECT(realloc, 0, 1, NULL, ENOMEM);
    result = _readline(fp);
    TEST_ASSERT_NULL((char *)result);
    (void)fclose(fp);
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
    /* 全てのテストの前に, 1 回だけ行う初期化 */
    startup();
    /* 各テストの前後に行う処理 */
    SET_SETUP(setup, NULL);
    SET_TEARDOWN(teardown, NULL);
    /* テストの実行 */
    RUN_TEST(test_readline);
    RUN_TEST(test_readline_failure);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}
