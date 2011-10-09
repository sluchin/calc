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
#include "calc.h"
#include "function.h"

/** エラー種別 */
enum ER {
    E_NONE = 0,  /**< エラーなし */
    E_DIVBYZERO, /**< ゼロ除算エラー */
    E_SYNTAX,    /**< 文法エラー */
    E_NOFUNC,    /**< 関数名なし */
    E_NAN,       /**< 定義域エラー */
    E_INFINITY,  /**< 値域エラー */
    MAXERROR     /**< エラーコード最大数 */
};

typedef enum ER ER;

/** エラーメッセージ取得 */
uchar *get_errormsg(calcinfo *tsd);

/** エラーコード設定 */
void set_errorcode(calcinfo *tsd, ER error);

/** エラークリア */
void clear_error(calcinfo *tsd);

/** エラー判定 */
bool is_error(calcinfo *tsd);

/** 数値の妥当性チェック */
void check_validate(calcinfo *tsd, dbl val);

/** 浮動小数点例外チェック */
void check_math_feexcept(calcinfo *tsd, dbl val);

#endif /* _ERROR_H_ */

