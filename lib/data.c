/**
 * @file  lib/data.c
 * @brief 送受信データ構造体
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

#include <string.h> /* memset memcpy */

#include "util.h"
#include "log.h"
#include "data.h"

/* アライメント */
#define ALIGN2(x)  (((x)+1) & ~1) /**< アライメント 2byte */
#define ALIGN4(x)  (((x)+3) & ~3) /**< アライメント 4byte */
#define ALIGN8(x)  (((x)+7) & ~7) /**< アライメント 8byte */

/**
 * クライアントデータ構造体設定
 *
 * @param[out] dt 送受信データ構造体
 * @param[in] buf 送受信バッファ
 * @param[in] len 長さ
 * @return 構造体バイト数
 */
ssize_t
set_client_data(struct client_data **dt, uchar *buf, const size_t len)
{
    size_t length = 0;  /* 構造体バイト数 */

    dbglog("start");

    length = sizeof(struct header) + len;
    length = ALIGN8(length);
    dbglog("length=%zu", length);

    (*dt) = (struct client_data *)malloc(length);
    if (!(*dt)) {
        outlog("malloc=%p", (*dt));
        return EX_NG;
    }
    (void)memset((*dt), 0, length);
    dbglog("dt=%p", (*dt));

    (*dt)->hd.checksum = in_cksum((ushort *)buf, (uint)len);
    (*dt)->hd.length = len; /* データ長を設定 */
    (void)memcpy((*dt)->expression, buf, len);

    return (ssize_t)length;
}

/**
 * サーバデータ構造体設定
 *
 * @param[out] dt 送受信データ構造体
 * @param[in] buf 送受信バッファ
 * @param[in] len 長さ
 * @return 構造体バイト数
 */
ssize_t
set_server_data(struct server_data **dt, uchar *buf, const size_t len)
{
    size_t length = 0;  /* 構造体バイト数 */

    dbglog("start");

    length = sizeof(struct header) + len;
    length = ALIGN8(length);
    dbglog("length=%zu", length);

    (*dt) = (struct server_data *)malloc(length);
    if (!(*dt)) {
        outlog("malloc=%p", (*dt));
        return EX_NG;
    }
    (void)memset((*dt), 0, length);
    dbglog("dt=%p", (*dt));

    (*dt)->hd.checksum = in_cksum((ushort *)buf, (uint)len);
    (*dt)->hd.length = len; /* データ長を設定 */
    (void)memcpy((*dt)->answer, buf, len);

    return (ssize_t)length;
}

