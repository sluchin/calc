/**
 * @file  calc/error.h
 * @brief エラー設定取得
 *
 * @author higashi
 * @date 2011-08-15 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011-2018 Tetsuya Higashi. All Rights Reserved.
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
#include "func.h"

/** エラーメッセージ取得 */
unsigned char *get_errormsg(calcinfo *calc);

/** エラーコード設定 */
void set_errorcode(calcinfo *calc, ER error);

/** エラークリア */
void clear_error(calcinfo *calc);

/** エラー判定 */
bool is_error(calcinfo *calc);

/** 数値の妥当性チェック */
void check_validate(calcinfo *calc, double val);

/** 浮動小数点例外チェック */
void check_math_feexcept(calcinfo *calc);

/** 浮動小数点例外チェッククリア */
void clear_math_feexcept(void);

#ifdef UNITTEST
struct _testerror {
    const char **errormsg;
};
typedef struct _testerror testerror;

void test_init_error(testerror *error);
#endif /* UNITTEST */

#endif /* _ERROR_H_ */

