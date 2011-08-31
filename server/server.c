/**
 * @file  server.c
 * @brief ソケット送受信
 *
 * @sa server.h
 * @author higashi
 * @date 2010-06-26 higashi 新規作成
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

#include <stdio.h>      /* FILE */
#include <stdlib.h>     /* strtol */
#include <string.h>     /* memcpy memset */
#include <ctype.h>      /* isdigit */
#include <sys/socket.h> /* socket setsockopt */
#include <sys/types.h>  /* setsockopt */
#include <arpa/inet.h>  /* inet_addr */
#include <errno.h>      /* errno */
#include <unistd.h>     /* close access */
#include <pthread.h>    /* pthread */

#include "log.h"
#include "data.h"
#include "util.h"
#include "calc.h"
#include "server.h"

#define SOCK_ERROR  (int)(-1) /**< ソケットエラー */

/* 外部変数 */
extern bool gflag;            /**< gオプションフラグ */

/* 内部関数 */
/** サーバプロセス */
static void *server_proc(void *arg);

/**
 * ソケット接続
 */
int
server_sock(const char *port)
{
    struct sockaddr_in addr; /* ソケットアドレス情報構造体 */
    int sock = -1;           /* ソケット */
    int retval = 0;          /* 戻り値 */
    int optval = 1;          /* 二値オプションを有効にする */

    dbglog("start");

    /* 初期化 */
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* ポート番号またはサービス名を設定 */
    if (set_port(&addr, port) < 0)
        return SOCK_ERROR;

    /* ソケット生成 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        outlog("sock[%d]", sock);
        return SOCK_ERROR;
    }

    /* ソケットオプション */
    retval = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval,
                        (socklen_t)sizeof(int));
    if (retval < 0) {
        outlog("setsockopt[%d]: sock[%d]", retval, sock);
        goto error_handler;
    }

    /* ソケットにアドレスを指定 */
    retval = bind(sock, (struct sockaddr *)&addr, (socklen_t)sizeof(addr));
    if (retval < 0) {
        outlog("bind[%d]: sock[%d]", retval, sock);
        goto error_handler;
    }

    /* アクセスバックログの指定 */
    retval = listen(sock, SOMAXCONN);
    if (retval < 0) {
        outlog("listen[%d]: sock[%d]", retval, sock);
        goto error_handler;
    }

    return sock;

error_handler:
    /* ソケットクローズ */
    if (sock != -1) {
        retval = close(sock);
        if (retval < 0)
            outlog("close[%d]: sock[%d]", retval, sock);
    }
    return SOCK_ERROR;
}

/**
 * 接続受付
 */
void
server_loop(int sock)
{
    struct sockaddr_in addr; /* ソケットアドレス情報構造体 */
    int addrlen = 0;         /* sockaddr_in構造体のサイズ */
    pthread_t thread_id;     /* スレッドID */
    int retval = 0;          /* 戻り値 */
    int acc = 0;             /* accept戻り値 */

    dbglog("start");

    addrlen = sizeof(addr);
    while (true) {
        /* 接続受付 */
        errno = 0;
        acc = accept(sock, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (acc < 0) {
            if (errno == EINTR) { /* 割り込み */
                outlog("interrupt[%d] acc[%d]", errno, acc);
                continue;
            }
            outlog("accept[%d]", acc);
            continue;
        }
        dbglog("accept[%d]: %s: %d", acc, inet_ntoa(addr.sin_addr),
               ntohs(addr.sin_port));
        /* スレッド生成 */
        retval = pthread_create(&thread_id, NULL, server_proc, (void *)acc);
        if (retval) {
            outlog("pthread_create[%d]", thread_id);
            /* アクセプトクローズ */
            retval = close(acc);
            if (retval < 0)
                outlog("close[%d]: acc[%d]", retval, acc);
            continue;
        }
        dbglog("pthread_create[%d]", thread_id);
    } /* while */
}

/**
 * サーバプロセス
 *
 * @param[in] arg ソケットディスクリプタ
 * @return 常にNULL
 */
static void *
server_proc(void *arg)
{
    size_t length = 0;            /* 長さ */
    unsigned char *result = NULL; /* 処理結果 */
    struct client_data rdata;     /* 受信データ構造体 */
    struct server_data sdata;     /* 送信データ構造体 */
    int retval = 0;               /* 戻り値 */
    unsigned short cs = 0;        /* チェックサム */
    int acc = 0;                  /* アクセプト */

    dbglog("start");

    /* スレッドデタッチ */
    pthread_detach(pthread_self());

    /* 引数の取得 */
    acc = (int)arg;

    while (true) {
        /* ヘッダ受信 */
        memset(&rdata, 0, sizeof(struct server_data));
        length = sizeof(struct header);
        retval = recv_data(acc, &rdata, length);
        if (retval < 0) /* エラー */
            break;
        stdlog("recv_data[%d]: rdata[%p]: length[%u]: %d",
               retval, &rdata, length, rdata.hd.length);
        if (gflag)
            outdump(&rdata, length, "rdata[%u]", length);

        length = rdata.hd.length; /* データ長を保持 */

        /* データ受信 */
        retval = recv_data(acc, rdata.expression, length);
        if (retval < 0) /* エラー */
            break;
        stdlog("recv_data[%d]: rdata[%p]: length[%u]",
               retval, &rdata, length);
        if (gflag)
            outdump(rdata.expression, length, "rdata.expression[%d]", length);

        /* チェックサム */
        cs = in_cksum((unsigned short *)rdata.expression, length);
        if (cs != rdata.hd.checksum) { /* チェックサムエラー */
            outlog("checksum error; cs[0x%x!=0x%x]", cs, rdata.hd.checksum);
            break;
        }

        if (!strcmp((char *)rdata.expression, "exit") ||
            !strcmp((char *)rdata.expression, "quit"))
            break;

        /* サーバ処理 */
        result = input(rdata.expression, length);

        /* データ送信 */
        memset(&sdata, 0, sizeof(struct client_data));
        set_server_data(&sdata, result, strlen((char *)result));
        length = sizeof(struct header) + strlen((char *)result);
        retval = send_data(acc, &sdata, length);
        if (retval < 0) /* エラー */
            break;
        stdlog("send_data[%d]: sdata[%p]: length[%u]",
               retval, &sdata, length);
        if (gflag)
            outdump(&sdata, length, "sdata[%u]", length);
    }

    /* アクセプトソケットクローズ */
    retval = close(acc);
    if (retval < 0)
        outlog("close[%d]: acc[%d]", retval, acc);

    /* メモリ開放 */
    free(result);
    result = NULL;

    return NULL;
}

