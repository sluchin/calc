/**
 * @file server/tests/test_server.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-24 higashi 新規作成
 * @version \Id
 *
 * Copyright (C) 2011 Tetsuya Higashi. All Rights Reserved.
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

//#include <unistd.h> /* pipe fork */
#include <cutter.h> /* cutter library */

#include "def.h"
#include "log.h"
#include "server.h"

/* 内部変数 */
static testserver server; /**< 関数構造体 */

/* プロトタイプ */
/** server_sock() 関数テスト */
void test_server_sock(void);
/** server_loop() 関数テスト */
void test_server_loop(void);
/** server_proc() 関数テスト */
void test_server_proc(void);

/* 内部関数 */

/**
 * 初期化処理
 *
 * @return なし
 */
void
cut_startup(void)
{
    (void)memset(&server, 0, sizeof(server));
    test_init_server(&server);
}

/**
 * test_server_sock() 関数テスト
 *
 * @return なし
 */
void
test_server_sock(void)
{

}

/**
 * test_server_loop() 関数テスト
 *
 * @return なし
 */
void
test_server_loop(void)
{

}

/**
 * test_server_proc() 関数テスト
 *
 * @return なし
 */
void
test_server_proc(void)
{

}

