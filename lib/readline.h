/**
 * @file  lib/readline.h
 * @brief 一行読込
 *
 * @author higashi
 * @date 2010-06-24 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
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

#ifndef CALCUTIL_READLINE_H
#define CALCUTIL_READLINE_H

#include <stdio.h> /* FILE */

#include "def.h"

#define FGETSBUF 1024u /**< バッファサイズ */

/* 一行読込 */
unsigned char *_readline(FILE *fp);

#endif /* CALCUTIL_READLINE_H */
