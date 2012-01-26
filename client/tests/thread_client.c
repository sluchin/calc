/**
 * @file  client/tests/thread_client.c
 * @brief クライアントスレッドテスト
 *
 * @author higashi
 * @date 2012-01-25 higashi 新規作成
 * @version \$Id
 *
 * Copyright (C) 2010-2012 Tetsuya Higashi. All Rights Reserved.
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

#include <stdio.h>   /* FILE stderr */
#include <stdlib.h>  /* exit EXIT_SUCCESS */
#include <string.h>  /* memset strlen */
#include <signal.h>  /* signal */
#include <pthread.h> /* pthread */
#include <assert.h>  /* assert */

#include "log.h"
#include "net.h"
#include "data.h"
#include "memfree.h"
#include "option.h"
#include "client.h"

#define MAX_THREADS 1000 /**< スレッド数 */
#define BUF_SIZE     256 /**< バッファサイズ */

/** スレッドデータ構造体 */
struct _thread_data {
    uchar expr[BUF_SIZE];      /**< 式 */
    struct client_data *sdata; /**< 送信データ */
    uchar *answer;             /**< 受信データ */
    uchar *expected[BUF_SIZE]; /**< 期待する文字列 */
    size_t len;                /**< 送信データ長 */
    int sock;                  /**< ソケット */
};
typedef struct _thread_data thread_data;

/* 内部変数 */
char expr[MAX_THREADS][BUF_SIZE];   /**< 式 */
char answer[MAX_THREADS][BUF_SIZE]; /**< 答え */
char *hostname = "127.0.0.1";       /**< ホスト名 */
char *port = "12345";               /**< ポート番号 */

/* 内部関数 */
/** スレッド生成 */
static void create_threads(void);
/** スレッド処理 */
static void *client_thread(void *arg);
/** スレッドクリーンアップハンドラ */
static void thread_cleanup(void *arg);
/** スレッドメモリ解放ハンドラ */
static void thread_memfree(void *arg);
/** シグナルハンドラ設定 */
static void set_sig_handler(void);

/**
 * main関数
 *
 * @param[in] argc 引数の数
 * @param[in] argv コマンド引数・オプション引数
 * @return ステータス
 */
