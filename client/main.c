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

#include <stdio.h>  /* stderr */
#include <stdlib.h> /* exit EXIT_SUCCESS */
#include <string.h> /* memset */
#include <signal.h> /* sigaction */

#include "log.h"
#include "net.h"
#include "option.h"
#include "client.h"

/* 内部変数 */
static int sockfd = -1; /**< ソケット */

/* 内部関数 */
/** atexit登録関数 */
static void exit_close_sock(void);
/** シグナルハンドラ設定 */
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
int main(int argc, char *argv[])
{
    st_client status = EX_SUCCESS; /* ステータス */

    dbglog("start");

    set_progname(argv[0]);

    /* シグナルハンドラ設定 */
    set_sig_handler();

    /* バッファリングしない */
    if (setvbuf(stdin, (char *)NULL, _IONBF, 0))
        outlog("setvbuf: stdin");
    if (setvbuf(stdout, (char *)NULL, _IONBF, 0))
        outlog("setvbuf: stdout");

    /* オプション引数 */
    parse_args(argc, argv);

    /* 関数登録 */
    if (atexit(exit_close_sock)) {
        outlog("atexit");
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
 *
 * @return なし
 */
static void
exit_close_sock(void)
{
    close_sock(&sockfd);
}

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

    (void)memset(&sa, 0, sizeof(struct sigaction));

    /* シグナルマスクの設定 */
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
    g_sig_handled = 1;
}

