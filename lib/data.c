/**
 * @file  data.c
 * @brief 送受信データ構造体
 *
 * @sa data.h
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

#include "data.h"
#include "util.h"

/**
 * クライアントデータ構造体設定
 */
void
set_client_data(struct client_data *dt,
                const unsigned char *buf, const size_t len)
{
    dt->hd.checksum = in_cksum((unsigned short *)buf, (unsigned int)len);
    dt->hd.length = len;
    (void)memset(dt->expression, 0, sizeof(dt->expression));
    (void)memcpy(dt->expression, buf, len);
}

/**
 * サーバデータ構造体設定
 */
void
set_server_data(struct server_data *dt,
                const unsigned char *buf, const size_t len)
{
    dt->hd.checksum = in_cksum((unsigned short *)buf, (unsigned int)len);
    dt->hd.length = len;
    (void)memset(dt->answer, 0, sizeof(dt->answer));
    (void)memcpy(dt->answer, buf, len);
}

