/**
 * @file  calc/tests/test_common.h
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-11-08 higashi 新規作成
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

#ifndef _TEST_COMMON_H_
#define _TEST_COMMON_H_

#include "calc.h"

#define MAX_STRING  32 /**< 最大文字列 */

/** 文字列設定 */
void set_string(calcinfo *calc, const char *str);

#endif /* _TEST_COMMON_H_ */

