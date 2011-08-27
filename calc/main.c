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
#if defined(_USE_READLINE)
#include <readline/readline.h>
#include <readline/history.h>
#endif /* _USE_READLINE */

#include "common.h"
#include "data.h"
#include "option.h"
#include "log.h"
#include "calc.h"

/* 外部変数 */
char *progname;                    /**< プログラム名 */
/* 内部変数 */
static uchar *expr = NULL;         /**< 式 */
static uchar *result = NULL;       /**< メッセージ */
#if defined(_USE_READLINE)
static HIST_ENTRY *history = NULL; /**< 履歴 */
#endif /* _USE_READLINE */

/* メモリ解放 */
#define FREEALL                                         \
    if (result) { free(result); } result = NULL;        \
    if (expr) { free(expr); } expr = NULL;

#if defined(_USE_READLINE)
#  define FREEHISTORY                                   \
    history = remove_history(0);                        \
    if (history) { free(history); } history = NULL;
#  define MAX_HISTORY    100 /**< 最大履歴数 */
#else
#  define FREEHISTORY do { } while(0);
#endif /* _USE_READLINE */


/* 内部関数 */
/** 標準入力読込 */
#if !defined(_USE_READLINE)
static uchar *read_stdin(void);
#endif /* _USE_READLINE */

/** シグナルハンドラ */
static void sig_handler(int signo);

#if !defined(_USE_READLINE)
/**
 * 標準入力読込
 *
 * @return 文字列
 */
static uchar *
read_stdin(void)
{
    char *retval = NULL; /* fgetsの戻り値 */
    size_t length = 0;   /* 文字列長 */
    size_t total = 0;    /* 文字列長全て */
    uchar buf[BUF_SIZE]; /* バッファ */
    uchar *line = NULL;  /* 文字列 */
    uchar *tmp = NULL;   /* 一時アドレス */

    while (true) {
        retval = fgets((char *)buf, sizeof(buf), stdin);
        if (feof(stdin) || ferror(stdin)) { /* エラー */
            outlog("fgets[%p]", retval);
            FREEALL;
            clearerr(stdin);
            break;
        }
        length = strlen((char *)buf);
        dbgdump((uchar *)buf, length, "buf[%u]", length);

        tmp = (uchar *)realloc((uchar *)line,
                               (total + length + 1) * sizeof(uchar));
        if (!tmp) {
            outlog("realloc[%p]: %u", line, total + length + 1);
            FREEALL;
            break;
        }
        line = tmp;

        (void)memcpy((uchar *)line + total, buf, length + 1);

        total += length;
        dbglog("length[%u] total[%u]", length, total);
        dbglog("line[%p]: %s", line, line);
        dbgdump((uchar *)line, total + 1, "line[%u]", total + 1);

        if (*(line + total - 1) == '\n')
            break;
    }
    *(line + total - 1) = '\0'; /* 改行削除 */

    return line;
}
#endif /* _USE_READLINE */

/**
 * ループ処理
 *
 * @return なし
 */
static void
main_loop(void)
{
    int retval = 0;      /* 戻り値 */
    size_t length = 0;   /* 文字列長 */
#if defined(_USE_READLINE)
    int hist_no = 0;     /* 履歴数 */
    char *prompt = NULL; /* プロンプト */
#endif /* _USE_READLINE */

    while (true) {
        retval = fflush(NULL);
        if (retval == EOF)
            outlog("fflush[%d]", retval);

#if defined(_USE_READLINE)
        expr = (uchar *)readline(prompt);
#else
        expr = read_stdin();
#endif /* _USE_READLINE */
        if (*expr == '\n' || !(*expr)) { /* 改行のみ */
            FREEALL;
            continue;
        }
        length = strlen((char *)expr);
        dbgdump((uchar *)expr, length + 1, "expr[%u]", length + 1);

        if (!strcmp((char *)expr, "quit") || !strcmp((char *)expr, "exit"))
            break;

        result = input(expr, sizeof(expr));
        dbglog("result[%p]", result);
        if (result) {
            retval = fprintf(stdout, "%s\n", (char *)result);
            if (retval < 0)
                outlog("fprintf[%d]", retval);
            retval = fflush(stdout);
            if (retval == EOF)
                outlog("fflush[%d]", retval);
        }
#if defined(_USE_READLINE)
        add_history((char *)expr);
        if (MAX_HISTORY < ++hist_no)
            FREEHISTORY;
#endif /* _USE_READLINE */
        FREEALL;
        dbglog("result[%p]", result);
    }
    FREEALL;
    FREEHISTORY;
}

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

    main_loop();

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
    FREEHISTORY;
    _exit(EXIT_SUCCESS);
}

