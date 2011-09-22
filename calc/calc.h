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

#define MAX_DIGIT      15 /**< 有効桁数最大値 */
#define DEFAULT_DIGIT  12 /**< 有効桁数デフォルト値 */

/* 外部変数 */
extern int digit;      /**< 桁数 */
extern bool tflag;     /**< tオプションフラグ */

/** 入力 */
uchar *input(uchar *buf, const size_t len);

/** 引数解析 */
void parse_func_args(const enum argtype num, dbl *x, dbl *y);

#ifdef _UT
int test_get_digit(const dbl val, const char *fmt);
#endif

#endif /* _CALC_H_ */

