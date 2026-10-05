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

#include <stdlib.h> /* realloc */
#include <string.h> /* memcpy memset */

#include "log.h"
#include "memfree.h"
#include "readline.h"

/* gcc 13 以降の -fanalyzer が, total が 0 のとき (alloc が確保した大きさより前) を読むと誤検知する.
 * total > 0u を確認してから読み書きしているので, この関数の中だけ, 抑える. */
#if defined(__GNUC__) && __GNUC__ >= 13
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wanalyzer-out-of-bounds"
#endif
/**
 * @brief 一行読込
 *
 * @param[in] fp ファイルポインタ
 * @return 文字列
 * @retval NULL エラー
 */
unsigned char *
_readline(FILE *fp)
{
    char *fgetsp = NULL;         /* fgets戻り値 */
    size_t length = 0u;          /* 文字列長 */
    size_t total = 0u;           /* 文字列長全て */
    unsigned char *alloc = NULL; /* reallocバッファ */
    unsigned char *tmp = NULL;   /* 一時ポインタ */
    unsigned char buf[FGETSBUF]; /* fgetsバッファ */

    /* fgetsのファイルポインタにNULLを渡した場合,
     * crashするかもしれない */
    if (fp == NULL)
        return NULL;

    do {
        (void)memset(buf, 0, sizeof(buf));
        fgetsp = fgets((char *)buf, sizeof(buf), fp);
        if (ferror(fp) != 0) { /* エラー */
            outlog("fgets=%p", (const void *)fgetsp);
            clearerr(fp);
            goto error_handler;
        }
        dbglog("fgets=%p, feof=%d", fgetsp, feof(fp));
        if (fgetsp == NULL) /* 入力の終わり (何も読めなかった) */
            break;

        length = strlen((char *)buf);
        dbgdump(buf, length, "buf=%p, length=%zu", buf, length);

        tmp = (unsigned char *)realloc(alloc, (total + length + 1u) * sizeof(unsigned char));
        if (tmp == NULL) {
            outlog("realloc: total+length+1=%zu", total + length + 1u);
            goto error_handler;
        }
        alloc = tmp;
        (void)memset(alloc + total, 0, (length + 1u) * sizeof(unsigned char));

        (void)memcpy(alloc + total, buf, length * sizeof(unsigned char));

        total += length;
        dbglog("alloc=%p, length=%zu, total=%zu", alloc + total, length * sizeof(unsigned char),
               total);

    } while (!((total > 0u) && (*(alloc + total - 1u) == '\n')) && (feof(fp) == 0));

    /* 改行なしで終わった最後の行も, 返す. 何も読めなかったときは, NULL */
    if ((alloc != NULL) && (total > 0u) && (*(alloc + total - 1u) == '\n'))
        *(alloc + total - 1u) = '\0'; /* 改行削除 */

    return alloc;

error_handler:
    memfree(&alloc, NULL); /* それまでに読んだ分を解放する */
    return NULL;
}
#if defined(__GNUC__) && __GNUC__ >= 13
#  pragma GCC diagnostic pop
#endif
