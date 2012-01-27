/**
 * @file  lib/net.c
 * @brief ネットワーク
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

#include <stdlib.h>     /* strtol */
#include <string.h>     /* memcpy memset */
#include <unistd.h>     /* close */
#include <ctype.h>      /* isdigit */
#include <sys/socket.h> /* send recv */
#include <sys/types.h>  /* send etc... */
#include <arpa/inet.h>  /* inet_aton inet_ntoa */
#include <netinet/in.h> /* struct in_addr */
#include <errno.h>      /* errno */
#include <fcntl.h>      /* fcntl */
#ifdef __cplusplus
# define __STDC_FORMAT_MACROS
#endif
#include <inttypes.h>   /* uint16_t, PRIu16 */

#include "log.h"
#include "net.h"

/**
 * ホスト名設定
 *
 * @param[out] addr sockaddr_in構造体
 * @param[in] host ホスト名またはIPアドレス
 * @retval EX_NG エラー
 */
int
set_hostname(struct sockaddr_in *addr, const char *host)
{
    struct hostent *hp = NULL; /* ホスト情報構造体 */
    struct in_addr s_addr;     /* IPアドレス情報構造体 */
    int retval = 0;            /* 戻り値 */

    dbglog("start: host=%s", host);

    if (!addr || !host)
        return EX_NG;

    (void)memset(&s_addr, 0, sizeof(struct in_addr));

    retval = inet_aton(host, &s_addr);
    if (!retval) { /* IPアドレスではない */
        dbglog("inet_aton: host=%s", host);
        hp = gethostbyname(host);
        if (!hp) { /* ホスト名でもない */
            outlog("gethostbyname: host=%s", host);
            return EX_NG;
        }
        dbglog("%p s_addr=%zu h_length=%d",
               &s_addr, sizeof(s_addr), hp->h_length);
        /* ホスト名を設定 */
        (void)memcpy(&s_addr, (struct in_addr *)*hp->h_addr_list,
                     hp->h_length);
    }

    /* IPアドレスを設定 */
    (void)memcpy(&addr->sin_addr, &s_addr, sizeof(struct in_addr));

    dbglog("%s", inet_ntoa(addr->sin_addr));

    return EX_OK;
}

/**
 * ポート番号設定
 *
 * @param[out] addr sockaddr_in構造体
 * @param[in] port ポート番号またはサービス名
 * @retval EX_NG エラー
 */
int
set_port(struct sockaddr_in *addr, const char *port)
{
    struct servent *sp = NULL; /* サービス情報構造体 */
    uint16_t portno = 0;       /* ポート番号 */
    const int base = 10;       /* 基数 */

    dbglog("start: addr=%p, port=%s", addr, port);

    if (!addr || !port)
        return EX_NG;

    if (isdigit(port[0])) { /* 先頭が数字 */
        portno = (uint16_t)strtol(port, NULL, base);
        dbglog("portno=%"PRIu16", 0x%"PRIx16"", portno, portno);
        if (portno <= 0 || 65535 <= portno) {
            outlog("portno=%d", portno);
            return EX_NG;
        }
        dbglog("portno=0x%"PRIx16"", htons(portno));
        addr->sin_port = (u_short)htons(portno);
    } else {
        sp = getservbyname(port, "tcp");
        if (!sp) {
            outlog("getservbyname: port=%s", port);
            return EX_NG;
        }
        addr->sin_port = sp->s_port;
    }
    dbglog("port=%"PRIu16"", ntohs((uint16_t)addr->sin_port));
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
    int flags = 0;  /* fcntl戻り値(F_GETFL) */
    int retval = 0; /* fcntl戻り値(F_SETFL) */

    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) {
        outlog("fcntl=0x%x", flags);
        return EX_NG;
    }
    dbglog("fcntl=0x%x", flags);

    if (mode == NONBLOCK) { /* ノンブロッキング */
        retval = fcntl(fd, F_SETFL, flags | O_NONBLOCK);
        if (retval < 0)
            outlog("fcntl=%d", retval);
    } else if (mode == BLOCKING) { /* ブロッキング */
        retval = fcntl(fd, F_SETFL, flags & ~O_NONBLOCK);
        if (retval < 0)
            outlog("fcntl=%d", retval);
    } else { /* no mode */
        outlog("mode=%d", mode);
        return EX_NG;
    }

#ifdef _DEBUG
    flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0)
        dbglog("fcntl=0x%x", flags);
    dbglog("fcntl=0x%x", flags);
