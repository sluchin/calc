/**
 * @file  server/server.h
 * @brief ソケット送受信
 *
 * @author higashi
 * @date 2010-06-26 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
 */

#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdbool.h>   /* bool */
#include <signal.h>    /* sig_atomic_t */
#include <signal.h>    /* sig_atomic_t */
#include <arpa/inet.h> /* struct sockaddr_in inet_addr */

#include "data.h"
#include "calc.h"

#define HOST_SIZE 48           /**< ホスト名サイズ */
#define PORT_SIZE  6           /**< ポート名サイズ */
#define DEFAULT_PORTNO "12345" /**< デフォルトポート番号 */


/* 外部変数 */
extern volatile sig_atomic_t g_sig_handled; /**< シグナル */
extern bool g_gflag;                        /**< gオプションフラグ */

/** ソケット情報構造体 */
struct _thread_data {
    int sock;                /**< ソケット */
    struct sockaddr_in addr; /**< sockaddr_in構造体 */
    socklen_t len;           /**< 長さ */
    sigset_t sigmask;        /**< シグナルマスク */
};
typedef struct _thread_data thread_data;

/** ポート番号文字列設定 */
int set_port_string(const char *port);

/** ソケット接続 */
int server_sock();

/** 接続受付 */
void server_loop(int sock);

#ifdef UNITTEST
struct _testserver {
    void *(*server_proc)(void *arg);
};
typedef struct _testserver testserver;

void test_init_server(testserver *server);
#endif /* UNITTEST */

#endif /* _SERVER_H_ */

