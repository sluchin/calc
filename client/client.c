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
#include <stdio.h>        /* FILE */
#include <stdlib.h>       /* strtol */
#include <string.h>       /* memcpy memset */
#include <ctype.h>        /* isdigit */
#include <sys/socket.h>   /* socket */
#include <arpa/inet.h>    /* inet_addr */
#include <netinet/in.h>   /* struct in_addr */
#include <errno.h>        /* errno */
#include <unistd.h>       /* close */
#ifdef _USE_SELECT
#  include <sys/select.h> /* pselect */
#else
#  define _GNU_SOURCE
#  define __USE_GNU
#  include <poll.h>       /* ppoll */
#endif

#include "readline.h"
#include "log.h"
#include "data.h"
#include "net.h"
#include "memfree.h"
#include "option.h"
#include "timer.h"
#include "fileio.h"
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
static uint start_time = 0;              /**< タイマ開始 */
static struct client_data *sdata = NULL; /**< 送信データ構造体 */
static uchar *expr = NULL;               /**< 入力バッファ */
static uchar *answer = NULL;             /**< 受信データ */

/* 内部関数 */
/** ソケット送信 */
static st_client send_sock(int sock);
/** ソケット受信 */
static st_client read_sock(int sock);
/** atexit登録関数 */
static void exit_memfree(void);

/**
 * ソケット接続
 *
 * @param[in] host ホスト名またはIPアドレス　
 * @param[in] port ポート番号
 * @return ソケット
 */
int
connect_sock(const char *host, const char *port)
{
    struct sockaddr_in server; /* ソケットアドレス情報構造体 */
    int sock = -1;             /* ソケット */
    int retval = 0;            /* 戻り値 */

    dbglog("start");

    /* 初期化 */
    (void)memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;

    if (set_hostname(&server, host) < 0)
        return EX_NG;
    if (set_port(&server, port) < 0)
        return EX_NG;

    /* ソケット生成 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        outlog("sock=%d", sock);
        return EX_NG;
    }

    /* コネクト */
    retval = connect(sock, (struct sockaddr *)&server,
                     sizeof(struct sockaddr_in));
    if (retval < 0) {
        outlog("connect=%d, sock=%d", retval, sock);
        /* ソケットクローズ */
        close_sock(&sock);
        return EX_NG;
    }
    return sock;
}

/**
 * ソケット送受信
 *
 * @param[in] sock ソケット
 * @return なし
 */
