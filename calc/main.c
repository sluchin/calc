/**
 * @file  calc/main.c
 * @brief main関数
 *
 * @author higashi
 * @date 2009-06-27 higashi 新規作成
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

#include <stdio.h>   /* FILE */
#include <stdlib.h>  /* exit EXIT_SUCCESS free */
#include <stdbool.h> /* true */
#include <string.h>  /* memset */
#include <unistd.h>  /* close */
#include <libgen.h>  /* basename */
#include <signal.h>  /* sigaction */

#include "common.h"
#include "data.h"
#include "option.h"
#include "log.h"
#include "calc.h"

/* 外部変数 */
char *progname; /**< プログラム名 */
/* 内部変数 */
static uchar *expr = NULL;   /* 式 */
static uchar *result = NULL; /* メッセージ */
static uchar *tmp = NULL;    /* 一時アドレス */

/* メモリ解放 */
#define FREEALL                                         \
    if (result) { free(result); } result = NULL;        \
    if (expr) { free(expr); } expr = NULL;              \


/* 内部関数 */
/** シグナルハンドラ */
static void sig_handler(int signo);

/** 
 * main関数
 *
 * @param[in] argc 引数の数
 * @param[in] argv コマンド引数・オプション引数
 * @return 常にEXIT_SUCCESS
 */
int main(int argc, char *argv[])
{
    struct sigaction sa;   /* シグナル */
    int retval = 0;        /* 戻り値 */
    char *fgetsval = NULL; /* fgetsの戻り値 */
    uchar buf[BUF_SIZE];   /* バッファ */
    size_t length;         /* 文字列長 */
    size_t total;          /* 文字列長全て */

    dbglog("start");

    progname = basename(argv[0]);

    /* シグナルマスクの設定 */
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) < 0)
        outlog("sigemptyset[%p]", sa);  
    if (sigaddset(&sa.sa_mask, SIGINT) < 0)
        outlog("sigaddset[%p]; SIGINT", sa);  
    if (sigaddset(&sa.sa_mask, SIGTERM) < 0)
        outlog("sigaddset[%p]; SIGTERM", sa);  
    if (sigaddset(&sa.sa_mask, SIGQUIT) < 0)
        outlog("sigaddset[%p]; SIGQUIT", sa);  

    /* シグナル補足 */
    if (sigaction(SIGINT, &sa, NULL) < 0)
        outlog("sigaction[%p]; SIGINT", sa);  
    if (sigaction(SIGTERM, &sa, NULL) < 0)
        outlog("sigaction[%p]; SIGTERM", sa);  
    if (sigaction(SIGQUIT, &sa, NULL) < 0)
        outlog("sigaction[%p]; SIGQUIT", sa);  

    /* オプション引数 */
    parse_args(argc, argv);

    while (true) {
        total = 0;
        while (true) {
            length = 0;
            (void)memset(buf, 0, sizeof(buf));
            fgetsval = fgets((char *)buf, sizeof(buf), stdin);
            if (feof(stdin) || ferror(stdin)) { /* エラー */
                outlog("fgets[%p]", retval);
                FREEALL;
                clearerr(stdin);
                break;
            }
            length = strlen((char *)buf);
            dbgdump((uchar *)buf, length, "buf[%u]", length);

            tmp = (uchar *)realloc((uchar *)expr,
                                   (total + length + 1) * sizeof(uchar));
            if (!tmp) {
                outlog("realloc[%p]: %u", expr, total + length + 1);
                FREEALL;
                break;
            }
            expr = tmp;

            (void)memcpy((uchar *)expr + total, buf, length + 1);

            total += length;
            dbglog("length[%u] total[%u]", length, total);
            dbglog("expr[%p]: %s", expr, expr);
            dbgdump((uchar *)expr, total + 1, "expr[%u]", total + 1);
            
            if (*(expr + total - 1) == '\n')
                break;
        }

        if (*expr == '\n' || !(*expr)) { /* 改行のみ */
            FREEALL;
            continue;
        }
        *(expr + total - 1) = '\0'; /* 改行削除 */
        dbgdump((uchar *)expr, total + 1, "expr[%u]", total + 1);

        if (!strcmp((char *)expr, "quit") || !strcmp((char *)expr, "exit"))
            break;

        result = input(expr, sizeof(expr));
        dbglog("result[%p]", result);
        if (result) {
            retval = fprintf(stdout, "%s\n", (char *)result);
            if (retval < 0)
                outlog("fprintf[%d]", retval);
            retval = fflush(stdout);
            if (retval < 0)
                outlog("fflush[%d]", retval);
        }
        FREEALL;
        dbglog("result[%p]", result);
        clearerr(stdin);
    }
    FREEALL;
    return EXIT_SUCCESS;
}

/** 
 * シグナルハンドラ
 *
 * @param[in] signo シグナル
 * @return なし
 */
static void sig_handler(int signo)
{
    FREEALL;
    clearerr(stdin);
    _exit(EXIT_SUCCESS);
}

