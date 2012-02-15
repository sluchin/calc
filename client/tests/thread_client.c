/**
 * @file  client/tests/thread_client.c
 * @brief サーバストレステスト
 *
 * クライアント送信をスレッド化する.
 *
 * オプション
 *  -i, --ipaddress  IPアドレス指定\n
 *  -p, --port       ポート番号指定\n
 *  -t, --threads    スレッド数設定\n
 *  -h, --help       ヘルプ表示\n
 *  -V, --version    バージョン情報表示\n
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
#include <getopt.h>  /* getopt_long */
#include <assert.h>  /* assert */

#include "def.h"
#include "log.h"
#include "net.h"
#include "data.h"
#include "memfree.h"
#include "version.h"
#include "client.h"

#define MAX_THREADS 1000 /**< スレッド数 */
#define BUF_SIZE      30 /**< バッファサイズ */

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
static int threads = 0; /**< スレッド数 */

/** オプション情報構造体(ロング) */
static struct option longopts[] = {
    { "ipaddress", required_argument, NULL, 'i' },
    { "port",      required_argument, NULL, 'p' },
    { "threads",   required_argument, NULL, 't' },
    { "help",      no_argument,       NULL, 'h' },
    { "version",   no_argument,       NULL, 'V' },
    { NULL,        0,                 NULL, 0   }
};

/** オプション情報文字列(ショート) */
static const char *shortopts = "p:i:t:hV";

/* 内部関数 */
/** スレッド生成 */
static void create_threads(void);
/** スレッド処理 */
static void *client_thread(void *arg);
/** スレッドクリーンアップハンドラ */
static void thread_cleanup(void *arg);
/** スレッドメモリ解放ハンドラ */
static void thread_memfree(void *arg);
/** オプション引数 */
static void parse_args(int argc, char *argv[]);
/** ヘルプの表示 */
static void print_help(const char *progname);
/** バージョン情報表示 */
static void print_version(const char *progname);
/** getoptエラー表示 */
static void parse_error(const int c, const char *msg);
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

    /* オプション引数 */
    parse_args(argc, argv);

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
    int retval = 0;                 /* 戻り値 */
    thread_data *dt = NULL;         /* 送信データ構造体 */
    pthread_t tid[threads];         /* スレッドID */
    void *thread_ret = NULL;        /* スレッド戻り値 */
    char tmp[48];                   /* 一時バッファ */
    char expr[threads][BUF_SIZE];   /* 式 */
    char answer[threads][BUF_SIZE]; /* 答え */

    /* データ作成 */
    int i;
    for (i = 0; i < threads; i++) {
        (void)memset(tmp, 0, sizeof(tmp));
        (void)memset(expr[i], 0, sizeof(expr[i]));
        (void)snprintf(tmp, sizeof(tmp), "%d", i);
        (void)strncpy(expr[i], "1+", sizeof(expr[i]) - 1);
        (void)strncat(expr[i], tmp, sizeof(expr[i]) - strlen(expr[i]) - 1);
        (void)memset(answer[i], 0, sizeof(answer[i]));
        (void)snprintf(answer[i], sizeof(answer[i]), "%d", (1 + i));
    }

    (void)memset(tid, 0, sizeof(tid));
    int j;
    for (j = 0; j < threads; j++) {
        dt = (thread_data *)malloc(sizeof(thread_data));
        if (!dt) {
            outstd("malloc: size=%zu", sizeof(thread_data));
            break;
        }
        (void)memset(dt, 0, sizeof(thread_data));
        (void)memcpy(dt->expr, expr[j], sizeof(dt->expr));
        (void)memcpy(dt->expected, answer[j], sizeof(dt->expected));
        retval = pthread_create(&tid[j], NULL, client_thread, dt);
        if (retval) {
            outstd("pthread_create=%lu, j=%d", (ulong)tid[j], j);
            memfree((void **)&dt, NULL);
            break;
        }
        stdlog("pthread_create: tid=%lu", (ulong)tid[j]);
    }

    int k;
    for (k = 0; k < threads; k++) {
        if (tid[k]) {
            dbglog("tid=%lu", (ulong)tid[k]);
            retval = pthread_join(tid[k], &thread_ret);
            if (retval) {
                outstd("pthread_join=%lu, k=%jd", (ulong)tid[k], k);
                continue;
            }
            if (thread_ret)
                outstd("thread error=%ld", (long)thread_ret);
            stdlog("pthread_join=%ld: tid=%lu",
                   (long)thread_ret, (ulong)tid[k]);
            assert(0 == thread_ret);
        }
    }
    (void)fprintf(stderr, "threads: %d\n", j);
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

    /* コネクト */
    dt->sock = connect_sock();
    if (dt->sock < 0) {
        outstd("Connect error");
        pthread_exit((void *)EX_CONNECT_ERR);
    }

    pthread_cleanup_push(thread_cleanup, &dt);

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

    stdlog("%s", dt->answer);

    retval = strcmp((char *)dt->expected, (char *)dt->answer);
    stdlog("strcmp=%d", retval);
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
    return (void *)EX_SUCCESS;
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
 * オプション引数
 *
 * オプション引数ごとに処理を分岐する.
 * @param[in] argc 引数の数
 * @param[in] argv コマンド引数・オプション引数
 * @return なし
 */
