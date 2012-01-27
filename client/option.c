/**
 * @file  client/option.c
 * @brief オプション引数の処理
 *
 * オプション
 *  -i, --ipaddress  IPアドレス指定\n
 *  -p, --port       ポート番号指定\n
 *  -t, --time       処理時間計測\n
 *  -g, --debug      デバッグモード\n
 *  -h, --help       ヘルプ表示\n
 *  -V, --version    バージョン情報表示\n
 *
 * @author higashi
 * @date 2010-06-23 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
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
#include <stdlib.h> /* EXIT_SUCCESS */
#include <getopt.h> /* getopt_long */

#include "log.h"
#include "version.h"
#include "client.h"
#include "option.h"

/* 内部変数 */
/** オプション情報構造体(ロング) */
static struct option longopts[] = {
    { "ipaddress", required_argument, NULL, 'i' },
    { "port",      required_argument, NULL, 'p' },
    { "time",      no_argument,       NULL, 't' },
    { "debug",     no_argument,       NULL, 'g' },
    { "help",      no_argument,       NULL, 'h' },
    { "version",   no_argument,       NULL, 'V' },
    { NULL,        0,                 NULL, 0   }
};

/** オプション情報文字列(ショート) */
static const char *shortopts = "p:i:thVg";

/* 内部関数 */
/** ヘルプの表示 */
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
 * @return なし
 */
void
parse_args(int argc, char *argv[])
{
    int opt = 0; /* オプション */

    dbglog("start");

    /* デフォルトのポート番号を設定 */
    if (set_port_string(DEFAULT_PORTNO) < 0)
        exit(EXIT_FAILURE);

    /* デフォルトのIPアドレスを設定 */
    if (set_host_string(DEFAULT_IPADDR) < 0)
        exit(EXIT_FAILURE);

    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != EOF) {
        dbglog("opt=%c, optarg=%s", opt, optarg);
        switch (opt) {
        case 'i': /* IPアドレス指定 */
            if (set_host_string(optarg) < 0) {
                fprintf(stderr, "Hostname string length %d", (HOST_SIZE - 1));
                exit(EXIT_FAILURE);
            }
            break;
        case 'p': /* ポート番号指定 */
            if (set_port_string(optarg) < 0) {
                fprintf(stderr, "Portno string length %d", (PORT_SIZE - 1));
                exit(EXIT_FAILURE);
            }
            break;
        case 't': /* 処理時間計測 */
            g_tflag = true;
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
    }
    if (optind < argc) {
        printf("non-option ARGV-elements: ");
        while (optind < argc)
            printf("%s ", argv[optind++]);
        printf("\n");
    }
}

/**
 * ヘルプ表示
 *
 * ヘルプを表示する.
 * @param[in] progname プログラム名
 * @return なし
 */
static void
print_help(const char *progname)
{
    (void)fprintf(stderr, "Usage: %s [OPTION]...\n", progname);
    (void)fprintf(stderr, "  -i, --ipaddress        %s%s%s",
                  "set ip address or host name (default: ",
                  DEFAULT_IPADDR, ")\n");
    (void)fprintf(stderr, "  -p, --port             %s%s%s",
                  "set port number or service name (default: ",
                  DEFAULT_PORTNO, ")\n");
    (void)fprintf(stderr, "  -g, --debug            %s",
                  "execute for debug mode\n");
    (void)fprintf(stderr, "  -t, --time             %s",
                  "print time\n");
    (void)fprintf(stderr, "  -h, --help             %s",
                  "display this help and exit\n");
    (void)fprintf(stderr, "  -V, --version          %s",
                  "output version information and exit\n");
}

/**
 * バージョン情報表示
 *
 * バージョン情報を表示する.
 * @param[in] progname プログラム名
 * @return なし
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
 * @return なし
 */
static void
parse_error(const int c, const char *msg)
{
    if (msg)
        (void)fprintf(stderr, "getopt[%d]: %s\n", c, msg);
    (void)fprintf(stderr, "Try `getopt --help' for more information\n");
}

