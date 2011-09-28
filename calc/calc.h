/**
 * @file  calc/calc.h
 * @brief 関数電卓インタプリタ
 *
 * @author higashi
 * @date 2010-06-27 higashi 新規作成
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

#ifndef _CALC_H_
#define _CALC_H_

#include <stdbool.h> /* bool */

#include "def.h"
#include "function.h"

// test
//#define MAX_DIGIT      15L /**< 有効桁数最大値 */
#define MAX_DIGIT      30L /**< 有効桁数最大値 */
#define DEFAULT_DIGIT  12L /**< 有効桁数デフォルト値 */

/* 外部変数 */
extern bool g_tflag; /**< tオプションフラグ */

/** 初期化 */
void init_calc(void *buf, long digit);

/** メモリ解放 */
void destroy_calc(void);

/** 入力 */
uchar *answer(void);

/** 引数解析 */
void parse_func_args(const enum argtype num, dbl *x, dbl *y);

#ifdef _UT
struct test_calc_func {
    dbl (*expression)(void);
    dbl (*term)(void);
    dbl (*factor)(void);
    dbl (*token)(void);
    dbl (*number)(void);
    int (*get_strlen)(const dbl val, const char *fmt);
    void (*readch)(void);
};
void test_init_calc(struct test_calc_func *func);
#endif

/**
 * @mainpage 処理詳細
 *
 * @section server サーバ
 * -# ポート番号の設定\n
 * -# ソケット生成\n
 * -# ソケットオプション設定\n
 * -# ソケットにアドレス設定(bind)\n
 * -# アクセスバックログの設定(listen)\n
 * -# 接続受付(accept)\n
 * -# スレッド生成(server_proc)\n
 * -# ヘッダ受信\n
 * -# データ受信\n
 * -# データのチェックサムをチェック\n
 * -# サーバ処理\n
 * -# データ送信\n
 *
 * @section client クライアント
 * -# IPアドレスまたはホスト名の設定\n
 * -# ポート番号の設定\n
 * -# ソケット生成\n
 * -# サーバに接続(connect)\n
 * -# 標準入力から文字列取得\n
 * -# データ送信\n
 * -# ヘッダ受信\n
 * -# データ受信\n
 * -# データのチェックサムをチェック\n
 * -# 標準出力に文字列出力\n
 */
/**
 * @page page1 処理シーケンス
 * @msc
 * server,calc,client;
 *
 * server<-server[label="server_sock()"];
 * server<-server[label="server_loop()"];
 * server<-server[label="server_proc()"];
 * client<-client[label="connect_sock()"];
 * client<-client[label="client_loop()"];
 * client box client[label="stdin"];
 * server<-client[label="send_data()"];
 * server<-server[label="recv_data()"];
 * server->calc[label="answer()"];
 *
 * calc->calc[label="expression()"];
 * calc->calc[label="term()"];
 * calc->calc[label="factor()"];
 * calc->calc[label="number()"];
 * calc->calc[label="expression()"];
 * ...;
 *
 * server<<calc[label="result"];
 * server->client[label="send_data()"];
 * client<-client[label="recv_data()"];
 * client box client[label="stdout"];
 *
 * @endmsc
 */
#endif /* _CALC_H_ */

