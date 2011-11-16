/**
 * @file  lib/net.c
 * @brief ネットワーク設定
 *
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

#include <stdlib.h>     /* strtol EXIT_SUCCESS */
#include <string.h>     /* memcpy memset strdup */
#include <unistd.h>     /* close */
#include <ctype.h>      /* isdigit */
#include <sys/types.h>  /* send recv */
#include <sys/socket.h> /* send recv */
#include <arpa/inet.h>  /* inet_aton */
#include <netinet/in.h> /* struct in_addr */
#include <errno.h>      /* errno */
#include <fcntl.h>      /* fcntl */

#include "log.h"
#include "util.h"
#include "net.h"

/**
 * ホスト名設定
 *
 * @param[in,out] addr sockaddr_in構造体
 * @param[in,out] h_addr in_addr構造体
 * @param[in] host ホスト名
 * @retval EX_NG エラー
 */
int
set_hostname(struct sockaddr_in *addr, struct in_addr h_addr, const char *host)
{
    struct hostent *hp = NULL; /* ホスト情報構造体 */
    int retval = 0;            /* 戻り値 */

    dbglog("start: addr=%p, h_addr=%p, host=%s",
           addr, &h_addr, host);

    if (!host)
        return EX_NG;

    retval = inet_aton(host, &h_addr);
    if (!retval) { /* IPアドレスではない */
        dbglog("inet_aton=%d, host=%s", retval, host);
        hp = gethostbyname(host);
        if (!hp) { /* ホスト名でもない */
            outlog("gethostbyname=%p, host=%s", hp, host);
            return EX_NG;
        }
        dbglog("h_addr=%p, h_addr=%u h_length=%d", &h_addr,
               sizeof(h_addr), hp->h_length);
        (void)memcpy(&h_addr, (struct in_addr *)*hp->h_addr_list,
                     hp->h_length);
    }
    /* IPアドレスを設定 */
    (void)memcpy(&addr->sin_addr, &h_addr, sizeof(addr->sin_addr));

    dbglog("h_addr=%p, inet_nta(h_addr)=%s", &h_addr, inet_ntoa(h_addr));

    return EX_OK;
}

/**
 * ポート番号設定
 *
 * @param[in,out] addr sockaddr_in構造体
 * @param[in] port ポート番号
 * @retval EX_NG エラー
 */
int
set_port(struct sockaddr_in *addr, const char *port)
{
    struct servent *sp = NULL; /* サービス情報構造体 */
    long portno = 0;           /* ポート番号 */
    const int base = 10;       /* 基数 */

    dbglog("start: addr=%p, port=%s", addr, port);

    if (!port)
        return EX_NG;

    if (isdigit(port[0])) { /* 先頭が数字 */
        portno = strtol(port, NULL, base);
        dbglog("portno=%d", portno);
        if (portno <= 0 || 65535 <= portno) {
            outlog("portno=%d", portno);
            return EX_NG;
        }
        dbglog("portno=%d", htons(portno));
        addr->sin_port = htons(portno);
    } else {
        sp = getservbyname(port, "tcp");
        if (!sp) {
            outlog("getservbyname=%p, port=%s", sp, port);
            return EX_NG;
        }
        addr->sin_port = sp->s_port;
    }

    dbglog("sp=%p, addr->sin_port=%d", sp, ntohs(addr->sin_port));

    return EX_OK;
}

/**
 * ブロッキングモードの設定
 *
 * @param[in] fd ファイルディスクリプタ
 * @param[in] mode ブロッキングモード
 * @retval EX_NG エラー
 */
int
set_block(int fd, blockmode mode)
{
    int flags = 0; /* fcntl戻り値 */

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        outlog("fcntl=%d", flags);
        return EX_NG;
    }
    if (mode == NONBLOCK) /* ノンブロッキング */
        fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    else if (mode == BLOCKING) /* ブロッキング */
        fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);

    return EX_OK;
}

/**
 * データ送信
 *
 * @param[in] sock ソケット
 * @param[in] sdata データ
 * @param[in] length データ長
 * @retval EX_NG エラー
 */
int
send_data(const int sock, const void *sdata, const size_t length)
{
    ssize_t len = 0;   /* sendの戻り値 */

    outlog("start: sdata=%p, length=%u", sdata, length);

    len = send(sock, sdata, length, 0);
    if (len < 0) {
        outlog("send=%d", len);
        return EX_NG;
    }
    dbglog("send=%d", len);
    return EX_OK;
}

/**
 * データ受信
 *
 * @param[in] sock ソケット
 * @param[out] rdata データ
 * @param[in] length データ長
 * @return 受信したデータ長
 * @retval EX_NG エラー
 */
int
recv_data(const int sock, void *rdata, const size_t length)
{
    ssize_t len = 0;   /* recvの戻り値 */
    int rlen = 0;      /* 受信した長さ */
    int left = length; /* 残りの長さ */

    outlog("start: rdata=%p", rdata);

    while (left > 0) {
        len = recv(sock, rdata + rlen, left, 0);
        dbglog("recv=%d, rlen=%d, left=%d", len, rlen, left);
        if (len < 0) { /* エラー */
            if (errno != EINTR && errno != EAGAIN) {
                outlog("recv=%d, rlen=%d, left=%d", len, rlen, left);
                return EX_NG;
            }
        } else if (len == 0) { /* ソケットが切断された */
            outlog("%s recv=%d, rlen=%d, left=%d",
                   "The socket is not connected.", len, rlen, left);
            return EX_NG;
        } else { /* 正常時 */
            rlen += len;
            left -= len;
        }
    }
    dbglog("recv=%d, rlen=%d, left=%d", len, rlen, left);
    return rlen;
}

/**
 * データ受信
 *
 * 新たに領域確保し, データを受信する.
 *
 * @param[in] sock ソケット
 * @param[in,out] length データ長
 * @retval NULL エラー
 */
void *
recv_data_new(const int sock, size_t *length)
{
    void *rdata = NULL; /* 受信データ */

    dbglog("start: rdata=%p", rdata);

    /* メモリ確保 */
    rdata = (uchar *)malloc(*length * sizeof(uchar));
    if (!rdata) {
        outlog("malloc=%p", rdata);
        return NULL;
    }
    (void)memset(rdata, 0, *length);

    /* データ受信 */
    *length = recv_data(sock, rdata, *length);
    if (*length < 0) { /* エラー */
        memfree((void **)&rdata, NULL);
        return NULL;
    }

    return rdata;
}

/**
 * ソケットクローズ
 *
 * @param[in] sock ソケット
 * @return なし
 */
void
close_sock(int *sock) {

    int retval = 0; /* 戻り値 */

    if (*sock != -1) {
        retval = close(*sock);
        if (retval < 0)
            outlog("close=%d, sock=%d", retval, *sock);
    }
    *sock = -1;
}

