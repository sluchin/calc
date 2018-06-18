/**
 * @file  calc/main.c
 * @brief main関数
 *
 * @author higashi
 * @date 2010-06-27 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
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
#include <signal.h>  /* sigaction */
#ifdef HAVE_READLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#else
#  include "readline.h"
#endif /* HAVE_READLINE */

#include "def.h"
#include "memfree.h"
#include "option.h"
#include "log.h"
#include "term.h"
#include "calc.h"

/* 内部変数 */
static volatile sig_atomic_t sig_handled = 0; /**< シグナル */
#ifdef HAVE_READLINE
static HIST_ENTRY *history = NULL;            /**< 履歴 */
static const unsigned int MAX_HISTORY = 100;  /**< 最大履歴数 */
#endif /* HAVE_READLINE */

/* 内部関数 */
/** ループ処理 */
static void main_loop(void);
#ifdef HAVE_READLINE
/* イベントフック */
static int check_state(void);
/* history クリア */
static void freehistory(HIST_ENTRY **hist);
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
int
main(int argc, char *argv[])
{
    dbglog("start");

    set_progname(argv[0]);

    /* シグナルハンドラ */
    set_sig_handler();

    /* バッファリングしない */
    if (setvbuf(stdin, (char *)NULL, _IONBF, 0))
        outlog("setvbuf: stdin");
    if (setvbuf(stdout, (char *)NULL, _IONBF, 0))
        outlog("setvbuf: stdout");

    /* オプション引数 */
    parse_args(argc, argv);

    /* メインループ */
    main_loop();

#ifdef HAVE_READLINE
    freehistory(&history);
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
    int retval = 0;              /* 戻り値 */
    calcinfo calc;               /* calcinfo構造体 */
    unsigned char *expr = NULL;  /* 式 */
#ifdef HAVE_READLINE
    unsigned int hist_no = 0;    /* 履歴数 */
    char *prompt = NULL;         /* プロンプト */

    rl_event_hook = &check_state;
#endif /* HAVE_READLINE */

    dbglog("start");

    retval = fflush(NULL);
    if (retval == EOF)
        outlog("fflush");

    do {
        dbgterm(STDIN_FILENO);
#ifdef HAVE_READLINE
        expr = (unsigned char *)readline(prompt);
#else
        expr = _readline(stdin);
#endif /* HAVE_READLINE */
        if (!expr)
            break;

        if (*expr == '\0') { /* 文字列長ゼロ */
            dbglog("expr=%p", expr);
            memfree((void **)&expr, NULL);
            continue;
        }
        dbgdump(expr, strlen((char *)expr) + 1,
                "stdin: expr=%p, strlen=%zu",
                expr, strlen((char *)expr) + 1);

        if (!strcmp((char *)expr, "quit") ||
            !strcmp((char *)expr, "exit"))
            break;

        (void)memset(&calc, 0, sizeof(calcinfo));

        if (!create_answer(&calc, expr)) { /* メモリ不足 */
            outlog("create_calc");
        } else {
            dbglog("expr=%p, answer=%p", expr, calc.answer);
            retval = fprintf(stdout, "%s\n", (char *)calc.answer);
            if (retval < 0)
                outlog("fprintf=%d", retval);
        }
#ifdef HAVE_READLINE
        if (MAX_HISTORY <= ++hist_no)
            freehistory(&history);
        add_history((char *)expr);
#endif /* HAVE_READLINE */

        destroy_answer(&calc);
        memfree((void **)&expr, NULL);

    } while (!sig_handled);
}

#ifdef HAVE_READLINE
/**
 * イベントフック
 *
 * readline 内から定期的に呼ばれる関数
 * @return 常にEX_OK
 */
static int check_state(void) {
    if (sig_handled) {
        /* 入力中のテキストを破棄 */
        rl_delete_text(0, rl_end);

        /* readlineをreturnさせる */
        rl_done = 1;
    }
    return EX_OK;
}

/**
 * history クリア
 *
 * @return なし
 */
static void
freehistory(HIST_ENTRY **hist)
{
    if (*hist) {
        *hist = remove_history(0);
        free(*hist);
    }
    *hist = NULL;
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
    struct sigaction sa; /* sigaction構造体 */
    sigset_t sigmask;    /* シグナルマスク */

    /* シグナルマスクの設定 */
    (void)memset(&sa, 0, sizeof(struct sigaction));
    if (sigemptyset(&sigmask) < 0)
        outlog("sigemptyset=0x%x", sigmask);
    if (sigfillset(&sigmask) < 0)
        outlog("sigfillset=0x%x", sigmask);
    dbglog("sigmask=0x%x", sigmask);

    /* シグナル補足 */
    if (sigaction(SIGINT, (struct sigaction *)NULL, &sa) < 0)
        outlog("sigaction=%p, SIGINT", &sa);
    sa.sa_handler = sig_handler;
    sa.sa_mask = sigmask;
    if (sigaction(SIGINT, &sa, (struct sigaction *)NULL) < 0)
        outlog("sigaction=%p, SIGINT", &sa);

    if (sigaction(SIGTERM, (struct sigaction *)NULL, &sa) < 0)
        outlog("sigaction=%p, SIGTERM", &sa);
    sa.sa_handler = sig_handler;
    sa.sa_mask = sigmask;
    if (sigaction(SIGTERM, &sa, (struct sigaction *)NULL) < 0)
        outlog("sigaction=%p, SIGTERM", &sa);

    if (sigaction(SIGQUIT, (struct sigaction *)NULL, &sa) < 0)
        outlog("sigaction=%p, SIGQUIT", &sa);
    sa.sa_handler = sig_handler;
    sa.sa_mask = sigmask;
    if (sigaction(SIGQUIT, &sa, (struct sigaction *)NULL) < 0)
        outlog("sigaction=%p, SIGQUIT", &sa);
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

