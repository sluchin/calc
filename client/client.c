/**
 * @file  client.c
 * @brief ソケット送受信
 *
 * @sa client.h
 * @author higashi
 * @date 2010-06-24 higashi 新規作成
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

#include <stdio.h>        /* FILE */
#include <stdlib.h>       /* strtol */
#include <string.h>       /* memcpy memset */
#include <ctype.h>        /* isdigit */
#include <sys/socket.h>   /* socket */
#include <arpa/inet.h>    /* inet_addr */
#include <errno.h>        /* errno */
#include <unistd.h>       /* close */
#ifdef _USE_SELECT
#  include <sys/select.h> /* select */
#  include <sys/time.h>   /* timerclear */
#else
#  include <sys/poll.h>   /* poll */
#endif

#include "def.h"
#include "log.h"
#include "data.h"
#include "util.h"
#include "client.h"

#define SOCK_ERROR    (int)(-1) /**< ソケットエラー */

/* 外部変数 */
extern bool gflag;              /**< gオプションフラグ */

#ifndef _USE_SELECT
enum {
    STDIN_POLL, /**< 標準入力 */
    SOCK_POLL,  /**< ソケット */
    MAX_POLL    /**< ポーリング数 */
};
#endif /* _USE_SELECT */

/** 標準入力読込 */
static int read_stdin(int sock, char *buf, size_t length);
/** ソケット読込 */
static int read_sock(int sock);

/**
 * 標準入力読込
 *
 * @param[in] ソケット
 * @param[out] バッファ
 * @param[in] バッファサイズ
 * @retval ST_NG エラー
 */
static int
read_stdin(int sock, char *buf, size_t length)
{
    int retval = 0;           /* 戻り値 */
    char *fgetsval = NULL;    /* fgets戻り値 */
    struct client_data sdata; /* 送信データ構造体 */
    size_t rlen = 0;          /* 送信または受信する長さ */

    dbglog("start");

    /* 標準入力 */
    fgetsval = fgets(buf, length, stdin);
    if (feof(stdin) || ferror(stdin)) {
        outlog("fgets[%p]: feof[%d]", fgetsval, feof(stdin));
        clearerr(stdin); /* エラーをクリア */
        return ST_NG;
    }
    dbgdump((char *)buf, length, "buf[%u]", strlen(buf));
    buf[strlen(buf) - 1] = '\0'; /* 改行削除 */
    dbgdump((char *)buf, length, "buf[%u]", strlen(buf));

    memset(&sdata, 0, sizeof(struct client_data));
    set_client_data(&sdata, (unsigned char *)buf, strlen(buf));
    /* データ送信 */
    rlen = sizeof(struct header) + strlen(buf);
    retval = send_data(sock, &sdata, rlen);
    if (retval < 0) /* エラー */
        return ST_NG;

    stdlog("send_data[%d]: sdata[%p]: rlen[%u]: %u",
           retval, &sdata, rlen, sdata.hd.length);
    dumplog(&sdata, rlen);
    if (gflag)
        outdump(&sdata, rlen, "sdata[%u]", rlen);
    return ST_OK;
}

/**
 * ソケット読込
 *
 * @param[in] ソケット
 * retval ST_NG エラー
 */
