/**
 * @file  server/main.c
 * @brief main関数
 *
 * @author higashi
 * @date 2009-06-25 higashi 新規作成
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

#include <stdlib.h> /* exit EXIT_SUCCESS */
#include <string.h> /* memset */
#include <unistd.h> /* close */
#include <libgen.h> /* basename */
#include <signal.h> /* sigaction */

#include "log.h"
#include "net.h"
#include "util.h"
#include "option.h"
#include "server.h"

/* 外部変数 */
volatile sig_atomic_t sig_handled = 0;    /**< シグナル */
volatile sig_atomic_t sighup_handled = 0; /**< シグナル */

/* 内部変数 */
static int sockfd = -1; /**< ソケット */
static int *argc;       /**< 引数の数 */
static char ***argv;    /**< コマンド引数 */
static char ***envp;    /**< 環境変数 */

/* 内部関数 */
/** シグナルハンドラ設定 */
static void set_sig_handler(void);
/** シグナルハンドラ(SIGINT SIGQUIT SIGTERM) */
static void sig_handler(int signo);
/** シグナルハンドラ(SIGHUP) */
static void sighup_handler(int signo);

/**
 * main関数
 *
 * @param[in] c 引数の数
 * @param[in] v コマンド引数・オプション引数
 * @param[in] ep 環境変数
 * @return 常にEXIT_SUCCESS
 */
int main(int c, char *v[], char *ep[])
{
#ifndef _DEBUG
    int retval = 0; /* 戻り値 */
#endif
    dbglog("start");

    /* アドレスを保持 */
    argc = &c;
    argv = &v;
    envp = &ep;

    /* シグナルハンドラ */
    set_sig_handler();

    /* プログラム名を保持 */
    progname = basename(v[0]);

    /* オプション引数 */
    parse_args(c, v);

    /* ソケット接続 */
    sockfd = server_sock(g_port_no);
    if (sockfd < 0)
        exit(EXIT_FAILURE);

    /* デーモン化する */
#ifndef _DEBUG
    retval = daemon(0, 0);
    if (retval < 0) {
        outlog("daemon[%d]", retval);
        exit(EXIT_FAILURE);
    }
#endif /* _DEBUG */

    /* ソケット送受信 */
    server_loop(sockfd);

    /* ソケットクローズ */
    close_sock(&sockfd);

    if (sighup_handled) { /* 再起動 */
        dbglog("signal hup");
        (void)alarm(0);
        sighup_handled = 0;
        (void)execve((*argv)[0], (*argv), (*envp));
    }

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
    struct sigaction sa, hup; /* シグナル */

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

    /* シグナルマスクの設定 */
    (void)memset(&hup, 0, sizeof(hup));
    hup.sa_handler = sighup_handler;
    hup.sa_flags = 0;
    if (sigemptyset(&hup.sa_mask) < 0)
        outlog("sigemptyset=%p", hup);
    if (sigaddset(&hup.sa_mask, SIGHUP) < 0)
        outlog("sigaddset=%p SIGHUP", hup);

    /* SIGHUPの補足 */
    if (sigaction(SIGHUP, &hup, NULL) < 0)
        outlog("sigaction=%p, SIGHUP", hup);

#if 0
    /* 子プロセスをゾンビにしない */
    struct sigaction chld;
    (void)memset(&chld, 0, sizeof(chld));
    chld.sa_handler = SIG_IGN;
    chld.sa_flags = SA_NOCLDWAIT;
    if (sigaction(SIGCHLD, &chld, NULL) < 0)
        outlog("sigaction[%p]; SIGCHLD", chld);
#endif
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

/**
 * シグナルハンドラ(SIGHUP)
 *
 * 再起動する.
 * @param[in] signo シグナル
 * @return なし
 */
static void sighup_handler(int signo)
{
    sighup_handled = 1;
}

