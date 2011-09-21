/**
 * @file  server/option.c
 * @brief オプション引数の処理
 *
 * オプション
 *  -p, --port     ポート番号指定\n
 *  -h, --debug    ヘルプの表示\n
 *  -V, --help     バージョンの表示\n
 *  -g, --version  デバッグ用\n
 *
 * @author higashi
 * @date 2010-06-25 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010 Tetsuya Higashi. All Rights Reserved.
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

#include <stdio.h>   /* fputs fprintf */
#include <string.h>  /* memset */
#include <stdlib.h>  /* EXIT_SUCCESS */
#include <getopt.h>  /* getopt_long */
#include <libgen.h>  /* basename */

#include "log.h"
#include "version.h"
#include "option.h"

/* 外部変数 */
bool gflag = false;            /**< gオプションフラグ */
char port_no[PORT_SIZE] = {0}; /**< ポート番号またはサービス名 */

/* 内部変数 */
/** オプション情報構造体(ロング) */
static struct option longopts[] = {
    { "port", required_argument, NULL, 'p' },
    { "debug", no_argument, NULL, 'g' },
    { "help", no_argument, NULL, 'h' },
    { "version", no_argument, NULL, 'V' },
    { NULL, 0, NULL, 0 }
};

/** オプション情報文字列(ショート) */
static const char *shortopts = "p:hVg";

/* 内部関数 */
/** ヘルプの表示 */
static void print_help(const char *prog_name);
/** バージョン情報表示 */
static void print_version(const char *prog_name);
/** getoptエラー表示 */
static void parse_error(const char *msg);

/**
 * ヘルプ表示
 *
 * ヘルプを表示する.
 * @param[in] prog_name プログラム名
 * @return なし
 */
static void
print_help(const char *prog_name)
{
    FILE *fp = stderr;
    (void)fprintf(fp, "Usage: %s [OPTION]...\n", progname);
    (void)fputs("  -p, --port             port\n", fp);
    (void)fputs("  -g, --debug            execute test mode\n", fp);
    (void)fputs("  -h, --help             display this help and exit\n", fp);
    (void)fputs("  -V, --version          output version information and exit\n", fp);
}

/**
 * バージョン情報表示
 *
 * バージョン情報を表示する.
 * @param[in] prog_name プログラム名
 * @return なし
 */
static void
print_version(const char *prog_name)
{
    FILE *fp = stderr;
    (void)fprintf(fp, "%s %s\n", prog_name, VERSION);
}

/**
 * getoptエラー表示
 *
 * getoptが異常な動作をした場合、エラーを表示する.
 * @param[in] msg メッセージ文字列
 * @return なし
 */
static void
parse_error(const char *msg)
{
    FILE *fp = stderr;
    if (msg)
        (void)fprintf(fp, "getopt: %s\n", msg);
    (void)fprintf(fp, "Try `getopt --help' for more information\n");
}

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
    FILE *fp = stderr; /* 標準エラー出力 */
    int opt = 0;       /* getopt_longの戻り値格納 */

    dbglog("start");

    /* デフォルトのポート番号を設定 */
    (void)memset(port_no, 0, sizeof(port_no));
    (void)strncpy(port_no, DEFAULT_PORTNO, sizeof(port_no));

    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != EOF) {
        dbglog("opt=%c, optarg=%s", opt, optarg);
        switch (opt) {
        case 'p':
            if (!optarg) {
                outlog("opt=%c, optarg=%s", opt, optarg);
                exit(EXIT_FAILURE);
            }
            if (strlen(optarg) < sizeof(port_no)) {
                (void)memset(port_no, 0, sizeof(port_no));
                (void)strncpy(port_no, optarg, sizeof(port_no));
            } else {
                (void)fprintf(fp, "port_no=%s\n", optarg);
                print_help(basename(argv[0]));
                exit(EXIT_FAILURE);
            }
            break;
        case 'h':
            print_help(basename(argv[0]));
            exit(EXIT_SUCCESS);
        case 'V':
            print_version(basename(argv[0]));
            exit(EXIT_SUCCESS);
        case 'g':
            gflag = true;
            break;
        case '?':
        case ':':
            (void)fprintf(fp, "getopt=%c\n", opt);
            parse_error(NULL);
            exit(EXIT_FAILURE);
        default:
            (void)fprintf(fp, "getopt=%c\n", opt);
            parse_error("internal error");
            exit(EXIT_FAILURE);
        }
    }
}

