/**
 * @file  client/client.c
 * @brief ソケット送受信
 *
 * @author higashi
 * @date 2010-06-24 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
 */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <stdio.h>      /* FILE */
#include <stdlib.h>     /* atexit */
#include <string.h>     /* memcpy memset */
#include <sys/socket.h> /* socket connect */
#include <sys/types.h>  /* socket etc... */
#include <arpa/inet.h>  /* ntohl*/
#include <errno.h>      /* errno */
#include <unistd.h>     /* STDIN_FILENO */
#ifdef _USE_SELECT
#  include <sys/select.h> /* pselect */
#else
#  define _GNU_SOURCE
#  define __USE_GNU
#  include <poll.h> /* ppoll */
#endif

#include "timer.h"
#include "readline.h"
#include "log.h"
#include "data.h"
#include "net.h"
#include "memfree.h"
#include "client.h"

#ifndef _USE_SELECT
/** ポーリング */
enum {
    STDIN_POLL, /**< 標準入力 */
    SOCK_POLL,  /**< ソケット */
    MAX_POLL    /**< ポーリング数 */
};
#endif /* _USE_SELECT */

/* 外部変数 */
volatile sig_atomic_t g_sig_handled = 0; /**< シグナル */
bool g_gflag = false;                    /**< gオプションフラグ */
bool g_tflag = false;                    /**< tオプションフラグ */

/* 内部変数 */
static char hostname[HOST_SIZE];         /**< ホスト名 */
static char portno[PORT_SIZE];           /**< ポート番号 */
static unsigned int start_time = 0U;     /**< タイマ開始 */
static struct client_data *sdata = NULL; /**< 送信データ構造体 */
static unsigned char *expr = NULL;       /**< 入力バッファ */
static unsigned char *answer = NULL;     /**< 受信データ */

/* 内部関数 */
static st_client send_sock(int sock);
static st_client read_sock(int sock);
static st_client drain_replies(int sock, int *pending, st_client status);
static sigset_t get_sigmask(void);
static void exit_memfree(void);

/**
 * ポート番号文字列設定
 *
 * @param[in] port ポート番号
 * @retval EX_NG エラー
 */
int
set_port_string(const char *port)
{
    if (sizeof(portno) <= strlen(port)) {
        outlog("port: length=%zu", strlen(port));
        return EX_NG;
    }
    (void)memset(portno, 0, sizeof(portno));
    (void)snprintf(portno, sizeof(portno), "%s", port);
    return EX_OK;
}

/**
 * @brief ホスト名文字列設定
 *
 * @param[in] host ホスト名
 * @retval EX_NG エラー
 */
int
set_host_string(const char *host)
{
    if (sizeof(hostname) <= strlen(host)) {
        outlog("host: length=%zu", strlen(host));
        return EX_NG;
    }
    (void)memset(hostname, 0, sizeof(hostname));
    (void)snprintf(hostname, sizeof(hostname), "%s", host);
    return EX_OK;
}

/**
 * @brief ソケット接続
 *
 * @return ソケット
 */
int
connect_sock(void)
{
    struct sockaddr_in server; /* ソケットアドレス情報構造体 */
    int sock = -1;             /* ソケット */
    int retval = 0;            /* 戻り値 */

    dbglog("start");

    /* 初期化 */
    (void)memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;

    retval = set_hostname(&server, hostname);
    if (retval < 0)
        return EX_NG;
    retval = set_port(&server, portno);
    if (retval < 0)
        return EX_NG;

    /* ソケット生成 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        outlog("sock=%d", sock);
        return EX_NG;
    }

    /* コネクト */
    /* sockaddr_in を sockaddr として渡すのは, ソケット API の使い方 (glibc の transparent union への
     * キャストが strict-aliasing の誤検知になる) */
#if defined(__GNUC__) && __GNUC__ >= 4
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif
    retval = connect(sock, (struct sockaddr *)&server, sizeof(struct sockaddr_in));
#if defined(__GNUC__) && __GNUC__ >= 4
#  pragma GCC diagnostic pop
#endif
    if (retval < 0) {
        outlog("connect=%d, sock=%d", retval, sock);
        /* ソケットクローズ */
        close_sock(&sock);
        return EX_NG;
    }
    return sock;
}

/**
 * @brief ソケット送受信
 *
 * @param[in] sock ソケット
 * @return ステータス (EX_SUCCESS: 正常, EX_FAILURE: 異常, EX_QUIT: quit または exit,
 *         EX_SEND_ERR: 送信エラー, EX_RECV_ERR: 受信エラー, EX_SIGNAL: シグナル受信)
 */
