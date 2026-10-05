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

#include <unistd.h>    /* ssize_t */
#include <string.h>    /* memset memcpy */
#include <stdlib.h>    /* malloc */
#include <arpa/inet.h> /* htonl */

#include "log.h"
#include "data.h"

/* アライメント */
#define ALIGN8(x) (((x) + 7U) & ~(size_t)7U) /**< アライメント 8byte */

/**
 * @brief クライアントデータ構造体設定
 *
 * @param[out] dt 送受信データ構造体
 * @param[in] buf 送受信バッファ
 * @param[in] len 長さ
 * @return 構造体バイト数
 * @retval EX_NG メモリ確保できない
 */
ssize_t
set_client_data(struct client_data **dt, const unsigned char *buf, const size_t len)
{
    size_t length = 0U;  /* 構造体バイト数 */
    size_t datalen = 0U; /* データ長 */

    dbglog("start: len=%zu", len);

    if (buf == NULL)
        return EX_NG;

    datalen = ALIGN8(len);
    length = sizeof(struct header) + datalen;
    dbglog("length=%zu", length);

    (*dt) = (struct client_data *)malloc(length);
    if (*dt == NULL) {
        outlog("malloc: length=%zu", length);
        return EX_NG;
    }
    (void)memset((*dt), 0, length);
    dbglog("dt=%p", (const void *)(*dt));

    (*dt)->hd.length = htonl((uint32_t)datalen); /* データ長を設定 */
    (void)memcpy((*dt)->expression, buf, len);

    dbgdump(*dt, length, "dt=%p, length=%zu", (const void *)(*dt), length);

    return (ssize_t)length;
}

/**
 * @brief サーバデータ構造体設定
 *
 * @param[out] dt 送受信データ構造体
 * @param[in] buf 送受信バッファ
 * @param[in] len 長さ
 * @return 構造体バイト数
 */
ssize_t
set_server_data(struct server_data **dt, const unsigned char *buf, const size_t len)
{
    size_t length = 0U;  /* 構造体バイト数 */
    size_t datalen = 0U; /* データ長 */

    dbglog("start: len=%zu", len);

    if (buf == NULL)
        return EX_NG;

    datalen = ALIGN8(len);
    length = sizeof(struct header) + datalen;
    dbglog("length=%zu", length);

    (*dt) = (struct server_data *)malloc(length);
    if (*dt == NULL) {
        outlog("malloc: length=%zu", length);
        return EX_NG;
    }
    (void)memset((*dt), 0, length);
    dbglog("dt=%p", (const void *)(*dt));

    (*dt)->hd.length = htonl((uint32_t)datalen); /* データ長を設定 */
    (void)memcpy((*dt)->answer, buf, len);

    dbgdump(*dt, length, "dt=%p, length=%zu", (const void *)(*dt), length);

    return (ssize_t)length;
}
