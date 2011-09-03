/**
 * @file  client.c
 * @brief ソケット送受信
 *
 * @sa client.h
 * @author higashi
 * @date 2010-06-24 higashi 新規作成
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

#include <stdio.h>        /* FILE */
#include <stdlib.h>       /* strtol */
#include <string.h>       /* memcpy memset */
#include <stdbool.h>      /* bool */
#include <ctype.h>        /* isdigit */
#include <sys/socket.h>   /* socket */
#include <arpa/inet.h>    /* inet_addr */
#include <errno.h>        /* errno */
#include <unistd.h>       /* close */
#include <signal.h>       /* sig_atomic_t */
#ifdef _USE_SELECT
#  include <sys/select.h> /* select */
#  include <sys/time.h>   /* timerclear */
#else
#  include <sys/poll.h>   /* poll */
#endif
#if defined(_USE_READLINE)
#include <readline/readline.h>
#include <readline/history.h>
#endif /* _USE_READLINE */

#include "def.h"
#include "log.h"
#include "data.h"
#include "util.h"
#include "client.h"

#define SOCK_ERROR    (int)(-1) /**< ソケットエラー */

/* 外部変数 */
extern bool gflag;                        /**< gオプションフラグ */
extern volatile sig_atomic_t sig_handled; /**< シグナル */
/* 内部変数 */
static struct client_data *sdata = NULL;  /* 送信データ構造体 */
static uchar *expr = NULL;                /* バッファ */
static uchar *answer = NULL;              /* 受信データ */
#if defined(_USE_READLINE)
static HIST_ENTRY *history = NULL;        /**< 履歴 */
#endif /* _USE_READLINE */

#if defined(_USE_READLINE)
#  define FREEHISTORY                           \
    if (history) {                              \
        history = remove_history(0);            \
        free(history); }                        \
    history = NULL;
#  define MAX_HISTORY    100 /**< 最大履歴数 */
#endif /* _USE_READLINE */

#ifndef _USE_SELECT
enum {
    STDIN_POLL, /**< 標準入力 */
    SOCK_POLL,  /**< ソケット */
    MAX_POLL    /**< ポーリング数 */
};
#endif /* _USE_SELECT */

/** ステータス */
typedef enum {
    ST_CONT = 0, /**< ループから抜け出さない */
    ST_BREAK     /**< ループから抜け出す */
} ST;

/* 内部関数 */
/** 標準入力読込 */
static ST read_stdin(int sock);
/** ソケット読込 */
static ST read_sock(int sock);
/* イベントフック */
#if defined(_USE_READLINE)
static int check_state(void);
#endif
/** メモリ解放 */
static void memfree(void);

/**
 * 標準入力読込
 *
 * @param[in] ソケット
 * @param[out] バッファ
 * @param[in] バッファサイズ
 * @retval ST_CONT ループを継続する
 * @retval ST_BREAK ループを抜ける
 */
static ST
read_stdin(int sock)
{
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* 文字列長 */
    size_t rlen = 0;   /* 送信するデータ長 */
#if defined(_USE_READLINE)
    int hist_no = 0;     /* 履歴数 */
    char *prompt = NULL; /* プロンプト */
#endif /* _USE_READLINE */

    dbglog("start");

#if defined(_USE_READLINE)
    expr = (uchar *)readline(prompt);
    if (!expr)
        return ST_BREAK;
#else
    expr = read_line(stdin);
#endif

    if (!expr || *expr == 0) {
        outlog("expr[%p]: %s");
        memfree();
        return ST_CONT;
    }
    
    length = strlen((char *)expr) + 1;
    dbgdump(expr, length, "expr[%u]", length);

    /* データ設定 */
    sdata = set_client_data(sdata, expr, length);
    if (!sdata) {
        memfree();
        return ST_CONT;
    }

    /* データ送信 */
    rlen = sizeof(struct header) + length;

    if (gflag)
        outdump(sdata, rlen, "sdata[%p] rlen[%u]", sdata, rlen);
    stddump(sdata, rlen, "sdata[%p] rlen[%u]", sdata, rlen);

    retval = send_data(sock, sdata, rlen);
    if (retval < 0) /* エラー */
        return ST_BREAK;

    if (!strcmp((char *)expr, "quit") ||
        !strcmp((char *)expr, "exit")) {
        return ST_BREAK;
    }
#if defined(_USE_READLINE)
    if (MAX_HISTORY <= ++hist_no) {
        FREEHISTORY;
    }
    add_history((char *)expr);
#endif /* _USE_READLINE */
    memfree();

    return ST_CONT;
}

