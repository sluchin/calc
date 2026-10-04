/**
 * @file lib/def.h
 * @brief 汎用定義
 *
 * @author higashi
 * @date 2011-08-22 higashi 新規作成
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

#ifndef DEF_H
#define DEF_H

/** 配列要素数 */
#define NELEMS(array) (sizeof(array) / sizeof(array[0]))

/** 関数戻り値 */
enum {
    EX_NG = -1, /**< エラー時 */
    EX_OK = 0   /**< 正常時 */
};

#endif /* DEF_H */
