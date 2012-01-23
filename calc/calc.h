/**
 * @file  calc/calc.h
 * @brief 関数電卓インタプリタ
 *
 * @author higashi
 * @date 2010-06-27 higashi 新規作成
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

#ifndef _CALC_H_
#define _CALC_H_

#include <stdbool.h> /* bool */

#include "def.h"

#ifdef _DEBUG
#  define MAX_DIGIT    30L /**< 有効桁数最大値 */
#else
#  define MAX_DIGIT    15L /**< 有効桁数最大値 */
#endif /* _DEBUG */
#define DEFAULT_DIGIT  12L /**< 有効桁数デフォルト値 */

/* 外部変数 */
extern bool g_tflag; /**< tオプションフラグ */

/** 引数の数 */
enum _argtype {
    ARG_0 = 0,
    ARG_1,
    ARG_2
};
typedef enum _argtype argtype;

/** エラー種別 */
enum _ER {
    E_NONE = 0,  /**< エラーなし */
    E_DIVBYZERO, /**< ゼロ除算エラー */
    E_SYNTAX,    /**< 文法エラー */
    E_NOFUNC,    /**< 関数名なし */
    E_NAN,       /**< 領域エラー */
    E_INFINITY,  /**< 極エラーまたは範囲エラー */
    MAXERROR     /**< エラーコード最大数 */
};
typedef enum _ER ER;

/** calc情報構造体 */
struct _calcinfo {
    int ch;                    /**< 文字 */
    uchar *ptr;                /**< 文字列走査用ポインタ */
    uchar *result;             /**< 結果文字列 */
    char fmt[sizeof("%.18g")]; /**< フォーマット */
    ER errorcode;              /**< エラーコード */
};
typedef struct _calcinfo calcinfo;

/** calcinfo構造体生成 */
calcinfo *create_calc(calcinfo *tsd, void *expr, long digit);

/** calcinfo構造体生成(スレッドセーフ版) */
calcinfo *create_calc_r(void *expr, long digit);

/** メモリ解放 */
void destroy_calc(void *tsd);

/** 入力 */
uchar *answer(calcinfo *tsd);

/** 引数解析 */
void parse_func_args(calcinfo *tsd, const argtype num, dbl *x, dbl *y);

#ifdef UNITTEST
struct _testcalc {
    dbl (*expression)(calcinfo *tsd);
    dbl (*term)(calcinfo *tsd);
    dbl (*factor)(calcinfo *tsd);
    dbl (*token)(calcinfo *tsd);
    dbl (*number)(calcinfo *tsd);
    int (*get_strlen)(const dbl val, const char *fmt);
    void (*readch)(calcinfo *tsd);
};
typedef struct _testcalc testcalc;

void test_init_calc(testcalc *calc);
#endif /* UNITTEST */

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