st_client
client_loop(int sock)
{
    int ready = 0;                 /* select戻り値 */
    struct timespec timeout;       /* タイムアウト値 */
    sigset_t sigmask;              /* シグナルマスク */
    st_client status = EX_SUCCESS; /* ステータス */
#ifdef _USE_SELECT
    fd_set fds, rfds;        /* selectマスク */
#else
    struct pollfd targets[MAX_POLL]; /* poll */
#endif /* _USE_SELECT */

    dbglog("start: sock=%d", sock);

    if (atexit(exit_memfree)) {
        outlog("atexit");
        return EX_FAILURE;
    }

#ifdef _USE_SELECT
    /* マスクの設定 */
    FD_ZERO(&fds);              /* 初期化 */
    FD_SET(sock, &fds);         /* ソケットをマスク */
    FD_SET(STDIN_FILENO, &fds); /* 標準入力をマスク */
#endif /* _USE_SELECT */

    /* シグナルマスクの設定 */
    if (sigemptyset(&sigmask) < 0) /* 初期化 */
        outlog("sigemptyset=0x%x", sigmask);
    if (sigfillset(&sigmask) < 0) /* シグナル全て */
        outlog("sigfillset=0x%x", sigmask);
    if (sigdelset(&sigmask, SIGINT) < 0) /* SIGINT除く*/
        outlog("sigdelset=0x%x", sigmask);
    dbglog("sigmask=0x%x", sigmask);

    /* タイムアウト値初期化 */
    (void)memset(&timeout, 0, sizeof(struct timespec));
    timeout.tv_sec = 1; /* 1秒 */
    timeout.tv_nsec = 0;

    do {
#ifdef _USE_SELECT
        (void)memcpy(&rfds, &fds, sizeof(fd_set)); /* マスクコピー */
        ready = pselect(sock + 1, &rfds,
                        NULL, NULL, &timeout, &sigmask);
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
        } else if (ready) {
#ifdef _USE_SELECT
            if (FD_ISSET(STDIN_FILENO, &fds)) {
                /* 標準入力レディ */
                status = send_sock(sock);
                if (status == EX_EMPTY)
                    continue;
                if (status)
                    return status;
            }
            if (FD_ISSET(sock, &fds)) {
                /* ソケットレディ */
                status = read_sock(sock);
                if (status)
                    return status;
            }
#else
            if (targets[STDIN_POLL].revents & POLLIN) {
                /* 標準入力レディ */
                status = send_sock(sock);
                if (status == EX_EMPTY)
                    continue;
                if (status)
                    return status;
            }
            if (targets[SOCK_POLL].revents & POLLIN) {
                /* ソケットレディ */
                status = read_sock(sock);
                if (status)
                    return status;
            }
#endif /* _USE_SELECT */
        } else { /* タイムアウト */
            continue;
        }
    } while (!g_sig_handled);

    return EX_SIGNAL;
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
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* 長さ */
    ssize_t slen = 0;  /* 送信するバイト数 */

    expr = _readline(stdin);
    if (!expr)
        return EX_ALLOC_ERR;

    if (*expr == '\0') { /* 文字列長ゼロ */
        memfree((void **)&expr, NULL);
        return EX_EMPTY;
    }

    if (!strcmp((char *)expr, "quit") ||
        !strcmp((char *)expr, "exit"))
        return EX_QUIT;

    length = strlen((char *)expr) + 1;
    dbgdump(expr, length, "stdin: expr=%zu", length);

    if (g_tflag)
        start_timer(&start_time);

    /* データ設定 */
    slen = set_client_data(&sdata, expr, length);
    if (slen < 0) /* メモリ確保できない */
        return EX_ALLOC_ERR;
    dbglog("slen=%zd", slen);

    if (g_gflag)
        outdump(sdata, slen, "send: sdata=%p, length=%zd", sdata, slen);
    stddump(sdata, slen, "send: sdata=%p, length=%zd", sdata, slen);

    /* データ送信 */
    retval = send_data(sock, sdata, (size_t *)&slen);
    if (retval < 0) /* エラー */
        return EX_SEND_ERR;

    memfree((void **)&expr, (void **)&sdata, NULL);

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
    int retval = 0;       /* 戻り値 */
    size_t length = 0;    /* 送信または受信する長さ */
    struct header hd;     /* ヘッダ */

    dbglog("start");

    /* ヘッダ受信 */
    length = sizeof(struct header);
    (void)memset(&hd, 0, length);
    retval = recv_data(sock, &hd, &length);
    if (retval < 0) /* エラー */
        return EX_RECV_ERR;
    dbglog("recv_data: hd=%p, length=%zu", &hd, length);

    if (g_gflag)
        outdump(&hd, length, "recv: hd=%p, length=%zu", &hd, length);
    stddump(&hd, length, "recv: hd=%p, length=%zu", &hd, length);

    length = hd.length; /* データ長を保持 */

    /* データ受信 */
    answer = (uchar *)recv_data_new(sock, &length);
    if (!answer) /* メモリ確保できない */
        return EX_ALLOC_ERR;
    if (!length) /* 受信エラー */
        return EX_RECV_ERR;
    dbglog("answer=%p, length=%zu", answer, length);

    if (g_gflag)
        outdump(answer, length,
                "recv: answer=%p, length=%zu", answer, length);
    stddump(answer, length,
            "recv: answer=%p, length=%zu", answer, length);

    if (g_tflag) {
        uint client_time = stop_timer(&start_time);
        print_timer(client_time);
    }

    retval = fprintf(stdout, "%s\n", answer);
    if (retval < 0)
        outlog("fprintf=%d", retval);

    memfree((void **)&answer, NULL);
    return EX_SUCCESS;
}

/**
 * atexit登録関数
 *
 * @return なし
 */
static void
exit_memfree(void)
{
    memfree((void **)&expr,
            (void **)&sdata,
            (void **)&answer, NULL);
}

#ifdef UNITTEST
void
test_init_client(testclient *client)
{
    client->send_sock = send_sock;
    client->read_sock = read_sock;
}
#endif /* UNITTEST */