/**
 * ソケット読込
 *
 * @param[in] ソケット
 * @retval ST_CONT ループを継続する
 * @retval ST_BREAK ループを抜ける
 */
static ST
read_sock(int sock)
{
    int retval = 0;    /* 戻り値 */
    size_t length = 0; /* 送信または受信する長さ */
    ushort cs = 0;     /* チェックサム値 */
    struct header hd;  /* ヘッダ */

    dbglog("start");

    /* ヘッダ受信 */
    dbglog("recv header");
    length = sizeof(struct header);
    (void)memset(&hd, 0, length);
    retval = recv_data(sock, &hd, length);
    if (retval < 0) /* エラー */
        return ST_CONT;
    dbglog("recv_data[%d]: hd[%p]: length[%u]",
           retval, &hd, length);
    if (gflag)
        outdump(&hd, length, "hd[%p] length[%u]", &hd, length);
    stddump(&hd, length, "hd[%p] length[%u]", &hd, length);

    length = hd.length; /* データ長を保持 */

    /* データ受信 */
    dbglog("recv data");

    /* メモリ確保 */
    answer = (uchar *)malloc(length * sizeof(uchar));
    if (!answer) {
        outlog("malloc[%p]", answer);
        return ST_CONT;
    }
    (void)memset(answer, 0, length);

    retval = recv_data(sock, answer, length);
    if (retval < 0) /* エラー */
        return ST_BREAK;

    dbglog("recv_data[%d]: answer[%p]: length[%u]",
           retval, answer, length);
    if (gflag)
        outdump(answer, length, "answer[%p] length[%u]", answer, length);
    stddump(answer, length, "answer[%p] length[%u]", answer, length);

    cs = in_cksum((ushort *)answer, length);
    if (cs != hd.checksum) { /* チェックサムエラー */
        outlog("checksum error: cs[0x%x!=0x%x]",
               cs, hd.checksum);
        memfree();
        return ST_CONT;
    }
    retval = fprintf(stdout, "%s\n", answer);
    if (retval < 0) {
        outlog("fprintf[%d]", retval);
        memfree();
        return ST_CONT;
    }
    retval = fflush(stdout);
    if (retval == EOF) {
        outlog("fflush[%d]", retval);
        memfree();
        return ST_CONT;
    }
    memfree();

    return ST_CONT;
}

/** 
 * イベントフック
 *
 * readline 内から定期的に呼ばれる関数
 * @return 常にST_OK
 */
#if defined(_USE_READLINE)
static int check_state(void) {
    if (sig_handled) {
        /* 入力中のテキストを破棄 */
        rl_delete_text(0, rl_end);

        /* readline を return させる */
        rl_done = 1;

        //rl_event_hook = 0;
        //rl_deprep_terminal();
        //close(STDIN_FILENO);
    }
    return ST_OK;
}
#endif /* _USE_READLINE */

/**
 * メモリ解放
 *
 * @return なし
 */
static void
memfree(void)
{
    if (sdata)
        free(sdata);
    sdata = NULL;
    if (expr)
        free(expr);
    expr = NULL;
    if (answer)
        free(answer);
    answer = NULL;
}

/**
 * ソケット接続
 *
 * @param[in] host ホスト名またはIPアドレス　
 * @param[in] port ポート番号
 * @return ソケット
 */
