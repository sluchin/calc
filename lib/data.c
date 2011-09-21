/**
 * @file  lib/data.c
 * @brief 送受信データ構造体
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

#include <string.h> /* memset memcpy */

#include "util.h"
#include "log.h"
#include "data.h"

/**
 * クライアントデータ構造体設定
 *
 * @param[in,out] dt 送受信データ構造体
 * @param[in] buf 送受信バッファ
 * @param[in] len 長さ
 * @return クライアントデータ構造体
 */
struct client_data *
set_client_data(struct client_data *dt,
                uchar *buf, const size_t len)
{
    size_t length = 0;
    size_t padding = 0;

    length = sizeof(struct header) + len;
    padding = length % 4;
    padding = padding ? 4 - padding : padding;
    dbglog("length=%u, padding=%d", length, padding);
    length += padding;

    dt = (struct client_data *)malloc(length);
    if (!dt) {
        outlog("malloc=%p", dt);
        return NULL;
    }
    (void)memset(dt, 0, length);
    dbglog("dt=%p", dt);

    dt->hd.checksum = in_cksum((ushort *)buf, (uint)len);
    dt->hd.length = len;
    (void)memcpy(dt->expression, buf, len);

    return dt;
}

/**
 * サーバデータ構造体設定
 *
 * @param[in,out] dt 送受信データ構造体
 * @param[in] buf 送受信バッファ
 * @param[in] len 長さ
 * @return サーバデータ構造体
 */
struct server_data *
set_server_data(struct server_data *dt,
                uchar *buf, const size_t len)
{
    size_t length = 0;
    size_t padding = 0;

    length = sizeof(struct header) + len;
    padding = length % 4;
    padding = padding ? 4 - padding : padding;
    dbglog("length=%u, padding=%d", length, padding);
    length += padding;

    dt = (struct server_data *)malloc(length);
    if (!dt) {
        outlog("malloc=%p", dt);
        return NULL;
    }
    (void)memset(dt, 0, length);
    dbglog("dt=%p", dt);

    dt->hd.checksum = in_cksum((ushort *)buf, (uint)len);
    dt->hd.length = len;
    (void)memcpy(dt->answer, buf, len);

    return dt;
}

