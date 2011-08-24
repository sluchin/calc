/**
 * @file  data.h
 * @brief 送受信データ構造体
 *
 * @sa data.c
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

#ifndef _DATA_H_
#define _DATA_H_

//#define BUF_SIZE  (int)256  /**< 送受信バッファ最大バイト数 */
#define BUF_SIZE  (int)6

/**
 * ヘッダー構造体
 */
struct header {
    size_t length;           /**< データ長 */
    unsigned short checksum; /**< チェックサム */
    unsigned short pack;     /**< パッキング */
};

/**
 * クライアントデータ構造体
 */
struct client_data {
    struct header hd;                   /**< ヘッダー構造体 */
    unsigned char expression[BUF_SIZE]; /**< データバッファ */
};

/**
 * サーバデータ構造体
 */
struct server_data {
    struct header hd;               /**< ヘッダー構造体 */
    unsigned char answer[BUF_SIZE]; /**< データバッファ */
};

/**
 * クライアントデータ構造体設定
 *
 * @param[in,out] dt 送受信データ構造体
 * @param[in] buf 送受信バッファ
 * @param[in] len 長さ
 * @return なし
 */
void
set_client_data(struct client_data *dt,
                const unsigned char *buf, const size_t len);

/**
 * サーバデータ構造体設定
 *
 * @param[in,out] dt 送受信データ構造体
 * @param[in] buf 送受信バッファ
 * @param[in] len 長さ
 * @return なし
 */
void
set_server_data(struct server_data *dt,
                const unsigned char *buf, const size_t len);

#endif /* _DATA_H_ */