int
connect_sock(const char *host, const char *port)
{
    struct sockaddr_in server; /* ソケットアドレス情報構造体 */
    struct in_addr addr;       /* IPアドレス情報構造体 */
    int sock = -1;             /* ソケット */
    int retval = 0;            /* 戻り値 */

    dbglog("start");

    /* 初期化 */
    (void)memset(&server, 0, sizeof(struct sockaddr_in));
    server.sin_family = AF_INET;

    if (set_hostname(&server, &addr, host) < 0)
        return SOCK_ERROR;
    if (set_port(&server, port) < 0)
        return SOCK_ERROR;

    /* ソケット生成 */
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        outlog("sock[%d]", sock);
        return SOCK_ERROR;
    }

    /* コネクト */
    retval = connect(sock, (struct sockaddr *)&server,
                     sizeof(struct sockaddr_in));
    if (retval < 0) {
        outlog("connect[%d]: sock[%d]", retval, sock);
        /* ソケットクローズ */
        close_sock(&sock);
        return SOCK_ERROR;
    }
    return sock;
}

/**
 * ソケット送受信
 *
 * @param[in] sock ソケット
 * @return なし
 */
void
client_loop(int sock)
{
    int ready = 0;          /* select戻り値 */
    ST status = ST_CONT;    /* ステータス */
#ifdef _USE_SELECT
    fd_set fds, tfds;       /* selectマスク */
    struct timeval timeout; /* タイムアウト値 */
#else
    struct pollfd targets[MAX_POLL]; /* poll */
#endif /* _USE_SELECT */

    dbglog("start: sock[%d]", sock);

#ifdef _USE_SELECT
    /* マスクの設定 */
    FD_ZERO(&tfds);              /* 初期化 */
    FD_SET(sock, &tfds);         /* ソケットをマスク */
    FD_SET(STDIN_FILENO, &tfds); /* 標準入力をマスク */

    /* タイムアウト値の初期化 */
    timerclear(&timeout);
#endif /* _USE_SELECT */

#if defined(_USE_READLINE)
    rl_event_hook = &check_state;
#endif
    do {
        errno = 0;    /* errno初期化 */
#ifdef _USE_SELECT
        fds = tfds; /* マスクの代入 */
        timeout.tv_sec = 1;   /* 1秒に設定 */
        timeout.tv_usec = 0;
        ready = select(sock + 1, &fds, NULL, NULL, &timeout); 
#else
        targets[STDIN_POLL].fd = STDIN_FILENO;
        targets[STDIN_POLL].events = POLLIN;
        targets[SOCK_POLL].fd = sock;
        targets[SOCK_POLL].events = POLLIN;
        ready = poll(targets, MAX_POLL, 1 * 1000);
#endif /* _USE_SELECT */
        switch (ready) {
        case -1:
            if (errno == EINTR) { /* 割り込み */
                outlog("interrupt[%d]", errno);
                break;
            }
            /* selectエラー */
            outlog("select[%d]", ready);
            break;
        case 0: /* タイムアウト */
            break;
        default:
#ifdef _USE_SELECT
            if (FD_ISSET(STDIN_FILENO, &fds)) /* 標準入力レディ */
                status = read_stdin(sock);
            if (FD_ISSET(sock, &fds)) /* ソケットレディ */
                status = read_sock(sock);
#else
            if (targets[STDIN_POLL].revents & POLLIN) /* 標準入力レディ */
                status = read_stdin(sock);
            if (targets[SOCK_POLL].revents & POLLIN) /* ソケットレディ */
                status = read_sock(sock);
#endif /* _USE_SELECT */
            break;
        } /* switch */
    } while (status == ST_CONT && !sig_handled); 

    /* 終了処理 */
    memfree();
#if defined(_USE_READLINE)
    FREEHISTORY;
#endif /* _USE_READLINE */
}

