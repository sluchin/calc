/**
 * @file lib/fileio.c
 * @brief ファイルIO
 *
 * @author higashi
 * @date 2011-12-20 higashi 新規作成
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

#include <stdio.h>   /* fflush fdopen */
#include <unistd.h>  /* read write */
#include <stdbool.h> /* bool */
#include <fcntl.h>   /* open */
#include <stdarg.h>  /* va_start va_arg va_end */
#include <errno.h>   /* errno */

#include "def.h"
#include "log.h"
#include "fileio.h"

/**
 * 受信
 *
 * @param[in] fd ソケット
 * @param[in] vptr 受信バッファ
 * @param[in] n バッファ長さ
 * @retval EX_NG エラー
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
                return EX_NG;
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
 * @retval EX_NG エラー
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
                return EX_NG;
        }
        nleft -= nwritten;
        ptr += nwritten;
    }
    return n;
}

/**
 * パイプ複製
 *
 * @param[in] fd ファイルディスクリプタ
 * @retval EX_NG エラー
 * @return ファイルディスクリプタ
 */
int
pipe_fd(const int fd)
{
    int pfd[] = {-1, -1}; /* pipe */
    int retval = 0;       /* 戻り値 */
    int newfd = 0;        /* dup2戻り値 */

    if (fd < 0)
        return EX_NG;

    retval = pipe(pfd);
    if (retval < 0) {
        outlog("pipe: pfd=%p", pfd);
        return EX_NG;
    }

    retval = close(fd);
    if (retval < 0) {
        outlog("close: fd=%d", fd);
        return EX_NG;
    }

    newfd = dup2(pfd[PIPE_W], fd);
    if (newfd < 0) {
        outlog("dup2: pfd[PIPE_W]=%d, fd=%d", pfd[PIPE_W], fd);
        close_fd(&pfd[PIPE_R], &pfd[PIPE_W], NULL);
        return EX_NG;
    }
    dbglog("newfd=%d, pfd[PIPE_W]=%d, fd=%d", newfd, pfd[PIPE_W], fd);

    close_fd(&pfd[PIPE_W], NULL);

    return pfd[PIPE_R];
}

/**
 * パイプ複製 2
 *
 * @param[in] pipefd パイプ
 * @param[in] oldfd コピー元
 * @param[in] newfd コピー先
 */
int
pipe_fd2(int *pipefd, int *oldfd, const int newfd)
{
    int retval = 0; /* 戻り値 */
    int fd = 0;     /* dup戻り値 */

    close_fd(pipefd, NULL);

    retval = close(newfd);
    if (retval < 0)
        outlog("close=%d", retval);

    fd = dup2(*oldfd, newfd);
    if (fd < 0) {
        outlog("dup2=%d", retval);
        close_fd(oldfd, NULL);
        return EX_NG;
    }

    close_fd(oldfd, NULL);

    return fd;
}

/**
 * リダイレクト
 *
 * @param[in] fd ファイルディスクリプタ
 * @param[in] path ファイルパス
 * @retval EX_NG エラー
 */
int
redirect(int fd, const char *path)
{
    FILE *fp = NULL; /* ファイルポインタ */
    int f = 0;       /* ファイルディスクリプタ */

    if (fd < 0 || !path)
        return EX_NG;

    /* フラッシュする */
    fp = fdopen(fd, "a+");
    if (!fp) { /* エラー */
        outlog("fdopen: fd=%d", fd);
    } else {
        if (fflush(fp) == EOF)
            outlog("fflush: %p", fp);
    }

    /* 書込権限の確認 */
    if (access(path, W_OK) < 0) {
        outlog("access: %s", path);
        return EX_NG;
    }

    f = open(path, O_WRONLY|O_APPEND);
    if (f < 0) {
        outlog("open=%d", f);
        return EX_NG;
    }

    if (close(fd) < 0)
        outlog("close: fd=%d", fd);

    if (dup2(f, fd) < 0) {
        outlog("dup2");
        return EX_NG;
    }

    if (close(f) < 0)
        outlog("close: f=%d", f);

    return EX_OK;
}

/**
 * クローズ
 *
 * @param[in,out] fd ファイルディスクリプタ
 * @param[in] ... 可変引数
 * @retval EX_NG エラー
 * @attention 最後の引数はNULLにすること.
 */
int
close_fd(int *fd, ...)
{
    va_list ap;       /* va_list */
    int *ptr = NULL;  /* ポインタ */
    bool err = false; /* エラーフラグ */

    dbglog("start");

    if (fd && *fd >= 0) {
        dbglog("%p fd=%d", fd, *fd);
        if (close(*fd) < 0) {
            outlog("close: fd=%d", *fd);
            err = true;
        }
    }
    *fd = -1;

    va_start(ap, fd);

    while ((ptr = va_arg(ap, int *)) != NULL) {
        dbglog("%p ptr=%d", ptr, *ptr);
        if (*ptr >= 0) {
            if (close(*ptr) < 0) {
                outlog("close: ptr=%d", *ptr);
                err = true;
            }
        }
        *ptr = -1;
    }
    va_end(ap);

    if (err)
        return EX_NG;

    return EX_OK;
}

