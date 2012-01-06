/**
 * @file  lib/readline.c
 * @brief 一行読込
 *
 * @author higashi
 * @date 2010-09-10 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010-2011 Tetsuya Higashi. All Rights Reserved.
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

#include <stdlib.h> /* realloc free */
#include <string.h> /* memcpy memset */
#include <errno.h>  /* errno */

#include "log.h"
#include "readline.h"

/**
 * 一行読込
 *
 * @param[in] fp ファイルポインタ
 * @return 文字列
 * @retval NULL エラー
 */
uchar *
_readline(FILE *fp)
{
    char *fgetsp = NULL; /* fgets戻り値 */
    size_t length = 0;   /* 文字列長 */
    size_t total = 0;    /* 文字列長全て */
    uchar *alloc = NULL; /* reallocバッファ */
    uchar *tmp = NULL;   /* 一時ポインタ */
    uchar buf[FGETSBUF]; /* fgetsバッファ */

    /* fgetsのファイルポインタにNULLを渡した場合,
     * crashするかもしれない */
    if (!fp)
        return NULL;

    do {
        (void)memset(buf, 0, sizeof(buf));
        fgetsp = fgets((char *)buf, sizeof(buf), fp);
        if (ferror(fp)) { /* エラー */
            outlog("fgets=%p", fgetsp);
            clearerr(fp);
            return NULL;
        }
        dbglog("fgets=%p, feof=%d", fgetsp, feof(fp));
        if (!fgetsp || feof(fp))
            break;

        length = strlen((char *)buf);
        dbgdump(buf, length, "buf=%p, length=%zu", buf, length);

        tmp = (uchar *)realloc(alloc, (total + length + 1) * sizeof(uchar));
        if (!tmp) {
            outlog("realloc: total+length+1=%zu", total + length + 1);
            return NULL;
        }
        alloc = tmp;

        (void)memcpy(alloc + total, buf, length + 1);

        total += length;
        dbglog("alloc=%p, length=%zu, total=%zu",
               alloc + total, length, total);

    } while (*(alloc + total - 1) != '\n');

    if (alloc && *(alloc + total - 1) == '\n')
        *(alloc + total - 1) = '\0'; /* 改行削除 */

    return alloc;
}

