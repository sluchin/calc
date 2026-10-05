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

#include <unistd.h>  /* read write */
#include <stdbool.h> /* bool */
#include <fcntl.h>   /* open */
#include <stdarg.h>  /* va_start va_arg va_end */
#include <errno.h>   /* errno */

#include "def.h"
#include "log.h"
#include "fileio.h"

/**
 * @brief 受信
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
    size_t nleft = 0U;  /* 受信する残りのバイト数 */
    ssize_t nread = 0L; /* 受信されたバイト数 */
    char *ptr = NULL;   /* ポインタ */

    ptr = (char *)vptr;
    nleft = n;
    /* n バイトを受信するまで繰り返す (read は, 少ないバイト数を返すことがある) */
    while (nleft > 0U) {
        nread = read(fd, ptr, nleft);
        if (nread < 0L) {
            if (errno == EINTR) /* 割り込まれたので, やり直す */
                nread = 0L;
            else
                return EX_NG;
        } else if (nread == 0L) { /* 入力の終わり */
            break;
        }
        nleft -= (size_t)nread;
        ptr += nread;
    }
    return (ssize_t)(n - nleft);
}

/**
 * @brief 送信
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
    size_t nleft = 0U;      /* 送信する残りのバイト数 */
    ssize_t nwritten = 0L;  /* 送信されたバイト数 */
    const char *ptr = NULL; /* ポインタ */

    ptr = (const char *)vptr;
    nleft = n;
    /* n バイトを送信するまで繰り返す (write は, 少ないバイト数を返すことがある) */
    while (nleft > 0U) {
        nwritten = write(fd, ptr, nleft);
        if (nwritten <= 0L) {
            if (errno == EINTR) /* 割り込まれたので, やり直す */
                nwritten = 0L;
            else
                return EX_NG;
        }
        nleft -= (size_t)nwritten;
        ptr += nwritten;
    }
    return (ssize_t)n;
}

/**
 * @brief パイプ複製
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

    /* パイプを作る */
    retval = pipe(pfd);
    if (retval < 0) {
        outlog("pipe: pfd=%p", (const void *)pfd);
        return EX_NG;
    }

    /* fd を閉じて, 書込側を fd に複製する (fd への書込が, パイプに送られる) */
    retval = close(fd);
    if (retval < 0) {
        outlog("close: fd=%d", fd);
        (void)close(pfd[PIPE_R]); /* パイプを閉じる */
        (void)close(pfd[PIPE_W]);
        return EX_NG;
    }

    /* fd は, 直前に閉じたので, 無効に見えるが, 書込側を, その番号に複製するのが目的 */
#if defined(__GNUC__) && __GNUC__ >= 10
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wanalyzer-fd-use-without-check"
#endif
    newfd = dup2(pfd[PIPE_W], fd);
#if defined(__GNUC__) && __GNUC__ >= 10
#  pragma GCC diagnostic pop
#endif
    if (newfd < 0) {
        outlog("dup2: pfd[PIPE_W]=%d, fd=%d", pfd[PIPE_W], fd);
        (void)close(pfd[PIPE_R]); /* パイプを閉じる */
        (void)close(pfd[PIPE_W]);
        return EX_NG;
    }
    dbglog("newfd=%d, pfd[PIPE_W]=%d, fd=%d", newfd, pfd[PIPE_W], fd);

    (void)close(pfd[PIPE_W]); /* 書込側は, fd に複製したので, 閉じる */

    return pfd[PIPE_R];
}

/**
 * @brief パイプ複製 2
 *
 * @param[in] pipefd パイプ
 * @param[in] oldfd コピー元
 * @param[in] newfd コピー先
 * @retval EX_NG エラー
 * @return 複製されたファイルディスクリプタ
 */
