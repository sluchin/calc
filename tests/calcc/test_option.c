/**
 * @file tests/calcc/test_option.c
 * @brief 単体テスト (option.c)
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
#include <stdlib.h> /* EXIT_SUCCESS */
#include <getopt.h> /* optind */

#include "test_helper.h"
#include "test_process.h"
#include "def.h"
#include "log.h"
#include "version.h"
#include "client.h"
#include "option.h"

DEFINE_FFF_GLOBALS

/* getopt_long() は, モックにして, 通常は本物を呼ぶ (想定外の値を返させる) */
FAKE_VALUE_FUNC(int, getopt_long, int, char *const *, const char *, const struct option *, int *)
TEST_PASSTHROUGH(int,
                 getopt_long,
                 (int argc,
                  char *const *argv,
                  const char *shortopts,
                  const struct option *longopts,
                  int *longindex),
                 (argc, argv, shortopts, longopts, longindex))

/* option.c が呼び出す client.c の関数は, モックにする */
FAKE_VALUE_FUNC(int, set_port_string, const char *)
FAKE_VALUE_FUNC(int, set_host_string, const char *)

#define BUF_SIZE 1024u /**< バッファサイズ */

/** parse_args() の引数 */
struct args {
    int argc;    /**< 引数の数 */
    char **argv; /**< 引数 */
};

/**
 * 初期化処理
 */
static void
startup(void)
{
    set_progname("testprog");
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
    RESET_FAKE(set_port_string);
    RESET_FAKE(set_host_string);
    g_gflag = false;
    g_tflag = false;
    TEST_PASSTHROUGH_RESET(getopt_long);
    FFF_RESET_HISTORY();
    optind = 0; /* getopt の状態を初期化 */
}

/**
 * parse_args() を子プロセスで実行するための関数
 *
 * @param[in] arg args構造体
 */
static void
run_parse_args(void *arg)
{
    struct args *a = (struct args *)arg; /* 引数 */

    optind = 0; /* 親プロセスで実行した getopt の状態を初期化 */
    parse_args(a->argc, a->argv);
}

/**
 * parse_args() を子プロセスで実行する
 *
 * @param[out] out 標準出力と標準エラー出力
 * @param[in] size 出力バッファのサイズ
 * @param[in] argc 引数の数
 * @param[in] argv 引数
 * @return 終了ステータス
 */
static int
exec_parse_args(char *out, size_t size, int argc, char **argv)
{
    struct args a = {argc, argv}; /* 引数 */

    return test_run_child(run_parse_args, &a, NULL, out, size);
}

/**
 * parse_args() 関数テスト (オプション以外の引数)
 */
TEST
test_parse_args_non_option(void)
{
    char out[BUF_SIZE] = {0};                        /* 出力 */
    char *argv[] = {"testprog", "abc", "def", NULL}; /* 引数 (abc def) */

    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 3, argv));
    TEST_ASSERT_STR("non-option ARGV-elements: abc def \n", out);
    PASS();
}

/**
 * print_help() 関数テスト (-h, --help)
 */
TEST
test_print_help(void)
{
    char out[BUF_SIZE] = {0};                     /* 出力 */
    char *argv1[] = {"testprog", "-h", NULL};     /* 引数 (-h) */
    char *argv2[] = {"testprog", "--help", NULL}; /* 引数 (--help) */

    /* 準備をして, 関数を実行し, 結果を確認する */
    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 2, argv1));
    TEST_ASSERT_MATCH("Usage: testprog \\[OPTION\\]", out);
    TEST_ASSERT_MATCH("-h, --help", out);
    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 2, argv2));
    TEST_ASSERT_MATCH("Usage: testprog", out);
    PASS();
}

/**
 * print_version() 関数テスト (-V, --version)
 */
TEST
test_print_version(void)
{
    char out[BUF_SIZE] = {0};                        /* 出力 */
    char *argv1[] = {"testprog", "-V", NULL};        /* 引数 (-V) */
    char *argv2[] = {"testprog", "--version", NULL}; /* 引数 (--version) */

    /* 準備をして, 関数を実行し, 結果を確認する */
    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 2, argv1));
    TEST_ASSERT_STR("testprog " VERSION "\n", out);
    TEST_ASSERT_INT(EXIT_SUCCESS, exec_parse_args(out, sizeof(out), 2, argv2));
    TEST_ASSERT_STR("testprog " VERSION "\n", out);
    PASS();
}

/**
 * parse_error() 関数テスト (不正なオプション)
 */
TEST
test_parse_error(void)
{
    char out[BUF_SIZE] = {0};                /* 出力 */
    char *argv[] = {"testprog", "-x", NULL}; /* 引数 (-x) */

    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 2, argv));
    TEST_ASSERT_MATCH("Try `getopt --help' for more information", out);
    PASS();
}

/**
 * parse_args() 関数テスト (-i, --ipaddress)
 */
TEST
test_parse_args_ipaddress(void)
{
    char *argv0[] = {"testprog", NULL};                          /* 引数 */
    char *argv1[] = {"testprog", "-i", "192.168.0.1", NULL};     /* 引数 (-i 192.168.0.1) */
    char *argv2[] = {"testprog", "--ipaddress=localhost", NULL}; /* 引数 (--ipaddress=localhost) */

    /* 指定しない場合は, デフォルトのIPアドレス */
    parse_args(1, argv0);
    TEST_ASSERT_INT(1, set_host_string_fake.call_count);
    TEST_ASSERT_STR(DEFAULT_IPADDR, set_host_string_fake.arg0_history[0]);

    /* ショートオプション */
    optind = 0;
    parse_args(3, argv1);
    TEST_ASSERT_INT(3, set_host_string_fake.call_count);
    TEST_ASSERT_STR("192.168.0.1", set_host_string_fake.arg0_history[2]);

    /* ロングオプション */
    optind = 0;
    parse_args(2, argv2);
    TEST_ASSERT_INT(5, set_host_string_fake.call_count);
    TEST_ASSERT_STR("localhost", set_host_string_fake.arg0_history[4]);
    PASS();
}

