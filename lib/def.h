/**
 * @file lib/def.h
 * @brief 汎用定義
 *
 * @author higashi
 * @date 2011-08-22 higashi 新規作成
 * @version \$Id$
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

#ifndef _DEF_H_
#define _DEF_H_

/** 配列要素数 */
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

/* 型定義 */
typedef unsigned char uchar;   /**< unsigned char */
typedef unsigned int uint;     /**< unsigned int */
typedef unsigned short ushort; /**< unsigned short */
typedef unsigned long ulong;   /**< unsigned long */
typedef double dbl;            /**< double */
typedef long double ldbl;      /**< long double */

/** 関数戻り値 */
enum {
    EX_NG = -1, /**< エラー時 */
    EX_OK =  0  /**< 正常時 */
};

#endif /* _DEF_H_ */

