/**
 * @file  client/client.c
 * @brief ソケット送受信
 *
 * @author higashi
 * @date 2010-06-24 higashi 新規作成
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
#include <stdio.h>        /* FILE */
#include <stdlib.h>       /* strtol */
#include <string.h>       /* memcpy memset */
#include <ctype.h>        /* isdigit */
#include <sys/socket.h>   /* socket */
#include <arpa/inet.h>    /* inet_addr */
#include <errno.h>        /* errno */
#include <unistd.h>       /* close */
#ifdef _USE_SELECT
#  include <sys/select.h> /* pselect */
#else
#  include <sys/poll.h>   /* ppoll */
#endif
#ifdef HAVE_READLINE
#  include <readline/readline.h>
#  include <readline/history.h>
#else
#  include "readline.h"
#endif /* HAVE_READLINE */
#include <limits.h>

#include "def.h"
#include "log.h"
#include "data.h"
#include "net.h"
#include "util.h"
#include "option.h"
#include "timer.h"
#include "client.h"

#define SOCK_ERROR    -1 /**< ソケットエラー */

#ifdef HAVE_READLINE
static HIST_ENTRY *history = NULL;   /**< 履歴 */
static const uint MAX_HISTORY = 100; /**< 最大履歴数 */
#endif /* HAVE_READLINE */

#ifndef _USE_SELECT
enum {
    STDIN_POLL, /**< 標準入力 */
    SOCK_POLL,  /**< ソケット */
    MAX_POLL    /**< ポーリング数 */
};
#endif /* _USE_SELECT */

/* 内部変数 */
static uint start_time = 0; /**< タイマ開始 */

/* 内部関数 */
/** 標準入力読込 */
static bool read_stdin(int sock);
/** ソケット読込 */
static bool read_sock(int sock);
#ifdef HAVE_READLINE
/* イベントフック */
static int check_state(void);
/* history クリア */
static void freehistory(HIST_ENTRY **hist);
#endif /* HAVE_READLINE */

/**
 * ソケット送受信
 *
 * @param[in] sock ソケット
 * @return なし
 */
void
client_loop(int sock)
{
    int ready = 0;                   /* select戻り値 */
    bool stdst = true;               /* 標準入力ステータス */
    bool sockst = true;              /* ソケットステータス */
    struct timespec timeout;         /* タイムアウト値 */
#ifdef _USE_SELECT
    fd_set fds, rfds;                /* selectマスク */
#else
    struct pollfd targets[MAX_POLL]; /* poll */
#endif /* _USE_SELECT */

    dbglog("start: sock=%d", sock);

#ifdef _USE_SELECT
    /* マスクの設定 */
    FD_ZERO(&fds);              /* 初期化 */
    FD_SET(sock, &fds);         /* ソケットをマスク */
    FD_SET(STDIN_FILENO, &fds); /* 標準入力をマスク */
#endif /* _USE_SELECT */

#ifdef HAVE_READLINE
    rl_event_hook = &check_state;
#endif /* HAVE_READLINE */

    /* タイムアウト値初期化 */
    (void)memset(&timeout, 0, sizeof(struct timespec));
    timeout.tv_sec = 0;  /* ポーリング */
    timeout.tv_nsec = 0;

    do {
#ifdef _USE_SELECT
        (void)memcpy(&rfds, &fds, sizeof(fd_set)); /* マスクコピー */
        ready = pselect(sock + 1, &rfds,
                        NULL, NULL, &timeout, &g_sigaction.sa_mask);
#else
        targets[STDIN_POLL].fd = STDIN_FILENO;
        targets[STDIN_POLL].events = POLLIN;
        targets[SOCK_POLL].fd = sock;
        targets[SOCK_POLL].events = POLLIN;
        ready = ppoll(targets, MAX_POLL, &timeout, &g_sigation.sa_mask);
#endif /* _USE_SELECT */
        if (ready < 0) {
            if (errno == EINTR) /* 割り込み */
                break;
            /* selectエラー */
            outlog("select=%d", ready);
            break;
        } else if (ready) {
#ifdef _USE_SELECT
            if (FD_ISSET(STDIN_FILENO, &fds))
                /* 標準入力レディ */
                stdst = read_stdin(sock);
            if (FD_ISSET(sock, &fds))
                /* ソケットレディ */
                sockst = read_sock(sock);
#else
            if (targets[STDIN_POLL].revents & POLLIN)
                /* 標準入力レディ */
                stdst = read_stdin(sock);
            if (targets[SOCK_POLL].revents & POLLIN)
                /* ソケットレディ */
                sockst = read_sock(sock);
#endif /* _USE_SELECT */
        } else { /* ポーリング */
            continue;
        }
    } while (stdst && sockst && !sig_handled);

#ifdef HAVE_READLINE
    freehistory(&history);
#endif /* HAVE_READLINE */
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
        outlog("sock=%d", sock);
        return SOCK_ERROR;
    }

    /* コネクト */
    retval = connect(sock, (struct sockaddr *)&server,
                     sizeof(struct sockaddr_in));
    if (retval < 0) {
        outlog("connect=%d, sock=%d", retval, sock);
        /* ソケットクローズ */
        close_sock(&sock);
        return SOCK_ERROR;
    }
    return sock;
}

/**
 * 標準入力読込
 *
 * @param[in] sock ソケット
 * @retval true ループを継続する
 * @retval false ループを抜ける
 */
