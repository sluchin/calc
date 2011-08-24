/**
 * @file  calc.h
 * @brief 関数電卓インタプリタ
 *
 * @sa calc.c
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

#include "common.h"

#define MAX_PREC      (long int)15 /**< 小数点以下の有効桁数最大値 */
#define DEFAULT_PREC  (long int)12 /**< デフォルト小数点以下の有効桁数 */

extern long int precision;

/** 引数の数 */
enum {
    ARG_0 = 0,
    ARG_1,
    ARG_2
};

/** 数学関数の引数に渡す値 */
struct arg_value {
    ldfl x;
    ldfl y;
} value;

/** 入力 */
uchar *input(uchar *buf, const size_t len);

/** 引数解析 */
void parse_func_args(struct arg_value *val, const int argnum);

#endif /* _CALC_H_ */

