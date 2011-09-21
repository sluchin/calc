/**
 * @file  calc/error.h
 * @brief エラー設定取得
 *
 * @author higashi
 * @date 2011-08-15 higashi 新規作成
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

#ifndef _ERROR_H_
#define _ERROR_H_

#include <stdbool.h> /* bool */

#include "def.h"
#include "function.h"

/** エラー種別 */
typedef enum {
    E_NONE = 0,  /**< エラーなし */
    E_DIVBYZERO, /**< ゼロ除算エラー */
    E_SYNTAX,    /**< 文法エラー */
    E_NOFUNC,    /**< 関数名なし */
    E_MATH,      /**< 計算不能なエラー */
    E_NAN,       /**< 数値ではない */
    E_OVERFLOW,  /**< オーバーフロー */
    E_UNDERFLOW, /**< アンダーフロー */
    E_PLUSINF,   /**< 正の無限大 */
    E_MINUSINF,  /**< 負の無限大 */
    E_NORMSMALL  /**< 小さすぎて正規化表現できない */
} ER;

/** エラーメッセージ取得 */
uchar *get_errormsg(void);

/** エラーコード設定 */
void set_errorcode(ER error);

/** エラークリア */
void clear_error(uchar **msg);

/** エラー判定 */
bool is_error(void);

/** 数値の妥当性チェック */
void check_validate(int num, ...);

/** 浮動小数点例外チェック */
void check_math_feexcept(dbl val);

#endif /* _ERROR_H_ */