static bool
read_stdin(int sock)
{
    int retval = 0;                    /* 戻り値 */
    size_t length = 0;                 /* 長さ */
    struct client_data *sdata = NULL;  /* 送信データ構造体 */
    uchar *expr = NULL;                /* バッファ */
#ifdef HAVE_READLINE
    int hist_no = 0;                   /* 履歴数 */
    char *prompt = NULL;               /* プロンプト */
#endif /* HAVE_READLINE */

    dbglog("start");

    retval = fflush(NULL);
    if (retval == EOF)
        outlog("fflush=%d", retval);

#ifdef HAVE_READLINE
    expr = (uchar *)readline(prompt);
#else
    expr = _readline(stdin);
#endif /* HAVE_READLINE */

#ifdef _TEST
    if (expr)
        free(expr);
    expr = NULL;
    /* test
     * 1400000000 OK 1.4G
     * 1480000000 NG
     */
    size_t test_len = 1400000000;
    expr = (uchar *)malloc(test_len * sizeof(uchar));
    if (!expr) {
        outlog("malloc=%p", expr);
        return false;
    }
    (void)memset(expr, 0x31, test_len);
    *(expr + (test_len - 1)) = '\0';
#endif /* UNITTEST */

    if (!expr) /* メモリ確保できない */
        return true;

    if (*expr == '\0') { /* 文字列長ゼロ */
        outlog("expr=%p, %c", expr, *expr);
        memfree((void **)&expr, NULL);
        return true;
    }

    length = strlen((char *)expr) + 1;
    dbgdump(expr, length, "expr=%u", length);

    if (g_tflag)
        start_timer(&start_time);

    /* データ設定 */
    sdata = set_client_data(sdata, expr, length);
    if (!sdata) { /* メモリ確保できない */
        memfree((void **)&expr, (void **)&sdata, NULL);
        return true;
    }

    /* データ送信 */
    length += sizeof(struct header);

    if (g_gflag)
        outdump(sdata, length, "sdata=%p, length=%u", sdata, length);
    stddump(sdata, length, "sdata=%p, length=%u", sdata, length);

    retval = send_data(sock, sdata, length);
    if (retval < 0) { /* エラー */
        memfree((void **)&expr, (void **)&sdata, NULL);
        return true;
    }

    if (!strcmp((char *)expr, "quit") ||
        !strcmp((char *)expr, "exit")) {
        memfree((void **)&expr, (void **)&sdata, NULL);
        return false;
    }
#ifdef HAVE_READLINE
    if (MAX_HISTORY <= ++hist_no) {
        freehistory(&history);
    }
    add_history((char *)expr);
#endif /* HAVE_READLINE */
    memfree((void **)&expr, (void **)&sdata, NULL);

    return true;
}

/**
 * ソケット読込
 *
 * @param[in] sock ソケット
 * @retval true ループを継続する
 * @retval false ループを抜ける
 */
static bool
read_sock(int sock)
{
    int retval = 0;       /* 戻り値 */
    size_t length = 0;    /* 送信または受信する長さ */
    ushort cs = 0;        /* チェックサム値 */
    struct header hd;     /* ヘッダ */
    uchar *answer = NULL; /* 受信データ */

    dbglog("start");

    /* ヘッダ受信 */
    length = sizeof(struct header);
    (void)memset(&hd, 0, length);
    retval = recv_data(sock, &hd, length);
    if (retval < 0) /* エラー */
        return false;
    dbglog("recv_data=%d, hd=%p, length=%u",
           retval, &hd, length);
    if (g_gflag)
        outdump(&hd, length, "hd=%p, length=%u", &hd, length);
    stddump(&hd, length, "hd=%p, length=%u", &hd, length);

    length = hd.length; /* データ長を保持 */

    /* データ受信 */
    answer = recv_data_new(sock, &length);
    if (!answer) { /* エラー */
        memfree((void **)&answer, NULL);
        if (length < 0) /* 受信エラー */
            return false;
        else /* メモリ不足 */
            return true;
    }
    dbglog("answer=%p, length=%u", answer, length);

    if (g_gflag)
        outdump(answer, length, "answer=%p, length=%u", answer, length);
    stddump(answer, length, "answer=%p, length=%u", answer, length);

    cs = in_cksum((ushort *)answer, length);
    if (cs != hd.checksum) { /* チェックサムエラー */
        outlog("checksum error: 0x%x!=0x%x",
               cs, hd.checksum);
        memfree((void **)&answer, NULL);
        return true;
    }

    if (g_tflag)
        print_timer(stop_timer(&start_time));

    retval = fprintf(stdout, "%s\n", answer);
    if (retval < 0) {
        outlog("fprintf=%d", retval);
        memfree((void **)&answer, NULL);
        return true;
    }

    retval = fflush(stdout);
    if (retval == EOF) {
        outlog("fflush=%d", retval);
        memfree((void **)&answer, NULL);
        return true;
    }
    memfree((void **)&answer, NULL);

    return true;
}

/**
 * イベントフック
 *
 * readline 内から定期的に呼ばれる関数
 * @return 常にEX_OK
 */
#ifdef HAVE_READLINE
static int check_state(void) {
    if (sig_handled) {
        /* 入力中のテキストを破棄 */
        rl_delete_text(0, rl_end);

        /* readlineをreturnさせる */
        rl_done = 1;

        //rl_event_hook = 0;
        //rl_deprep_terminal();
        //close(STDIN_FILENO);
    }
    return EX_OK;
}

/**
 * history クリア
 *
 * @return なし
 */
static void
freehistory(HIST_ENTRY **hist)
{
    if (*hist) {
        *hist = remove_history(0);
        free(*hist);
    }
    *hist = NULL;
}

#endif /* HAVE_READLINE */

