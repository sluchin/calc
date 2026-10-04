/**
 * @file  server/option.c
 * @brief オプション引数の処理
 *
 * オプション
 *  -p, --port    ポート番号指定\n
 *  -d, --digit   有効桁数設定\n
 *  -g, --debug   デバッグモード\n
 *  -h, --help    ヘルプ表示\n
 *  -V, --version バージョン情報表示\n
 *
 * @author higashi
 * @date 2010-06-25 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
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

#include <stdio.h>  /* fprintf */
#include <stdlib.h> /* EXIT_SUCCESS */
#include <getopt.h> /* getopt_long */

#include "log.h"
#include "version.h"
#include "server.h"
#include "option.h"

/* 内部変数 */
/** オプション情報構造体(ロング) */
static struct option longopts[] = {
    {"port",    required_argument, NULL, 'p'},
    {"digit",   required_argument, NULL, 'd'},
    {"debug",   no_argument,       NULL, 'g'},
    {"help",    no_argument,       NULL, 'h'},
    {"version", no_argument,       NULL, 'V'},
    {NULL,      0,                 NULL, 0  }
};

/** オプション情報文字列(ショート) */
static const char *shortopts = "p:d:hVg";

/* 内部関数 */
/** ヘルプ表示 */
static void print_help(const char *progname);
/** バージョン情報表示 */
static void print_version(const char *progname);
/** getoptエラー表示 */
static void parse_error(const int c, const char *msg);

/**
 * オプション引数
 *
 * オプション引数ごとに処理を分岐する.
 * @param[in] argc 引数の数
 * @param[in] argv コマンド引数・オプション引数
 */
void
parse_args(int argc, char *argv[])
{
    int opt = 0;         /* オプション */
    int retval = 0;      /* 戻り値 */
    long digit = 0L;     /* 桁数 */
    const int base = 10; /* 基数 */

    dbglog("start");

    /* デフォルトのポート番号を設定 */
    retval = set_port_string(DEFAULT_PORTNO);
    if (retval < 0)
        exit(EXIT_FAILURE);

    opt = getopt_long(argc, argv, shortopts, longopts, NULL);
    while (opt != EOF) {
        dbglog("opt=%c, optarg=%s", opt, optarg);
        switch (opt) {
        case 'p':
            retval = set_port_string(optarg);
            if (retval < 0) {
                fprintf(stderr, "Portno string length %u", (PORT_SIZE - 1u));
                exit(EXIT_FAILURE);
            }
            break;
        case 'd': /* 有効桁数設定 */
            digit = strtol(optarg, NULL, base);
            if ((digit <= 0L) || (MAX_DIGIT < digit)) {
                (void)fprintf(stderr, "Digits is 1-%ld.\n", MAX_DIGIT);
                exit(EXIT_FAILURE);
            }
            set_digit(digit);
            break;
        case 'g': /* デバッグモード */
            g_gflag = true;
            break;
        case 'h': /* ヘルプ表示 */
            print_help(get_progname());
            exit(EXIT_SUCCESS);
        case 'V': /* バージョン情報表示 */
            print_version(get_progname());
            exit(EXIT_SUCCESS);
        case '?':
        case ':':
            parse_error(opt, NULL);
            exit(EXIT_FAILURE);
        default:
            parse_error(opt, "internal error");
            exit(EXIT_FAILURE);
        }
        opt = getopt_long(argc, argv, shortopts, longopts, NULL);
    }
    if (optind < argc) {
        (void)printf("non-option ARGV-elements: ");
        while (optind < argc)
            (void)printf("%s ", argv[optind++]);
        (void)printf("\n");
    }
}

/**
 * ヘルプ表示
 *
 * ヘルプを表示する.
 * @param[in] progname プログラム名
 */
static void
print_help(const char *progname)
{
    (void)fprintf(stderr, "Usage: %s [OPTION]...\n", progname);
    (void)fprintf(stderr, "  -p, --port             %s%s%s",
                  "set port number or service name (default: ", DEFAULT_PORTNO, ")\n");
    (void)fprintf(stderr, "  -d, --digit            %s%ld%s", "set digit (1-", MAX_DIGIT, ")\n");
    (void)fprintf(stderr, "  -g, --debug            %s", "execute for debug mode\n");
    (void)fprintf(stderr, "  -h, --help             %s", "display this help and exit\n");
    (void)fprintf(stderr, "  -V, --version          %s", "output version information and exit\n");
}

/**
 * バージョン情報表示
 *
 * バージョン情報を表示する.
 * @param[in] progname プログラム名
 */
static void
print_version(const char *progname)
{
    (void)fprintf(stderr, "%s %s\n", progname, VERSION);
}

/**
 * getopt エラー表示
 *
 * getopt が異常な動作をした場合, エラーを表示する.
 * @param[in] c オプション引数
 * @param[in] msg メッセージ文字列
 */
static void
parse_error(const int c, const char *msg)
{
    if (msg != NULL)
        (void)fprintf(stderr, "getopt[%d]: %s\n", c, msg);
    (void)fprintf(stderr, "Try `getopt --help' for more information\n");
}
