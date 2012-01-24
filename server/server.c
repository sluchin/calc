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
#include <arpa/inet.h>  /* ntohl */
#include <errno.h>      /* errno */
#include <unistd.h>     /* close access */
#include <pthread.h>    /* pthread */
#include <signal.h>     /* sig_atomic_t */
#include <errno.h>      /* errno */
#include <sys/select.h> /* select */

#include "def.h"
#include "log.h"
#include "net.h"
#include "memfree.h"
#include "option.h"
#include "server.h"

/* 外部変数 */
volatile sig_atomic_t g_sig_handled = 0; /**< シグナル */
bool g_gflag = false;                    /**< gオプションフラグ */
long g_digit = DEFAULT_DIGIT;            /**< 桁数 */

/* 内部関数 */
/** サーバプロセス */
static void *server_proc(void *arg);
/** スレッドクリーンアップハンドラ */
static void thread_cleanup(void *arg);
/** スレッドメモリ解放ハンドラ */
static void thread_memfree(void *arg);
/** シグナルマスク取得 */
static sigset_t get_sigmask(void);
/** スレッドシグナルマスク設定 */
static void set_thread_sigmask(sigset_t sigmask);

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
    int optval = 0;          /* オプション */
    int sock = -1;           /* ソケット */

    dbglog("start");

    /* 初期化 */
    (void)memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);

    /* ポート番号またはサービス名を設定 */
    if (set_port(&addr, port) < 0)
        return EX_NG;

    /* ソケット生成 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        outlog("sock=%d", sock);
        return EX_NG;
    }

    /* ソケットオプション */
    optval = 1; /* 二値オプション有効 */
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
        outlog("listen=%d, sock=%d", retval, sock);
        goto error_handler;
    }

    return sock;

error_handler:
    close_sock(&sock);
    return EX_NG;
}

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
    pthread_t tid = 0;       /* スレッドID */
    int retval = 0;          /* pthread_create戻り値 */
    fd_set fds, rfds;        /* selectマスク */
    struct timespec timeout; /* タイムアウト値 */
    sigset_t sigmask;        /* シグナルマスク */
    thread_data *dt = NULL;  /* ソケット情報構造体 */

    dbglog("start: sock=%d", sock);

    /* マスクの設定 */
    FD_ZERO(&fds);      /* 初期化 */
    FD_SET(sock, &fds); /* ソケットをマスク */

    /* シグナルマスク取得 */
    sigmask = get_sigmask();

    /* タイムアウト値初期化 */
    (void)memset(&timeout, 0, sizeof(struct timespec));
    /* pselectの場合, constなのでループ前で値を入れる */
    timeout.tv_sec = 1; /* 1秒 */
    timeout.tv_nsec = 0;

    /* ノンブロッキングに設定 */
    if (set_block(sock, NONBLOCK) < 0)
        return;

    do {
        (void)memcpy(&rfds, &fds, sizeof(fd_set)); /* マスクコピー */
        ready = pselect(sock + 1, &rfds,
                        NULL, NULL, &timeout, &sigmask);
        if (ready < 0) {
            if (errno == EINTR) /* 割り込み */
                break;
            outlog("select=%d", ready);
            break;
        } else if (ready) {
            if (FD_ISSET(sock, &rfds)) {

                dt = (thread_data *)malloc(sizeof(thread_data));
                if (!dt) {
                    outlog("malloc: size=%zu", sizeof(thread_data));
                    continue;
                }
                (void)memset(dt, 0, sizeof(thread_data));
                dbglog("dt=%p", dt);

                /* 接続受付 */
                /* addrlenは入出力なのでここで初期化する */
                dt->len = (socklen_t)sizeof(dt->addr);
                dt->sock = accept(sock,
                                  (struct sockaddr *)&dt->addr,
                                  &dt->len);
                if (dt->sock < 0) {
                    outlog("accept: sin_addr=%s sin_port=%d",
                           inet_ntoa(dt->addr.sin_addr),
                           ntohs(dt->addr.sin_port));
                    memfree((void **)&dt, NULL);
                    continue;
                }

                /* スレッド生成 */
                dt->sigmask = sigmask;
                retval = pthread_create(&tid, NULL, server_proc, dt);
                if (retval) { /* エラー(非0) */
                    outlog("pthread_create=%lu", tid);
                    close_sock(&dt->sock); /* アクセプトクローズ */
                    memfree((void **)&dt, NULL);
                    continue;
                }
                dbglog("pthread_create=%lu, accept=%d", tid, dt->sock);

                retval = pthread_detach(tid);
                if (retval) /* エラー(非0) */
                    outlog("pthread_detach: tid=%lu", tid);
            }
        } else { /* タイムアウト */
            continue;
        }
    } while (!g_sig_handled);
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
    thread_data *dt = (thread_data *)arg; /* ソケット情報構造体 */
    int retval = 0;                       /* 戻り値 */
    size_t length = 0;                    /* 長さ */
    ssize_t slen = 0;                     /* 送信するバイト数 */
    struct header hd;                     /* ヘッダ構造体 */
    uchar *expr = NULL;                   /* 受信データ */
    calcinfo tsd;                         /* calc情報構造体 */
    struct server_data *sdata = NULL;     /* 送信データ構造体 */

    dbglog("start: accept=%d sin_addr=%s sin_port=%d, len=%d",
           dt->sock, inet_ntoa(dt->addr.sin_addr),
           ntohs(dt->addr.sin_port), dt->len);
    dbglog("dt=%p", dt);

    if (!dt)
        pthread_exit((void *)EXIT_FAILURE);

    /* シグナルマスクを設定 */
    set_thread_sigmask(dt->sigmask);

    pthread_cleanup_push(thread_cleanup, &dt);
    do {
        /* ヘッダ受信 */
        length = sizeof(struct header);
        (void)memset(&hd, 0, length);
        retval = recv_data(dt->sock, &hd, &length);
        if (retval < 0) /* エラーまたは接続先がシャットダウンされた */
            pthread_exit((void *)EXIT_FAILURE);

        dbglog("recv_data: hd=%p, length=%zu, hd.length=%zu",
               &hd, length, hd.length);

        if (g_gflag)
            outdump(&hd, length, "recv: hd=%p, length=%zu", &hd, length);
        stddump(&hd, length, "recv: hd=%p, length=%zu", &hd, length);

        /* データ受信 */
        length = (size_t)ntohl((uint32_t)hd.length); /* データ長を保持 */
        expr = (uchar *)recv_data_new(dt->sock, &length);
        if (!expr) /* メモリ不足 */
            pthread_exit((void *)EXIT_FAILURE);

        pthread_cleanup_push(thread_memfree, &expr);

        if (!length) /* 受信エラー */
            pthread_exit((void *)EXIT_FAILURE);

        dbglog("expr=%p, length=%zu", expr, length);

        if (g_gflag)
            outdump(expr, length,
                    "recv: expr=%p, length=%zu", expr, length);
        stddump(expr, length,
                "recv: expr=%p, length=%zu", expr, length);

        /* サーバ処理 */
        init_calc(&tsd, expr, g_digit);
        if (!create_answer(&tsd))
            pthread_exit((void *)EXIT_FAILURE);

        pthread_cleanup_push(destroy_answer, &tsd);

        length = strlen((char *)tsd.answer) + 1; /* 文字列長保持 */

        dbgdump(tsd.answer, length,
                "answer=%p, length=%zu", tsd.answer, length);

        /* データ送信 */
        slen = set_server_data(&sdata, tsd.answer, length);
        if (slen < 0) /* メモリ確保できない */
            pthread_exit((void *)EXIT_FAILURE);

        pthread_cleanup_push(thread_memfree, &sdata);
        dbglog("slen=%zd", slen);

        if (g_gflag)
            outdump(sdata, slen,
                    "send: sdata=%p, slen=%zd", sdata, slen);
        stddump(sdata, slen,
                "send: sdata=%p, slen=%zd", sdata, slen);

        retval = send_data(dt->sock, sdata, (size_t *)&slen);
        if (retval < 0) /* エラー */
            pthread_exit((void *)EXIT_FAILURE);

        dbglog("send_data: sdata=%p, slen=%zu", sdata, slen);

        pthread_cleanup_pop(1);
        pthread_cleanup_pop(1);
        pthread_cleanup_pop(1);

    } while (!g_sig_handled);

    pthread_cleanup_pop(1);
    pthread_exit((void *)EXIT_SUCCESS);
    return (void *)EXIT_SUCCESS;
}