static void
parse_args(int argc, char *argv[])
{
    int opt = 0;         /* オプション */
    const int base = 10; /* 基数 */

    dbglog("start");

    /* デフォルトのポート番号を設定 */
    if (set_port_string(DEFAULT_PORTNO) < 0)
        exit(EXIT_FAILURE);

    /* デフォルトのIPアドレスを設定 */
    if (set_host_string(DEFAULT_IPADDR) < 0)
        exit(EXIT_FAILURE);

    /* デフォルトのスレッド数を設定 */
    threads = MAX_THREADS;

    while ((opt = getopt_long(argc, argv, shortopts, longopts, NULL)) != EOF) {
        dbglog("opt=%c, optarg=%s", opt, optarg);
        switch (opt) {
        case 'i': /* IPアドレス指定 */
            if (set_host_string(optarg) < 0) {
                (void)fprintf(stderr, "Hostname string length %d",
                              (HOST_SIZE - 1));
                exit(EXIT_FAILURE);
            }
            break;
        case 'p': /* ポート番号指定 */
            if (set_port_string(optarg) < 0) {
                (void)fprintf(stderr, "Portno string length %d",
                              (PORT_SIZE - 1));
                exit(EXIT_FAILURE);
            }
            break;
        case 't': /* スレッド数設定 */
            threads = (int)strtol(optarg, NULL, base);
            break;
        case 'h': /* ヘルプ表示 */
            print_help(get_progname());
            exit(EXIT_SUCCESS);
        case 'V': /* バージョン情報表示 */
            print_version(get_progname());
            exit(EXIT_SUCCESS);
        case '?':
        case ':':
            parse_error(opt, NULL);
            exit(EXIT_FAILURE);
        default:
            parse_error(opt, "internal error");
            exit(EXIT_FAILURE);
        }
    }
    if (optind < argc) {
        (void)printf("non-option ARGV-elements: ");
        while (optind < argc)
            (void)printf("%s ", argv[optind++]);
        (void)printf("\n");
    }
}

/**
 * ヘルプ表示
 *
 * ヘルプを表示する.
 * @param[in] progname プログラム名
 * @return なし
 */
static void
print_help(const char *progname)
{
    (void)fprintf(stderr, "Usage: %s [OPTION]...\n", progname);
    (void)fprintf(stderr, "  -i, --ipaddress        %s%s%s",
                  "set ip address or host name (default: ",
                  DEFAULT_IPADDR, ")\n");
    (void)fprintf(stderr, "  -p, --port             %s%s%s",
                  "set port number or service name (default: ",
                  DEFAULT_PORTNO, ")\n");
    (void)fprintf(stderr, "  -t, --threads          %s",
                  "threads count\n");
    (void)fprintf(stderr, "  -t, --time             %s",
                  "print time\n");
    (void)fprintf(stderr, "  -h, --help             %s",
                  "display this help and exit\n");
    (void)fprintf(stderr, "  -V, --version          %s",
                  "output version information and exit\n");
}

/**
 * バージョン情報表示
 *
 * バージョン情報を表示する.
 * @param[in] progname プログラム名
 * @return なし
 */
static void
print_version(const char *progname)
{
    (void)fprintf(stderr, "%s %s\n", progname, VERSION);
}

/**
 * getopt エラー表示
 *
 * getopt が異常な動作をした場合, エラーを表示する.
 * @param[in] c オプション引数
 * @param[in] msg メッセージ文字列
 * @return なし
 */
static void
parse_error(const int c, const char *msg)
{
    if (msg)
        (void)fprintf(stderr, "getopt[%d]: %s\n", c, msg);
    (void)fprintf(stderr, "Try `getopt --help' for more information\n");
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

