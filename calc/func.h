/**
 * @file  calc/func.h
 * @brief 関数
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

#ifndef _FUNCTION_H_
#define _FUNCTION_H_

#include "def.h"
#include "calc.h"

/** 関数最大文字数 */
#define MAX_FUNC_STRING    4

/** 関数実行 */
dbl exec_func(calcinfo *calc, const char *func);

/** 指数取得 */
dbl get_pow(calcinfo *calc, dbl x, dbl y);

#ifdef UNITTEST
struct _testfunc {
    dbl (*get_pi)(calcinfo *calc);
    dbl (*get_e)(calcinfo *calc);
    dbl (*get_rad)(calcinfo *calc, dbl x);
    dbl (*get_deg)(calcinfo *calc, dbl x);
    dbl (*get_sqrt)(calcinfo *calc, dbl x);
    dbl (*check_math)(calcinfo *calc, dbl x, dbl (*callback)(dbl));
    dbl (*get_factorial)(calcinfo *calc, dbl n);
    dbl (*get_permutation)(calcinfo *calc, dbl n, dbl r);
    dbl (*get_combination)(calcinfo *calc, dbl n, dbl r);
};
typedef struct _testfunc testfunc;

void test_init_func(testfunc *func);
#endif /* UNITTEST */

#endif /* _FUNCTION_H_ */

