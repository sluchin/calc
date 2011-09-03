/**
 * @file  util.h
 * @brief ユーティリティ
 *
 * @sa util.c
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

#ifndef _UTIL_H_
#define _UTIL_H_

#include <stdio.h> /* FILE */
#include <netdb.h> /* struct sockaddr_in */

#include "def.h"

/** ホスト名設定 */
int set_hostname(struct sockaddr_in *addr,
                 struct in_addr h_addr, const char *host);

/** ポート番号設定 */
int set_port(struct sockaddr_in *addr, const char *port);

/** データ送信 */
int send_data(const int sock, const void *sdata, const size_t length);

/** データ受信 */
int recv_data(const int sock, void *rdata, const size_t length);

/** ソケットクローズ */
void close_sock(int *sock);

/** チェックサム */
ushort in_cksum(ushort *addr, const size_t len);

/** 一行読込 */
uchar *read_line(FILE *fp);

#endif /* _UTIL_H_ */