/**
 * parse_args() 関数テスト (-i, IPアドレスの設定に失敗)
 */
TEST
test_parse_args_ipaddress_failure(void)
{
    char out[BUF_SIZE] = {0};                     /* 出力 */
    char *argv[] = {"testprog", "-i", "x", NULL}; /* 引数 (-i x) */
    int seq[] = {0, -1};                          /* デフォルトは成功, -i の指定は失敗 */

    SET_RETURN_SEQ(set_host_string, seq, 2);
    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 3, argv));
    TEST_ASSERT_MATCH("Hostname string length [0-9]+", out);

    /* デフォルトのIPアドレスの設定に失敗 */
    RESET_FAKE(set_host_string);
    set_host_string_fake.return_val = -1;
    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 1, argv));

    /* デフォルトのポート番号の設定に失敗 */
    RESET_FAKE(set_host_string);
    set_port_string_fake.return_val = -1;
    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 1, argv));
    PASS();
}

/**
 * parse_args() 関数テスト (-p, --port)
 */
TEST
test_parse_args_port(void)
{
    char *argv0[] = {"testprog", NULL};                  /* 引数 */
    char *argv1[] = {"testprog", "-p", "8080", NULL};    /* 引数 (-p 8080) */
    char *argv2[] = {"testprog", "--port=http", NULL};   /* 引数 (--port=http) */
    char out[BUF_SIZE] = {0};                            /* 出力 */
    char *argv3[] = {"testprog", "-p", "1234567", NULL}; /* 引数 (-p 1234567) */
    int seq[] = {0, -1};                                 /* デフォルトは成功, -p の指定は失敗 */

    /* 指定しない場合は, デフォルトのポート番号 */
    parse_args(1, argv0);
    TEST_ASSERT_INT(1, set_port_string_fake.call_count);
    TEST_ASSERT_STR(DEFAULT_PORTNO, set_port_string_fake.arg0_history[0]);

    /* ショートオプション */
    optind = 0;
    parse_args(3, argv1);
    TEST_ASSERT_INT(3, set_port_string_fake.call_count);
    TEST_ASSERT_STR("8080", set_port_string_fake.arg0_history[2]);

    /* ロングオプション */
    optind = 0;
    parse_args(2, argv2);
    TEST_ASSERT_INT(5, set_port_string_fake.call_count);
    TEST_ASSERT_STR("http", set_port_string_fake.arg0_history[4]);

    /* 設定に失敗 (戻り値の列は, 呼び出し回数を基準にするので, 初期化する) */
    RESET_FAKE(set_port_string);
    SET_RETURN_SEQ(set_port_string, seq, 2);
    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 3, argv3));
    TEST_ASSERT_MATCH("Portno string length [0-9]+", out);
    PASS();
}

/**
 * parse_args() 関数テスト (-t, --time)
 */
TEST
test_parse_args_time(void)
{
    char *argv1[] = {"testprog", "-t", NULL};     /* 引数 (-t) */
    char *argv2[] = {"testprog", "--time", NULL}; /* 引数 (--time) */

    /* 準備をして, 関数を実行し, 結果を確認する */
    ASSERT_FALSE(g_tflag);
    parse_args(2, argv1);
    ASSERT(g_tflag);

    g_tflag = false;
    optind = 0;
    parse_args(2, argv2);
    ASSERT(g_tflag);
    PASS();
}

/**
 * parse_args() 関数テスト (-g, --debug)
 */
TEST
test_parse_args_debug(void)
{
    char *argv1[] = {"testprog", "-g", NULL};      /* 引数 (-g) */
    char *argv2[] = {"testprog", "--debug", NULL}; /* 引数 (--debug) */

    /* 準備をして, 関数を実行し, 結果を確認する */
    ASSERT_FALSE(g_gflag);
    parse_args(2, argv1);
    ASSERT(g_gflag);

    g_gflag = false;
    optind = 0;
    parse_args(2, argv2);
    ASSERT(g_gflag);
    PASS();
}

/**
 * parse_args() 関数テスト (getopt_long() が想定外の値を返す)
 */
TEST
test_parse_args_internal_error(void)
{
    char out[BUF_SIZE] = {0};          /* 出力 */
    char *argv[] = {"testprog", NULL}; /* 引数 */

    TEST_INJECT(getopt_long, 0, 1, 'z', 0);
    TEST_ASSERT_INT(EXIT_FAILURE, exec_parse_args(out, sizeof(out), 1, argv));
    TEST_ASSERT_MATCH("getopt\\[122\\]: internal error", out);
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
    /* テストの実行 */
    RUN_TEST(test_parse_args_ipaddress);
    RUN_TEST(test_parse_args_ipaddress_failure);
    RUN_TEST(test_parse_args_port);
    RUN_TEST(test_parse_args_time);
    RUN_TEST(test_parse_args_debug);
    RUN_TEST(test_parse_args_non_option);
    RUN_TEST(test_print_help);
    RUN_TEST(test_print_version);
    RUN_TEST(test_parse_error);
    RUN_TEST(test_parse_args_internal_error);
    /* 結果の表示と終了 */
    TEST_MAIN_END();
}
