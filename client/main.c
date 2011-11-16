/**
 * @file  client/main.c
 * @brief main関数
 *
 * @author higashi
 * @date 2009-06-23 higashi 新規作成
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

#include <stdio.h>  /* FILE stderr */
#include <stdlib.h> /* exit EXIT_SUCCESS */
#include <string.h> /* memset strlen */
#include <unistd.h> /* close write */
#include <libgen.h> /* basename */
#include <signal.h> /* sigaction */

#include "log.h"
#include "net.h"
#include "util.h"
#include "option.h"
#include "client.h"

/* 外部変数 */
volatile sig_atomic_t sig_handled = 0; /**< シグナル */
struct sigaction g_sigaction;          /**< sigaction構造体 */

/* 内部変数 */
static int sockfd = -1; /**< ソケット */

/* 内部関数 */
/** シグナルハンドラ設定 */
static void set_sig_handler(void);
/* シグナルハンドラ */
static void sig_handler(int signo);

/**
 * main関数
 *
 * @param[in] argc 引数の数
 * @param[in] argv コマンド引数・オプション引数
 * @retval EXIT_FAILURE ソケット接続失敗
 */
int main(int argc, char *argv[])
{
    dbglog("start");

    progname = basename(argv[0]);

    /* シグナルハンドラ */
    set_sig_handler();

    /* オプション引数 */
    parse_args(argc, argv);

    /* ソケット接続 */
    sockfd = connect_sock(g_host_name, g_port_no);
    if (sockfd < 0) {
        (void)fprintf(stderr, "connect error\n");
        return EXIT_FAILURE;
    }

    dbglog("sockfd=%d", sockfd);

    /* ソケット送受信 */
    client_loop(sockfd);

    /* ソケットクローズ */
    close_sock(&sockfd);

    exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

/**
 * シグナルハンドラ設定
 *
 * @return なし
 */
static void
set_sig_handler(void)
{
    /* シグナルマスクの設定 */
    (void)memset(&g_sigaction, 0, sizeof(g_sigaction));
    g_sigaction.sa_handler = sig_handler;
    g_sigaction.sa_flags = 0;
    if (sigemptyset(&g_sigaction.sa_mask) < 0)
        outlog("sigemptyset=%p", g_sigaction);
    if (sigaddset(&g_sigaction.sa_mask, SIGINT) < 0)
        outlog("sigaddset=%p, SIGINT", g_sigaction);
    if (sigaddset(&g_sigaction.sa_mask, SIGTERM) < 0)
        outlog("sigaddset=%p, SIGTERM", g_sigaction);
    if (sigaddset(&g_sigaction.sa_mask, SIGQUIT) < 0)
        outlog("sigaddset=%p, SIGQUIT", g_sigaction);

    /* シグナル補足 */
    if (sigaction(SIGINT, &g_sigaction, NULL) < 0)
        outlog("sigaction=%p, SIGINT", g_sigaction);
    if (sigaction(SIGTERM, &g_sigaction, NULL) < 0)
        outlog("sigaction=%p, SIGTERM", g_sigaction);
    if (sigaction(SIGQUIT, &g_sigaction, NULL) < 0)
        outlog("sigaction=%p, SIGQUIT", g_sigaction);
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

