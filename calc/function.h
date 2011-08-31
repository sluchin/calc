/**
 * @file  function.h
 * @brief 関数
 *
 * @sa function.c
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

#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "def.h"
#include "calc.h"

/** 関数最大文字数 */
#define MAX_FUNC_STRING    4

/** 関数実行 */
ldfl exec_func(struct arg_value *val, const char *func);

#endif /* _FUNCTION_H_ */

