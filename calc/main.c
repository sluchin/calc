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
 * This program is distributed in the hope that it will be useful, * but WITHOUT ANY WARRANTY; without even the implied warranty of
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
#ifdef HAVE_READLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#else
#  include "readline.h"
#endif /* HAVE_READLINE */

#include "def.h"
#include "util.h"
#include "data.h"
#include "option.h"
#include "log.h"
#include "calc.h"

/* 内部変数 */
static volatile sig_atomic_t sig_handled = 0; /**< シグナル */
static uchar *expr = NULL;                    /**< 式 */
static uchar *result = NULL;                  /**< メッセージ */
#ifdef HAVE_READLINE
static HIST_ENTRY *history = NULL;            /**< 履歴 */
#endif /* HAVE_READLINE */

#ifdef HAVE_READLINE
#  define FREEHISTORY                           \
    if (history) {                              \
        history = remove_history(0);            \
        free(history); }                        \
    history = NULL;
#  define MAX_HISTORY    100 /**< 最大履歴数 */
#endif /* HAVE_READLINE */

/* 内部関数 */
/** ループ処理 */
static void main_loop(void);
/* イベントフック */
#ifdef HAVE_READLINE
static int check_state(void);
#endif /* HAVE_READLINE */
/** シグナルハンドラ設定 */
static void set_sig_handler(void);
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
    dbglog("start");

    progname = basename(argv[0]);

    /* シグナルハンドラ */
    set_sig_handler();

    /* オプション引数 */
    parse_args(argc, argv);

    /* メインループ */
    main_loop();

#ifdef HAVE_READLINE
    FREEHISTORY;
#endif /* HAVE_READLINE */

    exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

/**
 * ループ処理
 *
 * @return なし
 */
static void
main_loop(void)
{
    int retval = 0;       /* 戻り値 */
    calcinfo *tsd = NULL; /* calc情報構造体 */
#ifdef HAVE_READLINE
    int hist_no = 0;      /* 履歴数 */
    char *prompt = NULL;  /* プロンプト */

    rl_event_hook = &check_state;
#endif /* HAVE_READLINE */

    dbglog("start");

    do {
        retval = fflush(NULL);
        if (retval == EOF)
            outlog("fflush=%d", retval);

#ifdef HAVE_READLINE
        expr = (uchar *)readline(prompt);
#else
        expr = _readline(stdin);
#endif /* HAVE_READLINE */
        if (!expr)
            break;

        if (*expr == 0) { /* 空 */
            dbglog("expr=%p", expr);
            memfree(1, &expr);
            continue;
        }
        if (!strcmp((char *)expr, "quit") ||
            !strcmp((char *)expr, "exit"))
            break;
        dbgdump(expr, strlen((char *)expr) + 1,
                "expr=%u", strlen((char *)expr) + 1);

        tsd = init_calc(expr, g_digit);
        if (!tsd) { /* エラー */
            outlog("tsd=%p", tsd);
            memfree(1, &expr);
            continue;
        }
        result = answer(tsd);
        dbglog("expr=%p, result=%p", expr, result);
        if (result) {
            retval = fprintf(stdout, "%s\n", (char *)result);
            if (retval < 0)
                outlog("fprintf=%d", retval);
            retval = fflush(stdout);
            if (retval == EOF)
                outlog("fflush=%d", retval);
        }
#ifdef HAVE_READLINE
        if (MAX_HISTORY <= ++hist_no) {
            FREEHISTORY;
        }
        add_history((char *)expr);
#endif /* HAVE_READLINE */

        destroy_calc(tsd);
        memfree(1, &expr);
        expr = result = NULL;

    } while (!sig_handled);
}

/**
 * イベントフック
 *
 * readline 内から定期的に呼ばれる関数
 * @return 常にEX_OK
 */
#ifdef HAVE_READLINE
static int check_state(void) {
    if (sig_handled) {
        /* 入力中のテキストを破棄 */
        rl_delete_text(0, rl_end);

        /* readlineをreturnさせる */
        rl_done = 1;

        //rl_event_hook = 0;
        //rl_deprep_terminal();
        //close(STDIN_FILENO);
    }
    return EX_OK;
}
#endif /* HAVE_READLINE */

/**
 * シグナルハンドラ設定
 *
 * @return なし
 */
static void
set_sig_handler(void)
{
    struct sigaction sa; /* シグナル */

    /* シグナルマスクの設定 */
    (void)memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sig_handler;
    sa.sa_flags = 0;
    if (sigemptyset(&sa.sa_mask) < 0)
        outlog("sigemptyset=%p", sa);
    if (sigaddset(&sa.sa_mask, SIGINT) < 0)
        outlog("sigaddset=%p, SIGINT", sa);
    if (sigaddset(&sa.sa_mask, SIGTERM) < 0)
        outlog("sigaddset=%p, SIGTERM", sa);
    if (sigaddset(&sa.sa_mask, SIGQUIT) < 0)
        outlog("sigaddset=%p, SIGQUIT", sa);

    /* シグナル補足 */
    if (sigaction(SIGINT, &sa, NULL) < 0)
        outlog("sigaction=%p, SIGINT", sa);
    if (sigaction(SIGTERM, &sa, NULL) < 0)
        outlog("sigaction=%p, SIGTERM", sa);
    if (sigaction(SIGQUIT, &sa, NULL) < 0)
        outlog("sigaction=%p, SIGQUIT", sa);
}

/**
 * シグナルハンドラ
 *
 * @param[in] signo シグナル
 * @return なし
 */
static void sig_handler(int signo)
{
    sig_handled = 1;
}

