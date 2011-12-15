/**
 * @file  client/client.h
 * @brief ソケット送受信
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

#ifndef _CLIENT_H_
#define _CLIENT_H_

#include <stdbool.h> /* bool */
#include <signal.h>  /* sig_atomic_t sigaction */

/* 外部変数 */
extern volatile sig_atomic_t sig_handled; /**< シグナル */
extern struct sigaction g_sigaction;      /**< sigaction構造体 */

/** ソケット接続 */
int connect_sock(const char *host, const char *port);

/** ソケット送受信 */
void client_loop(int sock);

#endif /* _CLIENT_H_ */

