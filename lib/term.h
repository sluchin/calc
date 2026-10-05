/**
 * @file  lib/term.h
 * @brief ターミナル属性の取得・設定
 *
 * @author higashi
 * @date 2011-01-06 higashi 新規作成
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

#ifndef TERM_H
#define TERM_H

#include <termios.h> /* termios */

/** モードタイプ */
enum mode_type { control = 0, input, output, local };

/* ターミナル属性シスログ出力 */
void sys_print_termattr(const int level,
                        const int option,
                        const char *pname,
                        const char *fname,
                        const int line,
                        const char *func,
                        int fd);

#ifdef UNITTEST
/** 内部関数の関数ポインタ構造体 (単体テスト用) */
struct _testterm {
    char *(*get_termattr)(const int fd, struct termios *mode); /**< 端末属性の文字列化 */
    tcflag_t *(*mode_type_flag)(const enum mode_type type, struct termios *mode); /**< フラグ取得 */
};
/** 内部関数の関数ポインタ構造体型 (単体テスト用) */
typedef struct _testterm testterm;

/* 単体テスト用の関数構造体の初期化 (内部関数を, テストから呼べるようにする) */
void test_init_term(testterm *term);

#endif /* UNITTEST */

#endif /* TERM_H */
