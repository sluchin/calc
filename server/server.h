/**
 * @file  server/server.h
 * @brief ソケット送受信
 *
 * @author higashi
 * @date 2010-06-26 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010 Tetsuya Higashi. All Rights Reserved.
 */

#ifndef _SERVER_H_
#define _SERVER_H_

#include <stdbool.h> /* bool */
#include <signal.h>  /* sig_atomic_t */

/* 外部変数 */
extern volatile sig_atomic_t sig_handled; /**< シグナル */
extern struct sigaction g_sigaction;      /**< sigaction構造体 */

/** ソケット接続 */
int server_sock(const char *port);

/** 接続受付 */
void server_loop(int sock);

#endif /* _SERVER_H_ */

