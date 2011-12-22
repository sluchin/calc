/**
 * @file lib/tests/test_common.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-12-20 higashi 新規作成
 * @version \Id
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

#include <unistd.h> /* read write */
#include <errno.h>  /* errno */
#include <cutter.h> /* cutter library */

#include "log.h"
#include "test_common.h"

#define EX_ERROR -1 /**< エラー戻り値 */

/**
 * 受信
 *
 * @param[in] fd ソケット
 * @param[in] vptr 受信バッファ
 * @param[in] n バッファ長さ
 * @return 受信されたバイト数
 */
ssize_t
readn(int fd, void *vptr, size_t n)
{
    size_t nleft = 0;  /* 受信する残りのバイト数 */
    ssize_t nread = 0; /* 受信されたバイト数 */
    char *ptr = NULL;  /* ポインタ */

    ptr = (char *)vptr;
    nleft = n;
    while (nleft > 0) {
        nread = read(fd, ptr, nleft);
        if (nread < 0) {
            if (errno == EINTR)
                nread = 0;
            else
                return EX_ERROR;
        } else if (nread == 0) {
            break;
        }
        nleft -= nread;
        ptr += nread;
    }
    return n - nleft;
}

/**
 * 送信
 *
 * @param[in] fd ソケット
 * @param[in] vptr 送信バッファ
 * @param[in] n バッファ長さ
 * @return 送信されたバイト数
 */
ssize_t
writen(int fd, const void *vptr, size_t n)
{
    size_t nleft = 0;       /* 送信する残りのバイト数 */
    ssize_t nwritten = 0;   /* 送信されたバイト数 */
    const char *ptr = NULL; /* ポインタ */

    ptr = (char *)vptr;
    nleft = n;
    while (nleft > 0) {
        nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0) {
            if (errno == EINTR)
                nwritten = 0;
            else
                return EX_ERROR;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

/**
 * ファイルディスクリプタクローズ
 *
 * @param[in,out] fd ファイルディスクリプタ
 * @param[in] ... 可変引数
 * @return なし
 */
void
close_fd(int *fd, ...)
{
    va_list ap;      /* va_list */
    int retval = 0;  /* 戻り値 */
    int *ptr = NULL; /* ポインタ */

    dbglog("start");

    if (fd && *fd >= 0) {
        dbglog("%p fd=%d", fd, *fd);
        retval = close(*fd);
        if (retval)
            cut_notify("close=%d", retval);
    }
    *fd = -1;

    va_start(ap, fd);

    while ((ptr = va_arg(ap, int *)) != NULL) {
        dbglog("%p ptr=%d", ptr, *ptr);
        if (*ptr >= 0) {
            retval = close(*ptr);
            if (retval < 0)
                cut_notify("close=%d", retval);
        }
        *ptr = -1;
    }

    va_end(ap);
}

