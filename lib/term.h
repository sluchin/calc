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

#ifndef _TERM_H_
#define _TERM_H_

#include <termios.h> /* termios */

/** モードタイプ */
enum mode_type {
    control = 0,
    input,
    output,
    local
};

/** ターミナル属性シスログ出力 */
void sys_print_termattr(const int level, const int option,
                        const char *pname, const char *fname,
                        const int line, const char *func, int fd);

#ifdef UNITTEST
struct _testterm {
    char *(*get_termattr)(const int fd, struct termios *mode);
    tcflag_t *(*mode_type_flag)(const enum mode_type type,
                                struct termios *mode);
};
typedef struct _testterm testterm;

void test_init_term(testterm *term);

#endif /* UNITTEST */

#endif /* _TERM_H_ */