/**
 * スレッドクリーンアップハンドラ
 *
 * @param[in] arg ポインタ
 * @return なし
 * @attention 引数にthread_data **型を渡さなければ不正アクセスになる.
 */
static void
thread_cleanup(void *arg)
{
    thread_data **dt = (thread_data **)arg; /* スレッドデータ構造体 */
    dbglog("start: *dt=%p, dt=%p, sock=%d", *dt, dt, (*dt)->sock);
    close_sock(&(*dt)->sock);
    memfree((void **)dt, NULL);
}

/**
 * スレッドメモリ解放ハンドラ
 *
 * @param[in] arg ポインタ
 * @return なし
 * @attention 引数にvoid **型を渡さなければ不正アクセスになる.
 */
static void
thread_memfree(void *arg)
{
    void **ptr = (void **)arg;
    dbglog("start: *ptr=%p, ptr=%p", *ptr, ptr);
    memfree(ptr, NULL);
}

/**
 * シグナルマスク取得
 *
 * @return シグナルマスク
 */
static sigset_t
get_sigmask(void)
{
    sigset_t sigmask; /* シグナルマスク */

    /* 初期化 */
    if (sigemptyset(&sigmask) < 0)
        outlog("sigemptyset=0x%x", sigmask);
    /* シグナル全て */
    if (sigfillset(&sigmask) < 0)
        outlog("sigfillset=0x%x", sigmask);
    /* SIGINT除く */
    if (sigdelset(&sigmask, SIGINT) < 0)
        outlog("sigdelset=0x%x", sigmask);
    /* SIGHUP除く */
    if (sigdelset(&sigmask, SIGHUP) < 0)
        outlog("sigdelset=0x%x", sigmask);
    dbglog("sigmask=0x%x", sigmask);

    return sigmask;
}

/**
 * スレッドシグナルマスク設定
 *
 * @param[in] sigmask シグナルマスク
 * @return なし
 */
static void
set_thread_sigmask(sigset_t sigmask)
{
    dbglog("sigmask=0x%x", sigmask);

    /* シグナル設定 */
    if (pthread_sigmask(SIG_BLOCK, &sigmask, NULL))
        outlog("pthread_sigmask=0x%x", sigmask);

#ifdef _DEBUG
    /* シグナル設定確認 */
    sigset_t newmask;
    if (sigemptyset(&newmask) < 0) /* 初期化 */
        dbglog("sigemptyset=0x%x", newmask);
    if (pthread_sigmask(SIG_SETMASK, NULL, &newmask))
        dbglog("pthread_sigmask");
    dbglog("sigmask=0x%x", newmask);
#endif /* _DEBUG */
}

#ifdef UNITTEST
void
test_init_server(testserver *server)
{
    server->server_proc = server_proc;
}
#endif /* UNITTEST */

