/**
 * @file tests/lib/test_data.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-19 higashi 新規作成
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

#include "test_helper.h"

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
TEST test_set_client_data(void);
/** set_server_data() 関数テスト */
TEST test_set_server_data(void);
/** set_client_data() 関数テスト (失敗) */
TEST test_set_client_data_failure(void);
/** set_server_data() 関数テスト (失敗) */
TEST test_set_server_data_failure(void);

/**
 * 初期化処理
 */
static void
startup(void)
{
    dbglog("char=%zu", ALIGNOF(char));
    dbglog("short=%zu", ALIGNOF(short));
    dbglog("int=%zu", ALIGNOF(int));
    dbglog("long=%zu", ALIGNOF(long));
    dbglog("double=%zu", ALIGNOF(double));
}

/**
 * set_client_data() 関数テスト
 */
TEST
test_set_client_data(void)
{
    size_t length = 0; /* データ長 */
    ssize_t len = 0; /* 送信データ長 */
    /* テストデータごとに, 実行して, 結果を確認する */
    struct client_data *dt = NULL; /* 送受信データ構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(test_data); i++) {
        length = strlen(test_data[i]) + 1;
        dbglog("length=%zu", length);
        len = set_client_data(&dt, (unsigned char *)test_data[i], length);
        dbglog("len=%zd, %s", len, test_data[i]);
        TEST_ASSERT_INT(0, len % ALIGN);
        dbglog("dt=%p", dt);
        TEST_ASSERT_NOT_NULL(dt);
        if (dt)
            free(dt);
        dt = NULL;
    }
    PASS();
}

/**
 * set_server_data() 関数テスト
 */
TEST
test_set_server_data(void)
{
    size_t length = 0; /* データ長 */
    ssize_t len = 0; /* 送信データ長 */
    /* テストデータごとに, 実行して, 結果を確認する */
    struct server_data *dt = NULL; /* 送受信データ構造体 */

    unsigned int i;
    for (i = 0; i < NELEMS(test_data); i++) {
        length = strlen(test_data[i]) + 1;
        dbglog("length=%zu", length);
        len = set_server_data(&dt, (unsigned char *)test_data[i], length);
        dbglog("len=%zd, %s", len, test_data[i]);
        TEST_ASSERT_INT(0, len % ALIGN);
        dbglog("dt=%p", dt);
        TEST_ASSERT_NOT_NULL(dt);
        if (dt)
            free(dt);
        dt = NULL;
    }
    PASS();
}


/**
 * set_client_data() 関数テスト (失敗)
 */
TEST
test_set_client_data_failure(void)
{
    struct client_data *dt = NULL; /* 送受信データ構造体 */
    unsigned char buf[] = "a";     /* 送受信バッファ */

    /* バッファがNULL */
    TEST_ASSERT_INT(EX_NG, set_client_data(&dt, NULL, 1));
    TEST_ASSERT_NULL(dt);

    /* メモリを確保できない (巨大なサイズ) */
    TEST_ASSERT_INT(EX_NG, set_client_data(&dt, buf, (size_t)-1 / 2));
    TEST_ASSERT_NULL(dt);
    PASS();
}

/**
 * set_server_data() 関数テスト (失敗)
 */
TEST
test_set_server_data_failure(void)
{
    struct server_data *dt = NULL; /* 送受信データ構造体 */
    unsigned char buf[] = "a";     /* 送受信バッファ */

    /* バッファがNULL */
    TEST_ASSERT_INT(EX_NG, set_server_data(&dt, NULL, 1));
    TEST_ASSERT_NULL(dt);

    /* メモリを確保できない (巨大なサイズ) */
    TEST_ASSERT_INT(EX_NG, set_server_data(&dt, buf, (size_t)-1 / 2));
    TEST_ASSERT_NULL(dt);
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
    /* テストの実行 */
    RUN_TEST(test_set_client_data);
    RUN_TEST(test_set_server_data);
    RUN_TEST(test_set_client_data_failure);
    RUN_TEST(test_set_server_data_failure);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}