#endif /* _DEBUG */

    return EX_OK;
}

/**
 * データ送信
 *
 * @param[in] sock ソケット
 * @param[in] sdata データ
 * @param[in,out] length データ長
 * @retval EX_NG エラー
 */
int
send_data(const int sock, const void *sdata, size_t *length)
{
    ssize_t len = 0;         /* send戻り値 */
    size_t left = 0;         /* 残りのバイト数 */
    const uchar *ptr = NULL; /* ポインタ */

    dbglog("start: sdata=%p, length=%zu", sdata, *length);

    ptr = (uchar *)sdata;
    left = *length;
    while (left > 0) {
        len = send(sock, sdata, *length, 0);
        dbglog("send=%zd, ptr=%p, left=%zu", len, ptr, left);
        if (len <= 0) {
            if ((errno == EINTR) ||
                (errno == EAGAIN) || (errno == EWOULDBLOCK))
                len = 0;
            else
                goto error_handler;
        }
        left -= len;
        ptr += len;
    }
    *length -= left;
    dbglog("send=%zd, sock=%d, ptr=%p, left=%zu, length=%zu",
           len, sock, ptr, left, *length);
    return EX_OK;

error_handler:
    *length -= left;
    outlog("send=%zd, sock=%d, ptr=%p, left=%zu, length=%zu",
           len, sock, ptr, left, *length);
    return EX_NG;
}

/**
 * データ受信
 *
 * @param[in] sock ソケット
 * @param[out] rdata データ
 * @param[in,out] length データ長
 * @retval EX_NG エラー
 */
int
recv_data(const int sock, void *rdata, size_t *length)
{
    ssize_t len = 0;   /* recv戻り値 */
    size_t left = 0;   /* 残りのバイト数 */
    uchar *ptr = NULL; /* ポインタ */

    dbglog("start: rdata=%p, length=%zu", rdata, *length);

    ptr = (uchar *)rdata;
    left = *length;
    while (left > 0) {
        len = recv(sock, ptr, left, 0);
        dbglog("recv=%zd, ptr=%p, left=%zu", len, ptr, left);
        if (len < 0) { /* エラー */
            if ((errno == EINTR) ||
                (errno == EAGAIN) || (errno == EWOULDBLOCK))
                len = 0;
            else
                goto error_handler;
        } else if (len == 0) { /* 接続先がシャットダウンした */
            outlog("The socket is not connected.");
            goto error_handler;
        } else { /* 正常時 */
            left -= len;
            ptr += len;
        }
    }
    *length -= left;
    dbglog("recv=%zd, sock=%d, ptr=%p, left=%zu, length=%zu",
           len, sock, ptr, left, *length);
    return EX_OK;

error_handler:
    *length -= left;
    outlog("recv=%zd, sock=%d, ptr=%p, left=%zu, length=%zu",
           len, sock, ptr, left, *length);
    return EX_NG;
}

/**
 * データ受信
 *
 * 新たに領域確保し, データを受信する.
 *
 * @param[in] sock ソケット
 * @param[in,out] length データ長
 * @return 受信されたデータポインタ
 * @retval NULL エラー
 */
void *
recv_data_new(const int sock, size_t *length)
{
    size_t len = *length; /* バイト数 */
    int retval = 0;       /* 戻り値 */
    void *rdata = NULL;   /* 受信データ */

    dbglog("start: length=%zu", *length);

    /* メモリ確保 */
    rdata = malloc(len);
    if (!rdata) {
        outlog("malloc: len= %zu", len);
        return NULL;
    }
    (void)memset(rdata, 0, len);

    /* データ受信 */
    retval = recv_data(sock, rdata, &len);
    if (retval < 0) { /* エラー */
        *length = 0;
        return rdata;
    }

    *length = len; /* 受信されたバイト数を設定 */
    return rdata;
}

/**
 * ソケットクローズ
 *
 * シャットダウン後, クローズする.
 * @param[in] sock ソケット
 * @retval EX_NG エラー
 */
int
close_sock(int *sock)
{
    int retval = 0;           /* 戻り値 */
    const int sockfd = *sock; /* ソケット */

    dbglog("start: %d", sockfd);

    if (sockfd < 0)
        return EX_OK;

    retval = close(sockfd);
    if (retval < 0) {
        outlog("close: sockfd=%d", sockfd);
        return EX_NG;
    }
    *sock = -1;
    return EX_OK;
}

