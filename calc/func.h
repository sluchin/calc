/**
 * @file  calc/func.h
 * @brief 関数
 *
 * @author higashi
 * @date 2011-08-15 higashi 新規作成
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

#ifndef FUNCTION_H
#define FUNCTION_H

#include "def.h"
#include "calc.h"

/** 関数最大文字数 */
#define MAX_FUNC_STRING 4U

/* 関数実行 */
double exec_func(calcinfo *calc, const char *func);

/* 指数取得 */
double get_pow(calcinfo *calc, double x, double y);

#ifdef UNITTEST
/** 内部関数の関数ポインタ構造体 (単体テスト用) */
struct _testfunc {
    double (*get_pi)(calcinfo *calc);                              /**< 円周率 */
    double (*get_e)(calcinfo *calc);                               /**< ネイピア数 */
    double (*get_rad)(calcinfo *calc, double x);                   /**< 角度をラジアンに変換 */
    double (*get_deg)(calcinfo *calc, double x);                   /**< ラジアンを角度に変換 */
    double (*get_sqrt)(calcinfo *calc, double x);                  /**< 平方根 */
    double (*get_ln)(calcinfo *calc, double x);                    /**< 自然対数 */
    double (*get_log)(calcinfo *calc, double x);                   /**< 常用対数 */
    double (*get_factorial)(calcinfo *calc, double n);             /**< 階乗 */
    double (*get_permutation)(calcinfo *calc, double n, double r); /**< 順列 */
    double (*get_combination)(calcinfo *calc, double n, double r); /**< 組み合わせ */
};
/** 内部関数の関数ポインタ構造体型 (単体テスト用) */
typedef struct _testfunc testfunc;

/* 単体テスト用の関数構造体の初期化 (内部関数をテストから呼べるようにする) */
void test_init_func(testfunc *func);
#endif /* UNITTEST */

#endif /* FUNCTION_H */