int main(int argc, char *argv[])
{
    dbglog("start");

    set_progname(argv[0]);

    /* シグナルハンドラ設定 */
    set_sig_handler();

    /* バッファリングしない */
    if (setvbuf(stdin, (char *)NULL, _IONBF, 0))
        outlog("setvbuf: stdin");
    if (setvbuf(stdout, (char *)NULL, _IONBF, 0))
        outlog("setvbuf: stdout");

    /* スレッド生成 */
    create_threads();

    exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

/**
 * スレッド生成
 *
 * @return なし
 */
static void
create_threads(void)
{
    int retval = 0;                   /* 戻り値 */
    thread_data *dt = NULL;           /* 送信データ構造体 */
    pthread_t tid[MAX_THREADS] = {0}; /* スレッドID */
    void *thread_ret = NULL;          /* スレッド戻り値 */
    char tmp[48];                     /* 一時バッファ */

    /* データ作成 */
    int i;
    for (i = 0; i < MAX_THREADS; i++) {
        (void)memset(tmp, 0, sizeof(tmp));
        (void)memset(expr[i], 0, sizeof(expr[i]));
        (void)snprintf(tmp, sizeof(tmp), "%d", i);
        (void)strncpy(expr[i], "1+", sizeof(expr[i]) - 1);
        (void)strncat(expr[i], tmp, sizeof(expr[i]) - strlen(expr[i]) - 1);
        (void)snprintf(answer[i], sizeof(answer[i]), "%d", (1 + i));
    }

    for (i = 0; i < MAX_THREADS; i++) {
        dt = (thread_data *)malloc(sizeof(thread_data));
        if (!dt) {
            outstd("malloc: size=%zu", sizeof(thread_data));
            break;
        }
        (void)memset(dt, 0, sizeof(thread_data));
        (void)memcpy(dt->expr, expr[i], sizeof(dt->expr));
        (void)memcpy(dt->expected, answer[i], sizeof(dt->expected));
        retval = pthread_create(&tid[i], NULL, client_thread, dt);
        if (retval) {
            outstd("pthread_create=%lu, i=%d", (ulong)tid[i], i);
            memfree((void **)&dt, NULL);
            break;
        }
        outstd("pthread_create: tid=%lu", (ulong)tid[i]);
    }

    for (i = 0; i < MAX_THREADS; i++) {
        if (tid[i]) {
            dbglog("tid=%lu", (ulong)tid[i]);
            retval = pthread_join(tid[i], &thread_ret);
            if (retval) {
                outstd("pthread_join=%lu, i=%jd", (ulong)tid[i], i);
                continue;
            }
            if (thread_ret) {
                outstd("thread error=%ld", (long)thread_ret);
                break;
            }
            outstd("pthread_join=%ld: tid=%lu",
                   (long)thread_ret, (ulong)tid[i]);
            assert(0 == thread_ret);
        }
    }
}

/**
 * スレッド処理
 *
 * @return なし
 */
static void *
client_thread(void *arg)
{
    /* スレッドデータ */
    thread_data *dt = (thread_data *)arg;
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* 長さ */
    ssize_t slen = 0;  /* 送信するバイト数 */
    struct header hd;  /* ヘッダ */

    pthread_cleanup_push(thread_cleanup, &dt);

    if (set_host_string(hostname) < 0) {
        outstd("set_host_string");
        pthread_exit((void *)EX_FAILURE);
    }
    if (set_port_string(port) < 0) {
        outstd("set_port_string");
        pthread_exit((void *)EX_FAILURE);
    }
    dt->sock = connect_sock();
    if (dt->sock < 0) {
        outstd("Connect error\n");
        pthread_exit((void *)EX_CONNECT_ERR);
    }

    length = strlen((char *)dt->expr) + 1;

    /* データ設定 */
    slen = set_client_data(&dt->sdata,  dt->expr, length);
    if (slen < 0) /* メモリ確保できない */
        pthread_exit((void *)EX_ALLOC_ERR);

    pthread_cleanup_push(thread_memfree, &dt->sdata);

    /* データ送信 */
    retval = send_data(dt->sock, dt->sdata, (size_t *)&slen);

    /* ヘッダ受信 */
    length = sizeof(struct header);
    (void)memset(&hd, 0, length);
    retval = recv_data(dt->sock, &hd, &length);
    if (retval < 0) /* エラー */
        pthread_exit((void *)EX_RECV_ERR);
    dbglog("recv_data: hd=%p, length=%zu", &hd, length);

    length = (size_t)ntohl((uint32_t)hd.length); /* データ長を保持 */

    /* データ受信 */
    dt->answer = (uchar *)recv_data_new(dt->sock, &length);
    if (!dt->answer) /* メモリ確保できない */
        pthread_exit((void *)EX_ALLOC_ERR);

    pthread_cleanup_push(thread_memfree, &dt->answer);

    if (!length) /* 受信エラー */
        pthread_exit((void *)EX_RECV_ERR);
    dbglog("answer=%p, length=%zu", dt->answer, length);

    outstd("%s", dt->answer);

    retval = strcmp((char *)dt->expected, (char *)dt->answer);
    outstd("strcmp=%d", retval);
    assert(0 == retval);

    retval = shutdown(dt->sock, SHUT_RDWR);
    if (retval < 0) {
        outstd("shutdown: sock=%d", dt->sock);
        pthread_exit((void *)EXIT_FAILURE);
    }

    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);
    pthread_cleanup_pop(1);

    pthread_exit((void *)EX_SUCCESS);
    return EX_SUCCESS;
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
 * シグナルハンドラ設定
 *
 * @return なし
 */
static void
set_sig_handler(void)
{
    /* シグナル無視 */
    if (signal(SIGINT, SIG_IGN) < 0)
        outlog("SIGINT");
    if (signal(SIGTERM, SIG_IGN) < 0)
        outlog("SIGTERM");
    if (signal(SIGQUIT, SIG_IGN) < 0)
        outlog("SIGQUIT");
    if (signal(SIGHUP, SIG_IGN) < 0)
        outlog("SIGHUP");
    if (signal(SIGALRM, SIG_IGN) < 0)
        outlog("SIGALRM");
}

