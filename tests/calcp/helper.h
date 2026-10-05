/**
 * @file  tests/calcp/helper.h
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-11-08 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011-2018 Tetsuya Higashi. All Rights Reserved.
 */
/* This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef HELPER_H
#define HELPER_H

#include "calc.h"

#define MAX_STRING 32 /**< 最大文字列 */

/* 文字列設定 */
void set_string(calcinfo *calc, const char *str);
/* set_string() で確保した文字列の解放 */
void free_strings(void);

#endif /* HELPER_H */
