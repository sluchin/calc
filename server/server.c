/**
 * @file  server/server.c
 * @brief ソケット送受信
 *
 * @author higashi
 * @date 2010-06-26 higashi 新規作成
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
#include <sys/select.h> /* select */

#include "def.h"
#include "log.h"
#include "data.h"
#include "net.h"
#include "memfree.h"
#include "calc.h"
#include "option.h"
#include "server.h"

/* 内部変数 */
static const int SOCK_ERROR = -1; /**< ソケットエラー */

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
    int ready = 0;           /* pselect戻り値 */
    struct sockaddr_in addr; /* ソケットアドレス情報構造体 */
    int addrlen = 0;         /* sockaddr_in構造体のサイズ */
    pthread_t tid;           /* スレッドID */
    int retval = 0;          /* pthread_create戻り値 */
    int acc = -1;            /* accept戻り値 */
    fd_set fds, rfds;        /* selectマスク */
    struct timespec timeout; /* タイムアウト値 */

    dbglog("start: sock=%d", sock);

    /* マスクの設定 */
    FD_ZERO(&fds);      /* 初期化 */
    FD_SET(sock, &fds); /* ソケットをマスク */

    /* タイムアウト値初期化 */
    (void)memset(&timeout, 0, sizeof(struct timespec));
    /* pselectの場合, constなのでループ前で値を入れる */
    timeout.tv_sec = 0;  /* ポーリング */
    timeout.tv_nsec = 0;

    /* ノンブロッキングに設定 */
    if (set_block(sock, NONBLOCK) < 0)
        return;

    do {
        (void)memcpy(&rfds, &fds, sizeof(fd_set)); /* マスクコピー */
        ready = pselect(sock + 1, &rfds,
                        NULL, NULL, &timeout, &g_sigaction.sa_mask);
        if (ready < 0) {
            if (errno == EINTR) /* 割り込み */
                break;
            outlog("select=%d", ready);
            break;
        } else if (ready) {
            if (FD_ISSET(sock, &rfds)) {
                /* 接続受付 */
                /* addrlenは入出力なのでここで初期化する */
                addrlen = (int)sizeof(addr);
                acc = accept(sock, (struct sockaddr *)&addr,
                             (socklen_t *)&addrlen);
                if (acc < 0) {
                    outlog("accept=%d, addr.sin_addr=%s addr.sin_port=%d",
                           acc, inet_ntoa(addr.sin_addr),
                           ntohs(addr.sin_port));
                    break;
                }
                dbglog("accept=%d, addr.sin_addr=%s addr.sin_port=%d", acc,
                       inet_ntoa(addr.sin_addr), ntohs(addr.sin_port));
                /* スレッド生成 */
                retval = pthread_create(&tid, NULL,
                                        server_proc, (void *)acc);
                if (retval) { /* エラー(非0) */
                    outlog("pthread_create=%lu", tid);
                    close_sock(&acc); /* アクセプトクローズ */
                    continue;
                }
                dbglog("pthread_create=%lu", tid);
            }
        } else { /* ポーリング */
            continue;
        }
    } while (!sig_handled);
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
    ssize_t slen = 0;                 /* 送信するバイト数 */
    struct header hd;                 /* ヘッダ構造体 */
    int retval = 0;                   /* 戻り値 */
    int acc = -1;                     /* アクセプト */
    struct server_data *sdata = NULL; /* 送信データ構造体 */
    uchar *expr = NULL;               /* 受信データ */
    calcinfo *tsd = NULL;             /* calc情報構造体 */

    dbglog("start");

    /* スレッドデタッチ */
    pthread_detach(pthread_self());

    /* 引数の取得 */
    acc = (int)arg;

    do {
        /* ヘッダ受信 */
        length = sizeof(struct header);
        (void)memset(&hd, 0, length);
        retval = recv_data(acc, &hd, &length);
        if (retval < 0) /* エラー */
            break;
        dbglog("recv_data=%d, hd=%p, length=%zu, hd.length=%zu",
               retval, &hd, length, hd.length);
        if (g_gflag)
            outdump(&hd, length, "hd=%p, length=%zu", &hd, length);
        stddump(&hd, length, "hd=%p, length=%zu", &hd, length);

        length = hd.length; /* データ長を保持 */

        /* データ受信 */
        expr = recv_data_new(acc, &length);
        if (!expr) { /* エラー */
            if (length < 0)
                break;
            else
                continue;
        }

        dbglog("expr=%p, length=%zu", expr, length);

        if (g_gflag)
            outdump(expr, length, "expr%p, length=%zu", expr, length);
        stddump(expr, length, "expr=%p, length=%zu", expr, length);

        if (!strcmp((char *)expr, "exit") ||
            !strcmp((char *)expr, "quit")) {
            memfree((void **)&expr, NULL);
            break;
        }

        /* サーバ処理 */
        tsd = init_calc(expr, g_digit);
        if (!tsd) { /* エラー */
            outlog("tsd=%p", tsd);
            memfree((void **)&expr, NULL);
            break;
        }

        tsd->result = answer(tsd);
        if (!tsd->result) { /* エラー */
            outlog("result=%p", tsd->result);
            memfree((void **)&expr, NULL);
            break;
        }
        dbglog("result=%s", tsd->result);

        length = strlen((char *)tsd->result) + 1; /* 文字列長保持 */
        dbgdump(tsd->result, length,
                "result=%p, length=%zu", tsd->result, length);

        /* データ送信 */
        slen = set_server_data(&sdata, tsd->result, length);
        if (!sdata) {
            destroy_calc(tsd);
            memfree((void **)&expr, NULL);
            break;
        }
        dbglog("slen=%zd", slen);

        if (g_gflag)
            outdump(sdata, length, "sdata=%p, length=%zd", sdata, slen);
        stddump(sdata, length, "sdata=%p, length=%zd", sdata, slen);

        retval = send_data(acc, sdata, (size_t *)&slen);
        if (retval < 0) { /* エラー */
            destroy_calc(tsd);
            memfree((void **)&expr, (void **)&sdata, NULL);
            break;
        }
        dbglog("send_data%d, sdata=%p, length=%zu", retval, sdata, length);

        destroy_calc(tsd);
        memfree((void **)&expr, (void **)&sdata, NULL);

    } while (!sig_handled);

    close_sock(&acc);

    return NULL;
}

