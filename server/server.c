/**
 * @file  server/server.c
 * @brief ソケット送受信
 *
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
#include <stdbool.h>    /* true */
#include <ctype.h>      /* isdigit */
#include <sys/socket.h> /* socket setsockopt */
#include <sys/types.h>  /* setsockopt */
#include <arpa/inet.h>  /* inet_addr */
#include <errno.h>      /* errno */
#include <unistd.h>     /* close access */
#include <pthread.h>    /* pthread */
#include <signal.h>     /* sig_atomic_t */
#include <errno.h>      /* errno */

#include "def.h"
#include "log.h"
#include "data.h"
#include "net.h"
#include "util.h"
#include "calc.h"
#include "option.h"
#include "server.h"

#define SOCK_ERROR  -1 /**< ソケットエラー */

/* 内部関数 */
/** サーバプロセス */
static void *server_proc(void *arg);

/**
 * 接続受付
 *
 * @param[in] sock ソケット
 * @return なし
 */
void
server_loop(int sock)
{
    struct sockaddr_in addr; /* ソケットアドレス情報構造体 */
    int addrlen = 0;         /* sockaddr_in構造体のサイズ */
    pthread_t thread_id;     /* スレッドID */
    int retval = 0;          /* 戻り値 */
    int acc = -1;            /* accept戻り値 */

    dbglog("start");

    addrlen = sizeof(addr);
    do {
        /* 接続受付 */
        errno = 0;
        acc = accept(sock, (struct sockaddr *)&addr, (socklen_t *)&addrlen);
        if (acc < 0) {
            if (errno != EINTR)
                outlog("accept=%d", acc);
            continue;
        }
        dbglog("accept=%d, addr.sin_addr=%s addr.sin_port=%d",
               acc, inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
        /* スレッド生成 */
        retval = pthread_create(&thread_id, NULL, server_proc, (void *)acc);
        if (retval) {
            outlog("pthread_create=%d", thread_id);
            /* アクセプトクローズ */
            close_sock(&acc);
        }
        dbglog("pthread_create=%d", thread_id);
    } while (!sig_handled && !sighup_handled);
}

/**
 * ソケット接続
 *
 * @param[in] port ポート番号またはサービス名
 * @return ソケット
 */
int
server_sock(const char *port)
{
    struct sockaddr_in addr; /* ソケットアドレス情報構造体 */
    int retval = 0;          /* 戻り値 */
    int optval = 1;          /* 二値オプションを有効にする */
    int sock = -1;           /* ソケット */

    dbglog("start");

    /* 初期化 */
    (void)memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* ポート番号またはサービス名を設定 */
    if (set_port(&addr, port) < 0)
        return SOCK_ERROR;

    /* ソケット生成 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        outlog("sock=%d", sock);
        return SOCK_ERROR;
    }

    /* ソケットオプション */
    retval = setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &optval,
                        (socklen_t)sizeof(int));
    if (retval < 0) {
        outlog("setsockopt=%d, sock=%d", retval, sock);
        goto error_handler;
    }

    /* ソケットにアドレスを指定 */
    retval = bind(sock, (struct sockaddr *)&addr, (socklen_t)sizeof(addr));
    if (retval < 0) {
        if (errno == EADDRINUSE)
            (void)fprintf(stderr, "Address already in use\n");
        outlog("bind=%d, sock=%d", retval, sock);
        goto error_handler;
    }

    /* アクセスバックログの指定 */
    retval = listen(sock, SOMAXCONN);
    if (retval < 0) {
        outlog("listen=%d sock=%d", retval, sock);
        goto error_handler;
    }

    return sock;

error_handler:
    close_sock(&sock);
    return SOCK_ERROR;
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
    size_t length = 0;                /* 長さ */
    struct header hd;                 /* ヘッダ構造体 */
    int retval = 0;                   /* 戻り値 */
    ushort cs = 0;                    /* チェックサム */
    int acc = -1;                     /* アクセプト */
    struct server_data *sdata = NULL; /* 送信データ構造体 */
    uchar *expr = NULL;               /* 受信データ */
    uchar *result = NULL;             /* 処理結果 */

    dbglog("start");

    /* スレッドデタッチ */
    pthread_detach(pthread_self());

    /* 引数の取得 */
    acc = (int)arg;

    do {
        /* ヘッダ受信 */
        length = sizeof(struct header);
        (void)memset(&hd, 0, length);
        retval = recv_data(acc, &hd, length);
        if (retval < 0) /* エラー */
            break;
        dbglog("recv_data=%d, hd=%p, length=%u, hd.length=%d",
               retval, &hd, length, hd.length);
        if (g_gflag)
            outdump(&hd, length, "hd=%p, length=%u", &hd, length);
        stddump(&hd, length, "hd=%p, length=%u", &hd, length);

        length = hd.length; /* データ長を保持 */

        /* データ受信 */
        expr = recv_data_new(acc, length);
        if (!expr) /* エラー */
            break;

        dbglog("expr=%p, length=%u", expr, length);

        if (g_gflag)
            outdump(expr, length, "expr%p, length=%u", expr, length);
        stddump(expr, length, "expr=%p, length=%u", expr, length);

        /* チェックサム */
        cs = in_cksum((ushort *)expr, length);
        if (cs != hd.checksum) { /* チェックサムエラー */
            outlog("checksum error: 0x%x!=0x%x", cs, hd.checksum);
            memfree(1, &expr);
            break;
        }

        if (!strcmp((char *)expr, "exit") ||
            !strcmp((char *)expr, "quit")) {
            memfree(1, &expr);
            break;
        }

        /* サーバ処理 */
        init_calc(expr, g_digit);
        result = answer();
        dbglog("result=%s", result);

        length = strlen((char *)result) + 1; /* 文字列長保持 */
        dbgdump(result, length, "result=%p, length=%u", result, length);

        /* データ送信 */
        sdata = set_server_data(sdata, result, length);
        if (!sdata) {
            destroy_calc();
            memfree(1, &expr);
            break;
        }
        length += sizeof(struct header);

        if (g_gflag)
            outdump(sdata, length, "sdata=%p, length=%u", sdata, length);
        stddump(sdata, length, "sdata=%p, length=%u", sdata, length);

        retval = send_data(acc, sdata, length);
        if (retval < 0) { /* エラー */
            destroy_calc();
            memfree(2, &expr, &sdata);
            break;
        }
        dbglog("send_data%d, sdata=%p, length=%u", retval, sdata, length);

        destroy_calc();
        memfree(2, &expr, &sdata);

    } while (!sig_handled && !sighup_handled);

    close_sock(&acc);

    return NULL;
}