st_client
client_loop(int sock)
{
    int ready = 0;                 /* select戻り値 */
    int retval = 0;                /* 戻り値 */
    struct timespec timeout;       /* タイムアウト値 */
    sigset_t sigmask;              /* シグナルマスク */
    st_client status = EX_SUCCESS; /* ステータス */
    int pending = 0;               /* 送信済みで未受信の答えの数 */
#ifdef _USE_SELECT
    fd_set fds, rfds; /* selectマスク */
#else
    struct pollfd targets[MAX_POLL]; /* poll */
#endif /* _USE_SELECT */

    dbglog("start: sock=%d", sock);

    retval = atexit(exit_memfree);
    if (retval != 0) {
        outlog("atexit=%d", retval);
        return EX_FAILURE;
    }

#ifdef _USE_SELECT
    /* マスクの設定 */
    FD_ZERO(&fds);              /* 初期化 */
    FD_SET(sock, &fds);         /* ソケットをマスク */
    FD_SET(STDIN_FILENO, &fds); /* 標準入力をマスク */
#endif                          /* _USE_SELECT */

    /* シグナルマスクの取得 */
    sigmask = get_sigmask();

    /* タイムアウト値初期化 */
    (void)memset(&timeout, 0, sizeof(struct timespec));
    timeout.tv_sec = 1; /* 1秒 */
    timeout.tv_nsec = 0;

    do {
#ifdef _USE_SELECT
        (void)memcpy(&rfds, &fds, sizeof(fd_set)); /* マスクコピー */
        ready = pselect(sock + 1, &rfds, NULL, NULL, &timeout, &sigmask);
#else
        targets[STDIN_POLL].fd = STDIN_FILENO;
        targets[STDIN_POLL].events = POLLIN;
        targets[SOCK_POLL].fd = sock;
        targets[SOCK_POLL].events = POLLIN;
        ready = ppoll(targets, MAX_POLL, &timeout, &sigmask);
#endif /* _USE_SELECT */
        if (ready < 0) {
            if (errno == EINTR) /* 割り込み */
                break;
            /* selectエラー */
            outlog("select=%d", ready);
            return EX_FAILURE;
        } else if (ready > 0) {
#ifdef _USE_SELECT
            if (FD_ISSET(STDIN_FILENO, &rfds) != 0) {
                /* 標準入力レディ */
                status = send_sock(sock);
                if (status == EX_EMPTY)
                    continue;
                if (status == EX_SUCCESS)
                    pending++;
                else
                    return drain_replies(sock, &pending, status);
            }
            if (FD_ISSET(sock, &rfds) != 0) {
                /* ソケットレディ */
                status = read_sock(sock);
                if (status != EX_SUCCESS)
                    return status;
                if (pending > 0)
                    pending--;
            }
#else
            if ((targets[STDIN_POLL].revents & POLLIN) != 0) {
                /* 標準入力レディ */
                status = send_sock(sock);
                if (status == EX_EMPTY)
                    continue;
                if (status == EX_SUCCESS)
                    pending++;
                else
                    return drain_replies(sock, &pending, status);
            }
            if ((targets[SOCK_POLL].revents & POLLIN) != 0) {
                /* ソケットレディ */
                status = read_sock(sock);
                if (status != EX_SUCCESS)
                    return status;
                if (pending > 0)
                    pending--;
            }
#endif           /* _USE_SELECT */
        } else { /* タイムアウト */
            continue;
        }
    } while (g_sig_handled == 0);

    return EX_SIGNAL;
}

/**
 * 送信済みで未受信の答えを受信
 * 標準入力から続けて入力する (パイプやファイル) と, 答えを受信する前に quit や入力の
 * 終わりになるので終了する前に, 未受信の答えを全て受信して, 出力する.
 * 送信に失敗して終了する場合は, サーバとの通信ができないので受信しない.
 *
 * @param[in] sock ソケット
 * @param[in,out] pending 送信済みで, 未受信の答えの数
 * @param[in] status 終了する理由のステータス
 * @return status, または受信に失敗したときは, そのステータス
 */
static st_client
drain_replies(int sock, int *pending, st_client status)
{
    st_client st = EX_SUCCESS; /* 受信のステータス */

    /* quit と, 入力の終わり (EX_ALLOC_ERR) のときだけ, 受信する */
    if ((status != EX_QUIT) && (status != EX_ALLOC_ERR))
        return status;

    while (*pending > 0) {
        st = read_sock(sock);
        if (st != EX_SUCCESS)
            return st;
        (*pending)--;
    }
    return status;
}