int
pipe_fd2(int *pipefd, int *oldfd, const int newfd)
{
    int retval = 0; /* 戻り値 */
    int fd = 0;     /* dup戻り値 */

    /* 使わない側を閉じる */
    close_fd(pipefd, NULL);

    /* newfd を閉じて, oldfd を newfd に複製する (newfd が, oldfd の指すパイプになる) */
    retval = close(newfd);
    if (retval < 0)
        outlog("close=%d", retval);

    /* newfd は, 直前に閉じたので, 無効に見えるが, oldfd を, その番号に複製するのが目的 */
#if defined(__GNUC__) && __GNUC__ >= 10
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wanalyzer-fd-use-without-check"
#endif
    fd = dup2(*oldfd, newfd);
#if defined(__GNUC__) && __GNUC__ >= 10
#  pragma GCC diagnostic pop
#endif
    if (fd < 0) {
        outlog("dup2=%d", retval);
        close_fd(oldfd, NULL);
        return EX_NG;
    }

    close_fd(oldfd, NULL);

    return fd;
}

/**
 * @brief リダイレクト
 *
 * この関数は, fd だけを扱い, フラッシュはしない.
 * fd から FILE は分からないので, 書き出されていない出力 (fprintf などで,
 * FILE にためたもの) は, 呼び出し側が, この関数を呼ぶ前に, fflush すること.
 * fflush しない場合, あとで書き出されるときに, リダイレクト先に出力される
 *
 * @param[in] fd ファイルディスクリプタ
 * @param[in] path ファイルパス
 * @retval EX_NG エラー
 */
int
redirect(int fd, const char *path)
{
    int f = 0;      /* ファイルディスクリプタ */
    int newfd = 0;  /* dup2戻り値 (リダイレクト先の fd) */
    int retval = 0; /* 戻り値 */

    if ((fd < 0) || (path == NULL))
        return EX_NG;

    /* 書込権限の確認 */
    if (access(path, W_OK) < 0) {
        outlog("access: %s", path);
        return EX_NG;
    }

    f = open(path, O_WRONLY | O_APPEND);
    if (f < 0) {
        outlog("open=%d", f);
        return EX_NG;
    }

    retval = close(fd);
    if (retval < 0)
        outlog("close: fd=%d", fd);

    /* fd は, 直前に閉じたので, 無効に見える. 複製された fd は, リダイレクト先として,
     * 開いたままにする (閉じない) */
#if defined(__GNUC__) && __GNUC__ >= 10
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wanalyzer-fd-use-without-check"
#  pragma GCC diagnostic ignored "-Wanalyzer-fd-leak"
#endif
    newfd = dup2(f, fd);
    if (newfd < 0) {
        outlog("dup2");
        (void)close(f);
        return EX_NG;
    }

    retval = close(f);
    if (retval < 0)
        outlog("close: f=%d", f);

    return EX_OK;
}
#if defined(__GNUC__) && __GNUC__ >= 10
#  pragma GCC diagnostic pop
#endif

/**
 * @brief クローズ
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
    int retval = 0;   /* 戻り値 */

    dbglog("start");

    /* 最初のファイルディスクリプタ */
    if ((fd != NULL) && (*fd >= 0)) {
        dbglog("%p fd=%d", (const void *)fd, *fd);
        retval = close(*fd);
        if (retval < 0) {
            outlog("close: fd=%d", *fd);
            err = true;
        }
    }
    *fd = -1;

    va_start(ap, fd);

    /* 続くファイルディスクリプタ (最後は NULL). 閉じたら -1 を代入する */
    ptr = va_arg(ap, int *);
    while (ptr != NULL) {
        dbglog("%p ptr=%d", (const void *)ptr, *ptr);
        if (*ptr >= 0) {
            retval = close(*ptr);
            if (retval < 0) {
                outlog("close: ptr=%d", *ptr);
                err = true;
            }
        }
        *ptr = -1;
        ptr = va_arg(ap, int *);
    }
    va_end(ap);

    if (err)
        return EX_NG;

    return EX_OK;
}
