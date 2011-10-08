/**
 * @file  lib/readline.c
 * @brief 一行読込
 *
 * @author higashi
 * @date 2010-09-10 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010 Tetsuya Higashi. All Rights Reserved.
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

#include <stdlib.h>  /* realloc free */
#include <string.h>  /* memcpy memset */
#include <errno.h>   /* errno */

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
    char *retval = NULL; /* fgets戻り値 */
    size_t length = 0;   /* 文字列長 */
    size_t total = 0;    /* 文字列長全て */
    uchar *line = NULL;  /* 文字列 */
    uchar *tmp = NULL;   /* 一時アドレス */
    uchar buf[MAXLINE];  /* バッファ */

    do {
        (void)memset(buf, 0, sizeof(buf));
        retval = fgets((char *)buf, sizeof(buf), fp);
        if (!retval || feof(fp) || ferror(fp)) { /* エラー */
            outlog("fgets=%p", retval);
            clearerr(fp);
            break;
        }
        length = strlen((char *)buf);
        dbgdump(buf, length, "buf=%u", length);

        tmp = (uchar *)realloc(line, (total + length + 1) * sizeof(uchar));
        if (!tmp) {
            outlog("realloc=%p, total+length+1=%u", line, total + length + 1);
            break;
        }
        line = tmp;

        (void)memcpy(line + total, buf, length + 1);

        total += length;
        dbglog("length=%u, total=%u", length, total);
        dbglog("line=%p: %s", line, line);
        dbgdump(line, total + 1, "line=%u", total + 1);

    } while ((*(line + total - 1) != '\n') && (*(line + total) != '\0'));

    if (line && *(line + total - 1) == '\n')
        *(line + total - 1) = '\0'; /* 改行削除 */

    return line;
}

