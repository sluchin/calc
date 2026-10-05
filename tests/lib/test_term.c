/**
 * @file tests/lib/test_term.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-01-06 higashi 新規作成
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

#include <unistd.h>  /* STDIN_FILENO */
#include <termios.h> /* termios */
#include <errno.h>   /* errno */

#include "test_helper.h"

#include "def.h"
#include "log.h"
#include "fileio.h"
#include "term.h"

#define BUF_SIZE 2048u /**< バッファサイズ */

DEFINE_FFF_GLOBALS

/* 端末が無くても実行できるように, tcgetattr() は FFF でモックにする */
FAKE_VALUE_FUNC(int, tcgetattr, int, struct termios *)

/* strdup() は, モックにして, 通常は本物を呼ぶ (失敗を注入する) */
FAKE_VALUE_FUNC(char *, strdup, const char *)
TEST_PASSTHROUGH(char *, strdup, (const char *str), (str))

/* プロトタイプ */
TEST test_sys_print_termattr(void);
TEST test_get_termattr(void);
TEST test_mode_type_flag(void);
TEST test_sys_print_termattr_failure(void);
TEST test_get_termattr_failure(void);
TEST test_sys_print_termattr_fd(void);

/* 内部変数 */
static testterm term; /**< 関数構造体 */
static int fd = -1;   /**< ファイルディスクリプタ */

/**
 * 初期化処理
 */
static void
startup(void)
{
    (void)memset(&term, 0, sizeof(testterm));
    test_init_term(&term);
}

/**
 * tcgetattr() のモック動作 (8bit, 受信有効の端末状態を返す)
 *
 * @param[in] fdesc ファイルディスクリプタ
 * @param[out] mode termios構造体
 * @return 0
 */
static int
fake_tcgetattr(int fdesc, struct termios *mode)
{
    (void)fdesc;
    mode->c_cflag = CS8 | CREAD | CLOCAL;
    mode->c_iflag = ICRNL | IXON;
    mode->c_oflag = OPOST | ONLCR;
    mode->c_lflag = ISIG | ICANON | ECHO;
    return 0;
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
    RESET_FAKE(tcgetattr);
    FFF_RESET_HISTORY();
    tcgetattr_fake.custom_fake = fake_tcgetattr;
    TEST_PASSTHROUGH_RESET(strdup);
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
    if (fd != -1) {
        if (close(fd) < 0)
            TEST_NOTIFY("close: fd=%d(%d)", fd, errno);
        fd = -1;
    }
}

/**
 * print_termattr() 関数テスト
 */
TEST
test_sys_print_termattr(void)
{
    ssize_t rlen = 0L;           /* 戻り値 */
    char actual[BUF_SIZE] = {0}; /* 実際の文字列 */
    const char expected[] =      /* 期待する文字列 */
        "programname\\[[0-9]+\\]: filename\\[15\\]: function: "
        "tcgetattr\\(.*\\)";

    /* 正常系 */
    fd = pipe_fd(STDERR_FILENO);
    if (fd < 0) {
        TEST_FAIL("pipe_fd(%d)", errno);
    }

    sys_print_termattr(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function",
                       STDIN_FILENO);

    rlen = read(fd, actual, sizeof(actual));
    if (rlen < 0L) {
        TEST_FAIL("read: fd=%d(%d)", fd, errno);
    }

    TEST_ASSERT_MATCH_MSG(expected, actual, "expected=%s actual=%s", expected, actual);
    PASS();
}

/**
 * get_termattr() 関数テスト
 */
TEST
test_get_termattr(void)
{
    char *ptr = NULL;    /* テスト関数戻り値 */
    struct termios mode; /* termios構造体 */

    (void)memset(&mode, 0, sizeof(struct termios));
    ptr = term.get_termattr(STDIN_FILENO, &mode);

    TEST_ASSERT_MATCH("tcgetattr(.*)", ptr);
    /* tcgetattr() が, 指定したファイルディスクリプタで 1 回呼ばれた */
    ASSERT_EQ(1, (int)tcgetattr_fake.call_count);
    ASSERT_EQ(STDIN_FILENO, tcgetattr_fake.arg0_val);
    ASSERT_EQ(&mode, tcgetattr_fake.arg1_val);

    if (ptr != NULL)
        free(ptr);
    ptr = NULL;
    PASS();
}

