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

#include "def.h"

#define HOST_SIZE 48 /**< ホスト名サイズ */
#define PORT_SIZE  6 /**< ポート名サイズ */

/* 外部変数 */
extern volatile sig_atomic_t g_sig_handled; /**< シグナル */
extern struct sigaction g_sigaction;        /**< sigaction構造体 */
extern bool g_gflag;                        /**< gオプションフラグ */
extern bool g_tflag;                        /**< tオプションフラグ */

/** ステータス */
enum _st_client {
    EX_SUCCESS = 0, /* 正常 */
    EX_FAILURE,     /* 異常 */
    EX_EMPTY,       /* 空の文字列 */
    EX_QUIT,        /* quitまたはexit文字列 */
    EX_CONNECT_ERR, /* 接続できない */
    EX_ALLOC_ERR,   /* メモリ不足 */
    EX_SEND_ERR,    /* 送信エラー */
    EX_RECV_ERR,    /* 受信エラー */
    EX_SIGNAL,      /* シグナル受信した */
};
typedef enum _st_client st_client;

/** ポート番号文字列設定 */
int set_port_string(const char *port);

/** ホスト名文字列設定 */
int set_host_string(const char *host);

/** ソケット接続 */
int connect_sock(void);

/** ソケット送受信 */
st_client client_loop(int sock);

#endif /* _CLIENT_H_ */

#ifdef UNITTEST
struct _testclient {
    st_client (*send_sock)(int sock);
    st_client (*read_sock)(int sock);
};
typedef struct _testclient testclient;

void test_init_client(testclient *client);
#endif /* UNITTEST */

