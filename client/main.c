/**
 * @file  client/main.c
 * @brief main関数
 *
 * @author higashi
 * @date 2010-06-23 higashi 新規作成
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

#include <stdio.h>  /* stderr */
#include <stdlib.h> /* exit EXIT_SUCCESS */
#include <string.h> /* memset */
#include <signal.h> /* SIGINT SIGTERM SIGQUIT SIGPIPE */

#include "log.h"
#include "net.h"
#include "sig.h"
#include "option.h"
#include "client.h"

/* 内部変数 */
static int sockfd = -1; /**< ソケット */

/* 内部関数 */
static void exit_close_sock(void);
static void set_sig_handler(void);
/* シグナルハンドラ */
static void sig_handler(int signo);

/**
 * main関数
 *
 * @param[in] argc 引数の数
 * @param[in] argv コマンド引数・オプション引数
 * @return ステータス
 */
int
main(int argc, char *argv[])
{
    st_client status = EX_SUCCESS; /* ステータス */
    int retval = 0;                /* 戻り値 */

    dbglog("start");

    set_progname(argv[0]);

    /* シグナルハンドラ設定 */
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

    /* 関数登録 */
    retval = atexit(exit_close_sock);
    if (retval != 0) {
        outlog("atexit=%d", retval);
        exit(EX_FAILURE);
    }

    /* ソケット接続 */
    sockfd = connect_sock();
    if (sockfd < 0) {
        (void)fprintf(stderr, "Connect error\n");
        exit(EX_CONNECT_ERR);
    }

    /* ソケット送受信 */
    status = client_loop(sockfd);

    exit(status);
    return status;
}

/**
 * atexit登録関数
 */
static void
exit_close_sock(void)
{
    close_sock(&sockfd);
}

/**
 * シグナルハンドラ設定
 */
static void
set_sig_handler(void)
{
    /* シグナルの設定 */
    struct sig_setting {
        int signo;            /* シグナル番号 */
        void (*handler)(int); /* ハンドラ (SIG_IGN で, 無視する) */
        int flags;            /* 追加するフラグ */
    };
    static const struct sig_setting settings[] = {
        {SIGINT,  sig_handler, 0},
        {SIGTERM, sig_handler, 0},
        {SIGQUIT, sig_handler, 0},
        /* 接続先が閉じたあとの send() で, プロセスが終了しないように, SIGPIPE を無視する.
         * (send() が EPIPE を返して, 送信エラーとして処理される) */
        {SIGPIPE, SIG_IGN,     0}
    };
    unsigned int i = 0u; /* 繰り返し */
    int retval = 0;      /* 戻り値 */

    for (i = 0u; i < NELEMS(settings); i++) {
        retval = set_sigaction(settings[i].signo, settings[i].handler, settings[i].flags);
        if (retval < 0)
            outlog("set_sigaction: signo=%d", settings[i].signo);
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
    g_sig_handled = 1;
}
