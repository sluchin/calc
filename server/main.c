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
#include "option.h"
#include "server.h"

/* グローバル変数 */
char *progname;                           /**< プログラム名 */
char port_no[PORT_SIZE];                  /**< ポート番号またはサービス名 */
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

    /* シグナルマスクの設定 */
    (void)memset(&hup, 0, sizeof(hup));
    hup.sa_handler = sighup_handler;
    hup.sa_flags = 0;
    if (sigemptyset(&hup.sa_mask) < 0)
        outlog("sigemptyset[%p]", hup);
    if (sigaddset(&hup.sa_mask, SIGHUP) < 0)
        outlog("sigaddset[%p]; SIGHUP", hup);

    /* SIGHUPの補足 */
    if (sigaction(SIGHUP, &hup, NULL) < 0)
        outlog("sigaction[%p]; SIGHUP", hup);

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
    int retval = 0; /* 戻り値 */

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

    /* デーモン化する */
#ifndef _DEBUG
    retval = daemon(0, 0);
    if (retval < 0) {
        outlog("daemon[%d]", retval);
        exit(EXIT_FAILURE);
    }
#endif /* _DEBUG */

    /* ソケット接続 */
    sockfd = server_sock(port_no);
    if (sockfd < 0)
        exit(EXIT_FAILURE);

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

    return EXIT_SUCCESS;
}

/**
 * @mainpage 処理詳細
 *
 * @section server サーバ
 * -# ポート番号の設定\n
 * -# ソケット生成\n
 * -# ソケットオプション設定\n
 * -# ソケットにアドレス設定(bind)\n
 * -# アクセスバックログの設定(listen)\n
 * -# 接続受付(accept)\n
 * -# スレッド生成(server_proc)\n
 * -# ヘッダ受信\n
 * -# データ受信\n
 * -# データのチェックサムをチェック\n
 * -# サーバ処理\n
 * -# データ送信\n
 *
 * @section client クライアント
 * -# IPアドレスまたはホスト名の設定\n
 * -# ポート番号の設定\n
 * -# ソケット生成\n
 * -# サーバに接続(connect)\n
 * -# 標準入力から文字列取得\n
 * -# データ送信\n
 * -# ヘッダ受信\n
 * -# データ受信\n
 * -# データのチェックサムをチェック\n
 * -# 標準出力に文字列出力\n
 */
/**
 * @page page1 処理シーケンス
 * @msc
 * server,calc,client;
 *
 * server<-server[label="server_sock()"];
 * server<-server[label="server_loop()"];
 * server<-server[label="server_proc()"];
 * client<-client[label="connect_sock()"];
 * client<-client[label="client_loop()"];
 * client box client[label="stdin"];
 * server<-client[label="send_data()"];
 * server<-server[label="recv_data()"];
 * server->calc[label="input()"];
 *
 * calc->calc[label="expression()"];
 * calc->calc[label="term()"];
 * calc->calc[label="factor()"];
 * calc->calc[label="number()"];
 * calc->calc[label="expression()"];
 * ...;
 *
 * server<<calc[label="result"];
 * server->client[label="send_data()"];
 * client<-client[label="recv_data()"];
 * client box client[label="stdout"];
 *
 * @endmsc
 */