/**
 * mode_type_flag() 関数テスト
 */
TEST
test_mode_type_flag(void)
{
    tcflag_t *flag = NULL; /* テスト関数戻り値 */
    struct termios mode;   /* termios構造体 */
    int retval = 0;        /* 戻り値 */

    (void)memset(&mode, 0, sizeof(struct termios));
    retval = tcgetattr(STDIN_FILENO, &mode);
    if (retval < 0) {
        TEST_FAIL("tcgetattr: mode=%p(%d)", (void *)&mode, errno);
    }

    /* 正常系 */
    flag = term.mode_type_flag(control, &mode);
    TEST_ASSERT_NOT_NULL(flag);
    flag = term.mode_type_flag(input, &mode);
    TEST_ASSERT_NOT_NULL(flag);
    flag = term.mode_type_flag(output, &mode);
    TEST_ASSERT_NOT_NULL(flag);
    flag = term.mode_type_flag(local, &mode);
    TEST_ASSERT_NOT_NULL(flag);
    /* 異常系 */
    flag = term.mode_type_flag((enum mode_type)4, &mode);
    TEST_ASSERT_NULL(flag);
    PASS();
}

/**
 * sys_print_termattr() 関数テスト (失敗)
 */
TEST
test_sys_print_termattr_failure(void)
{
    /* 端末の情報を取得できないときは, 何も出力しない */
    tcgetattr_fake.custom_fake = NULL;
    tcgetattr_fake.return_val = -1;
    sys_print_termattr(LOG_INFO, LOG_PID | LOG_PERROR, "programname", "filename", 15, "function",
                       STDIN_FILENO);
    TEST_ASSERT_INT(1, tcgetattr_fake.call_count);
    PASS();
}

/**
 * get_termattr() 関数テスト (失敗)
 */
TEST
test_get_termattr_failure(void)
{
    struct termios mode; /* termios構造体 */

    (void)memset(&mode, 0, sizeof(struct termios));

    /* 不正なファイルディスクリプタ */
    TEST_ASSERT_NULL(term.get_termattr(-1, &mode));
    TEST_ASSERT_INT(0, tcgetattr_fake.call_count);

    /* tcgetattr() に失敗 */
    tcgetattr_fake.custom_fake = NULL;
    tcgetattr_fake.return_val = -1;
    TEST_ASSERT_NULL(term.get_termattr(STDIN_FILENO, &mode));
    TEST_ASSERT_INT(1, tcgetattr_fake.call_count);

    /* strdup() に失敗 */
    tcgetattr_fake.custom_fake = fake_tcgetattr;
    TEST_INJECT(strdup, 0, 1, NULL, ENOMEM);
    TEST_ASSERT_NULL(term.get_termattr(STDIN_FILENO, &mode));
    PASS();
}

/**
 * sys_print_termattr() 関数テスト (指定したファイルディスクリプタの端末情報を取得する)
 */
TEST
test_sys_print_termattr_fd(void)
{
    const int termfd = 7; /* 標準入力以外のファイルディスクリプタ */

    sys_print_termattr(LOG_INFO, LOG_PID, "programname", "filename", 15, "function", termfd);
    TEST_ASSERT_INT(1, tcgetattr_fake.call_count);
    TEST_ASSERT_INT(termfd, tcgetattr_fake.arg0_val);
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
    RUN_TEST(test_sys_print_termattr);
    RUN_TEST(test_get_termattr);
    RUN_TEST(test_mode_type_flag);
    RUN_TEST(test_sys_print_termattr_failure);
    RUN_TEST(test_get_termattr_failure);
    RUN_TEST(test_sys_print_termattr_fd);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}
