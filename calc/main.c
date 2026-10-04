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

#include <stdio.h>   /* FILE */
#include <stdlib.h>  /* exit EXIT_SUCCESS free */
#include <stdbool.h> /* true */
#include <string.h>  /* memset */
#include <unistd.h>  /* close */
#include <signal.h>  /* SIGINT SIGTERM SIGQUIT */
#ifdef HAVE_READLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#endif /* HAVE_READLINE */

#include "def.h"
#include "memfree.h"
#include "readline.h"
#include "option.h"
#include "log.h"
#include "term.h"
#include "sig.h"
#include "calc.h"

/* 内部変数 */
static volatile sig_atomic_t sig_handled = 0; /**< シグナル */
#ifdef HAVE_READLINE
static const int MAX_HISTORY = 100; /**< 最大履歴数 */
#endif                              /* HAVE_READLINE */

/* 内部関数 */
/** ループ処理 */
static void main_loop(void);
#ifdef HAVE_READLINE
/* イベントフック */
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
int
main(int argc, char *argv[])
{
    int retval = 0; /* 戻り値 */

    dbglog("start");

    set_progname(argv[0]);

    /* シグナルハンドラ */
    set_sig_handler();

    /* バッファリングしない */
    retval = setvbuf(stdin, (char *)NULL, _IONBF, 0);
    if (retval != 0)
        outlog("setvbuf: stdin");
    retval = setvbuf(stdout, (char *)NULL, _IONBF, 0);
    if (retval != 0)
        outlog("setvbuf: stdout");

    /* オプション引数 */
    parse_args(argc, argv);

    /* メインループ */
    main_loop();

    exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

/**
 * ループ処理
 */
static void
main_loop(void)
{
    int retval = 0;               /* 戻り値 */
    calcinfo calc;                /* calcinfo構造体 */
    unsigned char *answer = NULL; /* create_answer戻り値 */
    unsigned char *expr = NULL;   /* 式 */
    bool interactive = false;     /* 標準入力が端末か */
#ifdef HAVE_READLINE
    char *prompt = NULL; /* プロンプト */

    /* readline は, 端末のときだけ使う. 端末でない (パイプやファイル) とき, イベントフック
     * (rl_event_hook) があると, 入力の終わり (EOF) で終了せずに, CPU を使い続ける. */
    interactive = (isatty(STDIN_FILENO) != 0);
    if (interactive) {
        rl_event_hook = &check_state;
        stifle_history(MAX_HISTORY); /* 履歴が上限を超えると, 古いものから削除する */
    }
#endif /* HAVE_READLINE */

    dbglog("start");

    retval = fflush(NULL);
    if (retval == EOF)
        outlog("fflush");

    do {
        dbgterm(STDIN_FILENO);
#ifdef HAVE_READLINE
        if (interactive)
            expr = (unsigned char *)readline(prompt);
        else
#endif /* HAVE_READLINE */
            expr = _readline(stdin);
        if (expr == NULL)
            break;

        if (*expr == '\0') { /* 文字列長ゼロ */
            dbglog("expr=%p", expr);
            memfree(&expr, NULL);
            continue;
        }
        dbgdump(expr, strlen((char *)expr) + 1u, "stdin: expr=%p, strlen=%zu", expr,
                strlen((char *)expr) + 1u);

        if (strcmp((char *)expr, "quit") == 0 || strcmp((char *)expr, "exit") == 0)
            break;

        (void)memset(&calc, 0, sizeof(calcinfo));

        answer = create_answer(&calc, expr);
        if (answer == NULL) { /* メモリ不足 */
            outlog("create_calc");
        } else {
            dbglog("expr=%p, answer=%p", expr, calc.answer);
            retval = fprintf(stdout, "%s\n", (char *)calc.answer);
            if (retval < 0)
                outlog("fprintf=%d", retval);
        }
#ifdef HAVE_READLINE
        if (interactive)
            add_history((char *)expr);
#endif /* HAVE_READLINE */

        destroy_answer(&calc);
        memfree(&expr, NULL);

    } while (sig_handled == 0);
}

#ifdef HAVE_READLINE
/**
 * イベントフック
 *
 * readline 内から定期的に呼ばれる関数
 * @return 常にEX_OK
 */
static int
check_state(void)
{
    if (sig_handled != 0) {
        /* 入力中のテキストを破棄 */
        rl_delete_text(0, rl_end);

        /* readlineをreturnさせる */
        rl_done = 1;
    }
    return EX_OK;
}

#endif /* HAVE_READLINE */

/**
 * シグナルハンドラ設定
 */
static void
set_sig_handler(void)
{
    /* ハンドラで補足するシグナル */
    static const int catch_signals[] = {SIGINT, SIGTERM, SIGQUIT};
    unsigned int i = 0u; /* 繰り返し */
    int retval = 0;      /* 戻り値 */

    for (i = 0u; i < NELEMS(catch_signals); i++) {
        retval = set_sigaction(catch_signals[i], sig_handler, 0);
        if (retval < 0)
            outlog("set_sigaction: signo=%d", catch_signals[i]);
    }
}

/**
 * シグナルハンドラ
 *
 * @param[in] signo シグナル
 */
static void
sig_handler(int signo)
{
    (void)signo; /* 使用しない */
    sig_handled = 1;
}