/**
 * ソケット送信
 *
 * @param[in] sock ソケット
 * @return ステータス
 */
static st_client
send_sock(int sock)
{
    int retval = 0;     /* 戻り値 */
    size_t length = 0U; /* 長さ */
    ssize_t slen = 0L;  /* 送信するバイト数 */

    expr = _readline(stdin);
    if (expr == NULL)
        return EX_ALLOC_ERR;

    if (*expr == '\0') { /* 文字列長ゼロ */
        memfree(&expr, NULL);
        return EX_EMPTY;
    }

    if ((strcmp((char *)expr, "quit") == 0) || (strcmp((char *)expr, "exit") == 0))
        return EX_QUIT;

    length = strlen((char *)expr) + 1U;
    dbgdump(expr, length, "stdin: expr=%zu", length);

    if (g_tflag)
        start_timer(&start_time);

    /* データ設定 */
    slen = set_client_data(&sdata, expr, length);
    if (slen < 0L) /* メモリ確保できない */
        return EX_ALLOC_ERR;
    dbglog("slen=%zd", slen);

    if (g_gflag)
        outdump(sdata, (size_t)slen, "send: sdata=%p, length=%zd", (const void *)sdata, slen);
    stddump(sdata, (size_t)slen, "send: sdata=%p, length=%zd", (const void *)sdata, slen);

    /* データ送信 */
    retval = send_data(sock, sdata, (size_t *)&slen);
    if (retval < 0) /* エラー */
        return EX_SEND_ERR;

    memfree(&expr, &sdata, NULL);

    return EX_SUCCESS;
}

/**
 * ソケット受信
 *
 * @param[in] sock ソケット
 * @return ステータス
 */
static st_client
read_sock(int sock)
{
    int retval = 0;     /* 戻り値 */
    size_t length = 0U; /* 送信または受信する長さ */
    struct header hd;   /* ヘッダ */

    dbglog("start");

    /* ヘッダ受信 */
    length = sizeof(struct header);
    (void)memset(&hd, 0, length);
    retval = recv_data(sock, &hd, &length);
    if (retval < 0) /* エラー */
        return EX_RECV_ERR;
    dbglog("recv_data: hd=%p, length=%zu", (const void *)&hd, length);

    if (g_gflag)
        outdump(&hd, length, "recv: hd=%p, length=%zu", (const void *)&hd, length);
    stddump(&hd, length, "recv: hd=%p, length=%zu", (const void *)&hd, length);

    length = (size_t)ntohl((uint32_t)hd.length); /* データ長を保持 */
    if (length > MAX_DATA_LENGTH) {              /* 巨大なメモリを確保させない */
        outlog("data length=%zu, max=%u", length, MAX_DATA_LENGTH);
        return EX_RECV_ERR;
    }

    /* データ受信 */
    answer = (unsigned char *)recv_data_new(sock, &length);
    if (answer == NULL) /* メモリ確保できない */
        return EX_ALLOC_ERR;
    if (length == 0U) /* 受信エラー */
        return EX_RECV_ERR;
    dbglog("answer=%p, length=%zu", answer, length);

    if (g_gflag)
        outdump(answer, length, "recv: answer=%p, length=%zu", answer, length);
    stddump(answer, length, "recv: answer=%p, length=%zu", answer, length);

    if (g_tflag) {
        unsigned int client_time = stop_timer(&start_time);
        print_timer(client_time);
    }

    retval = fprintf(stdout, "%s\n", answer);
    if (retval < 0)
        outlog("fprintf=%d", retval);

    memfree(&answer, NULL);
    return EX_SUCCESS;
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
    int retval = 0;   /* 戻り値 */

    /* 初期化 */
    retval = sigemptyset(&sigmask);
    if (retval < 0)
        outlog("sigemptyset");
    /* シグナル全て */
    retval = sigfillset(&sigmask);
    if (retval < 0)
        outlog("sigfillset");
    /* SIGINT除く*/
    retval = sigdelset(&sigmask, SIGINT);
    if (retval < 0)
        outlog("sigdelset");
    dbglog("sigmask=%p", (const void *)&sigmask);

    return sigmask;
}

/**
 * atexit登録関数
 */
static void
exit_memfree(void)
{
    memfree(&expr, &sdata, &answer, NULL);
}

#ifdef UNITTEST
/**
 * @brief 単体テスト用の関数構造体の初期化 (内部関数をテストから呼べるようにする)
 *
 * @param[out] client 関数構造体
 */
void
test_init_client(testclient *client)
{
    client->send_sock = send_sock;
    client->read_sock = read_sock;
}
#endif /* UNITTEST */
