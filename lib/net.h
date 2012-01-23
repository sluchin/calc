/**
 * @file  lib/net.h
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

#ifndef _NET_H_
#define _NET_H_

#include <netdb.h>   /* struct sockaddr_in */

#include "def.h"

/** ブロッキングモード */
enum _blockmode {
    NONBLOCK = 0, /**< ノンブロッキングモード */
    BLOCKING      /**< ブロッキングモード */
};
typedef enum _blockmode blockmode;

/** ホスト名設定 */
int set_hostname(struct sockaddr_in *addr, const char *host);

/** ポート番号設定 */
int set_port(struct sockaddr_in *addr, const char *port);

/** ブロッキングモード設定 */
int set_block(int fd, blockmode mode);

/** データ送信 */
int send_data(const int sock, const void *sdata, size_t *length);

/** データ受信 */
int recv_data(const int sock, void *rdata, size_t *length);

/** データ受信 */
void *recv_data_new(const int sock, size_t *length);

/** ソケットクローズ */
int close_sock(int *sock);

#endif /* _NET_H_ */