static int
read_sock(int sock)
{
    int retval = 0;           /* 戻り値 */
    size_t length = 0;        /* 送信または受信する長さ */
    unsigned short cs;        /* チェックサム値 */
    struct server_data rdata; /* 受信データ構造体 */

    /* ヘッダ受信 */
    memset(&rdata, 0, sizeof(struct server_data));
    length = sizeof(struct header);
    retval = recv_data(sock, &rdata, length);
    if (retval < 0) /* エラー */
        return ST_NG;
    stdlog("recv_data[%d]: rdata[%p]: length[%u]",
           retval, &rdata, length);
    dumplog(&rdata, length);
    if (gflag)
        outdump(&rdata, length, "rdata[%u]", length);

    length = rdata.hd.length; /* データ長を保持 */

    /* データ受信 */
    retval = recv_data(sock, rdata.answer, length);
    if (retval < 0) /* エラー */
        return ST_NG;

    stdlog("recv_data[%d]: rdata[%p]: length[%u]",
           retval, &rdata, length);
    dumplog(rdata.answer, length);
    if (gflag)
        outdump(rdata.answer, length, "rdata.answer[%u]", length);

    cs = in_cksum((unsigned short *)rdata.answer, length);
    if (cs != rdata.hd.checksum) { /* チェックサムエラー */
        outlog("checksum error; cs[0x%x!=0x%x]",
               cs, rdata.hd.checksum);
        return ST_NG;
    }
    retval = fprintf(stdout, "%s\n", rdata.answer);
    if (retval < 0) {
        outlog("fprintf[%d]", retval);
        return ST_NG;
    }
    retval = fflush(stdout);
    if (retval == EOF) {
        outlog("fflush[%d]", retval);
        return ST_NG;
    }
    return ST_OK;
}

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
    struct in_addr addr;       /* IPアドレス情報構造体 */
    int sock = -1;             /* ソケット */
    int retval = 0;            /* 戻り値 */
    int closeval = 0;          /* close戻り値 */

    dbglog("start");

    /* 初期化 */
    memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;

    if (set_hostname(&server, &addr, host) < 0)
        return SOCK_ERROR;
    if (set_port(&server, port) < 0)
        return SOCK_ERROR;

    /* ソケット生成 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        outlog("sock[%d]", sock);
        return SOCK_ERROR;
    }

    /* コネクト */
    retval = connect(sock, (struct sockaddr *)&server,
                     sizeof(struct sockaddr_in));
    if (retval < 0) {
        outlog("connect[%d]: sock[%d]", retval, sock);
        /* ソケットクローズ */
        if (sock != -1) {
            closeval = close(sock);
            if (closeval < 0)
                outlog("close[%d]: sock[%d]", closeval, sock);
        }
        return SOCK_ERROR;
    }
    return sock;
}

/**
 * ソケット送受信
 *
 * @param[in] sock ソケット
 * @return なし
 */
void
client_loop(int sock)
{
    char buf[BUF_SIZE];       /* 送受信バッファ */
    int retval = 0;           /* 戻り値 */
    int ready = 0;            /* select戻り値 */
#ifdef _USE_SELECT
    fd_set fds, tfds;         /* selectマスク */
    struct timeval timeout;   /* タイムアウト値 */
#else
    struct pollfd targets[MAX_POLL]; /* poll */
#endif /* _USE_SELECT */

    dbglog("start: sock[%d]", sock);

#ifdef _USE_SELECT
    /* マスクの設定 */
    FD_ZERO(&tfds);              /* 初期化 */
    FD_SET(sock, &tfds);         /* ソケットをマスク */
    FD_SET(STDIN_FILENO, &tfds); /* 標準入力をマスク */

    /* タイムアウト値の初期化 */
    timerclear(&timeout);
#endif /* _USE_SELECT */

    do {
        errno = 0;    /* errno初期化 */
#ifdef _USE_SELECT
        fds = tfds; /* マスクの代入 */
        timeout.tv_sec = 1;   /* 1秒に設定 */
        timeout.tv_usec = 0;
        ready = select(sock + 1, &fds, NULL, NULL, &timeout); 
#else
        targets[STDIN_POLL].fd = STDIN_FILENO;
        targets[STDIN_POLL].events = POLLIN;
        targets[SOCK_POLL].fd = sock;
        targets[SOCK_POLL].events = POLLIN;
        ready = poll(targets, MAX_POLL, 1 * 1000);
#endif /* _USE_SELECT */
        memset(buf, 0, sizeof(buf)); /* 初期化 */
        switch (ready) {
        case -1:
            if (errno == EINTR) { /* 割り込み */
                outlog("interrupt[%d]", errno);
                break;
            }
            /* selectエラー */
            outlog("select[%d]", ready);
            break;
        case 0: /* タイムアウト */
            break;
        default:
#ifdef _USE_SELECT
            if (FD_ISSET(STDIN_FILENO, &fds)) /* 標準入力レディ */
                retval = read_stdin(sock, buf, sizeof(buf));
            if (FD_ISSET(sock, &fds)) /* ソケットレディ */
                retval = read_sock(sock);
#else
            if (targets[STDIN_POLL].revents & POLLIN) /* 標準入力レディ */
                retval = read_stdin(sock, buf, sizeof(buf));
            if (targets[SOCK_POLL].revents & POLLIN) /* ソケットレディ */
                retval = read_sock(sock);
#endif /* _USE_SELECT */
            break;
        } /* switch */
    } while (strcmp(buf, "exit") && strcmp(buf, "quit") && !retval); 
}

