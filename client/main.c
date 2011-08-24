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
#include "option.h"
#include "client.h"

/* 外部変数 */
char *progname;            /**< プログラム名 */
char host_name[HOST_SIZE]; /**< IPアドレスまたはホスト名 */
char port_no[PORT_SIZE];   /**< ポート番号またはサービス名 */

/* 内部変数 */
static volatile sig_atomic_t sockfd = -1; /**< ソケット */

/* 内部関数 */
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
    FILE *fp = stderr;   /* 標準エラー出力 */
    struct sigaction sa; /* シグナル */
    int retval = 0;      /* 戻り値 */

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

    /* ソケット接続 */
    sockfd = connect_sock(host_name, port_no);
    if (sockfd < 0) {
        (void)fprintf(fp, "connect error\n");
        return EXIT_FAILURE;
    }

    /* ソケット送受信 */
    client_loop(sockfd);

    /* ソケットクローズ */
    if (sockfd != -1) {
        retval = close(sockfd);
        if (retval < 0)
            outlog("close[%d] sockfd[%d]", retval, sockfd);
        sockfd = -1;
    }

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
    int closeval = 0; /* close戻り値 */
    int writeval = 0; /* write戻り値 */
    char *mes = NULL; /* メッセージ */

    /* ソケットクローズ */
    if (sockfd != -1) {
        closeval = close(sockfd);
        if (closeval < 0) {
            mes = "close: error\n";
            writeval = write(STDOUT_FILENO, mes, strlen(mes));
        }
        sockfd = -1;
    }
    _exit(EXIT_SUCCESS);
}

