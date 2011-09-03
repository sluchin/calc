/**
 * @file  util.c
 * @brief ユーティリティ
 *
 * @sa util.h
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
 
#include "log.h"
#include "util.h"

#define BUF_SIZE  (int)256  /**< 送受信バッファ最大バイト数 */

/**
 * ホスト名設定
 *
 * @param[in,out] addr sockaddr_in構造体
 * @param[in,out] h_addr in_addr構造体
 * @param[in] host ホスト名
 * @retval ST_NG エラー
 */
int
set_hostname(struct sockaddr_in *addr, struct in_addr h_addr, const char *host)
{
    struct hostent *hp = NULL; /* ホスト情報構造体 */
    int retval = 0;            /* 戻り値 */

    dbglog("start: addr[%p] h_addr[%p] host[%s]", addr, &h_addr, host);

    if (!host)
        return ST_NG;

    retval = inet_aton(host, &h_addr);
    if (!retval) { /* IPアドレスではない */
        dbglog("inet_aton[%d]: %s", retval, host);
        hp = gethostbyname(host);
        if (!hp) { /* ホスト名でもない */
            outlog("gethostbyname[%p]: %s", hp, host);
            return ST_NG;
        }
        dbglog("h_addr[%p]: length: h_addr[%u] h_length[%d]", &h_addr,
               sizeof(h_addr), hp->h_length);
        (void)memcpy(&h_addr, (struct in_addr *)*hp->h_addr_list,
                     hp->h_length);
    }
    /* IPアドレスを設定 */
    (void)memcpy(&addr->sin_addr, &h_addr, sizeof(addr->sin_addr));

    dbglog("h_addr[%p]: %s", &h_addr, inet_ntoa(h_addr));

    return ST_OK;
}

/**
 * ポート番号設定
 *
 * @param[in,out] addr sockaddr_in構造体
 * @param[in] port ポート番号
 * @retval ST_NG エラー
 */
int
set_port(struct sockaddr_in *addr, const char *port)
{
    struct servent *sp = NULL; /* サービス情報構造体 */
    long portno = 0;           /* ポート番号 */
    const int base = 10;       /* 基数 */

    dbglog("start: addr[%p] port[%s]", addr, port);

    if (!port)
        return ST_NG;

    if (isdigit(port[0])) { /* 先頭が数字 */
        portno = strtol(port, NULL, base);
        dbglog("portno[%d]", portno);
        if (portno <= 0 || 65535 <= portno) {
            outlog("portno[%d]", portno);
            return ST_NG;
        }
        dbglog("portno[%d]", htons(portno));
        addr->sin_port = htons(portno);
    } else {
        sp = getservbyname(port, "tcp");
        if (!sp) {
            outlog("getservbyname[%p]: %s", sp, port);
            return ST_NG;
        }
        addr->sin_port = sp->s_port;
    }

    dbglog("sp[%p]: %d", sp, ntohs(addr->sin_port));

    return ST_OK;
}

/**
 * データ送信
 *
 * @param[in] sock ソケット
 * @param[in] sdata データ
 * @param[in] length データ長
 * @retval ST_NG エラー
 */
int
send_data(const int sock, const void *sdata, const size_t length)
{
    ssize_t len = 0; /* sendの戻り値 */

    dbglog("start: sdata[%p]", sdata);

    if (length <= 0)
        return ST_NG;
    len = send(sock, sdata, length, 0);
    dbglog("send[%d]", len);
    if (len < 0) { /* エラー */
        outlog("send[%d]", len);
        return ST_NG;
    }
    return ST_OK;
}

/**
 * データ受信
 *
 * @param[in] sock ソケット
 * @param[out] rdata データ
 * @param[in] length データ長
 * @retval ST_NG エラー
 */
int
recv_data(const int sock, void *rdata, const size_t length)
{
    ssize_t len = 0;     /* recvの戻り値 */
    int rlen = 0;        /* 受信した長さ */
    int remain = length; /* 残りの長さ */

    dbglog("start: rdata[%p]", rdata);

    while (rlen < length) {
        if (remain <= 0)
            return ST_NG;
        len = recv(sock, rdata + rlen, remain, 0);
        dbglog("recv[%d]: rlen[%d]: remain[%d]", len, rlen, remain);
        if (len < 0) { /* エラー */
            outlog("recv[%d]: rlen[%d]: remain[%d]", len, rlen, remain);
            return ST_NG;
        } else if (len == 0) { /* ソケットが切断された */
            outlog("recv[%d]: EOF: rlen[%d]: remain[%d]", len, rlen, remain);
            return ST_NG;
        }
        remain -= len;
        rlen += len;
    }
    return ST_OK;
}

/**
 * ソケットクローズ
 *
 * @return なし
 */
void
close_sock(int *sock) {

    int retval = 0; /* 戻り値 */

    if (*sock != -1) {
        retval = close(*sock);
        if (retval < 0)
            outlog("close[%d]: sock[%d]", retval, *sock);
    }
    *sock = -1;
}

/**
 * チェックサム
 *
 * チェックサムの計算をする.
 *
 * @param[in] addr
 * @param[in] len 長さ
 * @return チェックサム値
 */
ushort
in_cksum(unsigned short *addr, const size_t len)
{
    int nleft = len;
    int sum = 0;
    ushort *w = addr;
    ushort answer = 0;

    while (nleft > 1) {
        sum += *w++;
        nleft -= 2;
    }
    if (nleft == 1) {
        *(uchar *)(&answer) = *(uchar *)w;
        sum += answer;
    }
    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;

    return answer;
}

/**
 * 一行読込
 *
 * @param[in] ファイルポインタ
 * @return 文字列
 * @retval NULL エラー
 */
uchar *
read_line(FILE *fp)
{
    char *retval = NULL; /* fgetsの戻り値 */
    size_t length = 0;   /* 文字列長 */
    size_t total = 0;    /* 文字列長全て */
    uchar *line = NULL;  /* 文字列 */
    uchar *tmp = NULL;   /* 一時アドレス */
    uchar buf[BUF_SIZE]; /* バッファ */

    do {
        (void)memset(buf, 0, sizeof(buf));
        retval = fgets((char *)buf, sizeof(buf), fp);
        if (feof(fp) || ferror(fp)) { /* エラー */
            outlog("fgets[%p]", retval);
            clearerr(fp);
            break;
        }
        length = strlen((char *)buf);
        dbgdump(buf, length, "buf[%u]", length);

        tmp = (uchar *)realloc(line, (total + length + 1) * sizeof(uchar));
        if (!tmp) {
            outlog("realloc[%p]: %u", line, total + length + 1);
            break;
        }
        line = tmp;

        (void)memcpy(line + total, buf, length + 1);

        total += length;
        dbglog("length[%u] total[%u]", length, total);
        dbglog("line[%p]: %s", line, line);
        dbgdump(line, total + 1, "line[%u]", total + 1);

    } while (*(line + total - 1) != '\n');

    if (line && *(line + total - 1) == '\n')
        *(line + total - 1) = '\0'; /* 改行削除 */

    return line;
}

