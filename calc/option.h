/**
 * @file  calc/option.h
 * @brief オプション引数の処理
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

#ifndef _CALC_OPTION_H_
#define _CALC_OPTION_H_

#define DEFAULT_IPADDR "192.168.0.5" /**< デフォルトのIPアドレス */
#define DEFAULT_PORTNO "12345"       /**< デフォルトのポート番号 */
#define MAX_DIGIT      30L           /**< 有効桁数最大値 */
#define DEFAULT_DIGIT  12L           /**< 有効桁数デフォルト値 */

/* 外部変数 */
extern long g_digit; /**< 桁数 */

/** オプション引数 */
void parse_args(int argc, char *argv[]);

#endif /* _CALC_OPTION_H_ */

