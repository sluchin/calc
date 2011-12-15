/**
 * @file lib/tests/test_server.c
 * @brief ログ監視
 *
 * ログを監視し, 入力があれば標準出力に出力する.
 *
 * @author higashi
 * @date 2011-11-19 higashi 新規作成
 * @version \$Id$
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

#include <stdio.h>      /* fprintf printf */
#include <string.h>     /* memset */
#include <stdlib.h>     /* exit */
#include <errno.h>      /* errno */
#include <time.h>       /* struct tm */
#include <sys/time.h>   /* struct timeval */
#include <sys/stat.h>   /* open */
#include <fcntl.h>      /* open */
#include <signal.h>     /* signal */
#include <libgen.h>     /* basename */
#include <unistd.h>     /* lseek pipe dup2 */
#include <sys/socket.h> /* socket setsockopt */
#include <sys/types.h>  /* setsockopt */
#include <sys/un.h>     /* struct sockaddr_un */
#include <sys/select.h> /* select */
#include <pthread.h>    /* pthread */

#include "def.h"
#include "log.h"
#include "net.h"
#include "test_server.h"

/** パイプ */
enum {
    PIPE_R = 0, /**< リード */
    PIPE_W,     /**< ライト */
    MAX_PIPE    /**< パイプ数 */
};

/* 内部変数 */
static const int EX_ERROR = -1; /**< エラー戻り値 */

/* 内部関数 */
/** サーバプロセス */
static void server_proc(const int fd, char *buf, const size_t size);
/** シグナル設定 */
static void set_sig_handler(void);

/**
 * リダイレクト
 *
 * @param[in] fd ファイル記述子
 * @retval EX_ERROR エラー
 * @return ファイルディスクリプタ
 */
int
pipe_fd(const int fd)
{
    int pfd[MAX_PIPE]; /* pipe */
    int retval = 0;    /* 戻り値 */

    retval = pipe(pfd);
    if (retval < 0) {
        outlog("pipe=%d", retval);
        return EX_ERROR;
    }

    retval = dup2(pfd[PIPE_W], fd);
    if (retval < 0) {
        outlog("dup2=%d", retval);
        return EX_ERROR;
    }

    retval = close(pfd[PIPE_W]);
    if (retval < 0) {
        outlog("close=%d, fd=%d", retval, fd);
        return EX_ERROR;
    }

    return pfd[PIPE_R];
}

/**
 * ファイル記述子クローズ
 *
 * @param[in] fd ファイル記述子
 * @retval EX_NG エラー
 */
int
close_fd(const int fd)
{
    int retval = 0; /* 戻り値 */

    dbglog("start: fd=%d", fd);

    retval = close(fd);
    if (retval < 0) {
        outlog("close=%d, fd=%d", retval, fd);
        return EX_NG;
    }
    return EX_OK;
}

/**
 * サーバループ
 *
 * 一度処理を実行した後、すぐブレイクする。
 *
 * @param[in] fd ファイル記述子
 * @param[in,out] buf バッファ
 * @param[in] size バッファサイズ
 * @retval EX_NG エラー
 */
int
server_loop(const int fd, char *buf, const size_t size)
{
    int ready = 0;           /* select戻り値 */
    fd_set fds, rfds;        /* selectマスク */
    struct timeval timeout;  /* タイムアウト値 */

    /* タイムアウト値初期化 */
    (void)memset(&timeout, 0, sizeof(struct timeval));

    /* マスクの設定 */
    FD_ZERO(&fds);    /* 初期化 */
    FD_SET(fd, &fds); /* ソケットをマスク */

    set_sig_handler();

    /* ノンブロッキングに設定 */
    if (set_block(fd, NONBLOCK) < 0)
        return EX_NG;

    for (;;) {
        (void)memcpy(&rfds, &fds, sizeof(fd_set)); /* マスクコピー */
        /* selectの場合, timeval構造体は
           入出力のためここで値を入れる */
        timeout.tv_sec = 5; /* 5秒タイムアウト */
        timeout.tv_usec = 0;
        ready = select(fd + 1, &rfds, NULL, NULL, &timeout);
        if (ready < 0) { /* エラー */
            if (errno == EINTR) /* 割り込み */
                continue;
            outlog("select=%d", ready);
            return EX_NG;
        } else if (ready) {
            if (FD_ISSET(fd, &rfds)) {
                server_proc(fd, buf, size);
                break;
            }
        } else { /* タイムアウト */
            return EX_NG;
        }
    }
    return EX_OK;
}

/**
 * サーバプロセス
 *
 * 割り込みの場合のみループする.
 *
 * @param[in] fd ファイル記述子
 * @param[in,out] buf バッファ
 * @param[in] size バッファサイズ
 * @return なし
 */
static void
server_proc(const int fd, char *buf, const size_t size)
{
    ssize_t len = 0; /* read戻り値 */
    int rlen = 0;    /* 読み込んだ長さ */
    int left = 0;    /* 残りの長さ */

    left = size - 1;
loop:
    len = read(fd, buf + rlen, left);
    if (len < 0) {
        if (errno == EINTR || errno == EWOULDBLOCK) {
            rlen += len;
            left -= len;
            goto loop;
        }
        outlog("read=%u", len);
    }

    if (buf[strlen(buf) - 1] == '\n')
        buf[strlen(buf) - 1] = '\0'; /* 改行削除 */

    dbglog("buf=%s", buf);
    return;
}

/**
 * シグナル設定
 *
 * @return なし
 */
static void
set_sig_handler(void)
{
    /* シグナル無視 */
    if (signal(SIGINT, SIG_IGN) < 0)
        outlog("SIGINT");
    if (signal(SIGTERM, SIG_IGN) < 0)
        outlog("SIGTERM");
    if (signal(SIGQUIT, SIG_IGN) < 0)
        outlog("SIGQUIT");
    if (signal(SIGHUP, SIG_IGN) < 0)
        outlog("SIGHUP");
    if (signal(SIGCHLD, SIG_IGN) < 0)
        outlog("SIGCHLD");
    if (signal(SIGALRM, SIG_IGN) < 0)
        outlog("SIGALRM");
}

