/**
 * @file  server/option.h
 * @brief オプション引数の処理
 *
 * @author higashi
 * @date 2010-06-25 higashi 新規作成
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

#ifndef _SERVER_OPTION_H_
#define _SERVER_OPTION_H_

#include <stdbool.h> /* bool */

#define DEFAULT_IPADDR "127.0.0.1" /**< デフォルトのIPアドレス */
#define DEFAULT_PORTNO "12345"     /**< デフォルトのポート番号 */
#define HOST_SIZE      48          /**< ホスト名サイズ */
#define PORT_SIZE      48          /**< ポート名サイズ */
#define MAX_DIGIT      30L         /**< 有効桁数最大値 */
#define DEFAULT_DIGIT  12L         /**< 有効桁数デフォルト値 */

/* 外部変数 */
extern bool g_gflag;              /**< gオプションフラグ */
extern char g_port_no[PORT_SIZE]; /**< ポート番号またはサービス名 */
extern long g_digit;              /**< 桁数 */

/** オプション引数 */
void parse_args(int argc, char *argv[]);

#endif /* _SERVER_OPTION_H_ */

