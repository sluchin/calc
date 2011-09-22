/**
 * @file  calc/option.c
 * @brief オプション引数の処理
 *
 * オプション
 *  -p 小数点以下有効桁数の設定\n
 *  -h ヘルプの表示\n
 *  -V バージョンの表示\n
 *
 * @author higashi
 * @date 2010-06-27 higashi 新規作成
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

#include <stdio.h>  /* fputs fprintf */
#include <string.h> /* memset */
#include <stdlib.h> /* EXIT_SUCCESS */
#include <getopt.h> /* getopt_long */
#include <libgen.h> /* basename */

#include "log.h"
#include "version.h"
#include "calc.h"
#include "option.h"

/* 内部変数 */
/** オプション情報構造体(ロング) */
static struct option longopts[] = {
    { "digit",     required_argument, NULL, 'd' },
    { "time",      no_argument,       NULL, 't' },
    { "help",      no_argument,       NULL, 'h' },
    { "version",   no_argument,       NULL, 'V' },
    { NULL,        0,                 NULL, 0   }
};

/** オプション情報文字列(ショート) */
static const char *shortopts = "hVtd:";

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
    (void)fprintf(fp, "  -d, --digit            %s%d%s",
                  "set digit(1-", MAX_DIGIT, ")\n");
    (void)fprintf(fp, "  -t, --time             %s",
                  "time test\n");
    (void)fprintf(fp, "  -h, --help             %s",
                  "display this help and exit\n");
    (void)fprintf(fp, "  -V, --version          %s",
                  "output version information and exit\n");
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
    FILE *fp = stderr;   /* 標準エラー出力 */
    int opt = 0;         /* getopt_longの戻り値格納 */
    const int base = 10; /* 基数 */

    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != EOF) {
        dbglog("opt=%c, optarg=%s", opt, optarg);
        switch (opt) {
        case 'd': /* 有効桁数設定 */
            digit = strtol(optarg, NULL, base);
            if (digit <= 0 || MAX_DIGIT < digit) {
                (void)fprintf(fp, "Maximum digit is %d.\n", MAX_DIGIT);
                exit(EXIT_FAILURE);
            }
            break;
        case 't':
            tflag = true;
            break;
        case 'h': /* ヘルプ表示 */
            print_help(basename(argv[0]));
            exit(EXIT_SUCCESS);
        case 'V': /* バージョン情報表示 */
            print_version(basename(argv[0]));
            exit(EXIT_SUCCESS);
        case '?':
        case ':':
            (void)fprintf(fp, "getopt[%c]\n", opt);
            parse_error(NULL);
            exit(EXIT_FAILURE);
        default:
            (void)fprintf(fp, "getopt[%c]\n", opt);
            parse_error("internal error");
            exit(EXIT_FAILURE);
        }
    }
}

