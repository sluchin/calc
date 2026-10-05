/**
 * @file  server/main.c
 * @brief main関数
 *
 * @author higashi
 * @date 2010-06-25 higashi 新規作成
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

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE /**< execvpe を使うために必要 */
#endif
#include <stdlib.h> /* exit EXIT_SUCCESS realpath */
#include <limits.h> /* PATH_MAX */
#include <string.h> /* memset */
#include <unistd.h> /* alarm execve */
#include <signal.h> /* SIGINT SIGHUP SIGCHLD SA_NOCLDWAIT */

#include "log.h"
#include "net.h"
#include "sig.h"
#include "memfree.h"
#include "option.h"
#include "calc.h"
#include "server.h"

/* 内部変数 */
static volatile sig_atomic_t hupflag = 0; /**< シグナル種別 */
static int sockfd = -1;                   /**< ソケット */

/* 内部関数 */
static void set_sig_handler(void);
static void sig_handler(int signo);

/**
 * main関数
 *
 * @param[in] argc 引数の数
 * @param[in] argv コマンド引数・オプション引数
 * @param[in] envp 環境変数
 * @retval EXIT_SUCCESS 正常
 */
int
main(int argc, char *argv[], char *envp[])
{
#ifndef _DEBUG
    int retval = 0; /* 戻り値 */
#endif
    char exepath[PATH_MAX];        /* 実行ファイルの絶対パス */
    const char *restart = argv[0]; /* 再起動する実行ファイル */
    const char *resolved = NULL;   /* realpath戻り値 */

    dbglog("start");

    /* シグナルハンドラ */
    set_sig_handler();

    /* プログラム名を保持 */
    set_progname(argv[0]);

    /* オプション引数 */
    parse_args(argc, argv);

    /* daemon() は, カレントディレクトリを / に変えるので, 再起動 (SIGHUP) のために,
     * 実行ファイルのパスを, 前もって絶対パスにする. (/ を含まないときは, PATH から探す) */
    if (strchr(argv[0], '/') != NULL) {
        resolved = realpath(argv[0], exepath);
        if (resolved != NULL)
            restart = exepath;
    }

    /* ソケット接続 */
    sockfd = server_sock();
    if (sockfd < 0)
        exit(EXIT_FAILURE);

    /* デーモン化する */
#ifndef _DEBUG
    retval = daemon(0, 0);
    if (retval < 0) {
        outlog("daemon[%d]", retval);
        exit(EXIT_FAILURE);
    }
#endif /* _DEBUG */

    /* ソケット送受信 */
    server_loop(sockfd);

    /* ソケットクローズ */
    close_sock(&sockfd);

    if (hupflag != 0) { /* 再起動 */
        dbglog("SIGHUP");
        (void)alarm(0u);
        (void)execvpe(restart, argv, envp);
        outlog("execvpe: %s", restart); /* 成功すると, ここには戻らない */
        exit(EXIT_FAILURE);
    }

    exit(EXIT_SUCCESS);
    return EXIT_SUCCESS;
}

/**
 * シグナルハンドラ設定
 */
static void
set_sig_handler(void)
{
    /* シグナルの設定 */
    struct sig_setting {
        int signo;            /* シグナル番号 */
        void (*handler)(int); /* ハンドラ (SIG_IGN で, 無視する) */
        int flags;            /* 追加するフラグ */
    };
    static const struct sig_setting settings[] = {
        /* シグナル補足 */
        {SIGINT,  sig_handler, 0           },
        {SIGTERM, sig_handler, 0           },
        {SIGQUIT, sig_handler, 0           },
        {SIGHUP,  sig_handler, 0           },
        /* 子プロセスをゾンビ化しない */
        {SIGCHLD, SIG_IGN,     SA_NOCLDWAIT}, /* Linux 2.6 以降 */
        /* シグナル無視 */
        {SIGALRM, SIG_IGN,     SA_NODEFER  },
        {SIGPIPE, SIG_IGN,     SA_NODEFER  },
        {SIGUSR1, SIG_IGN,     SA_NODEFER  },
        {SIGUSR2, SIG_IGN,     SA_NODEFER  },
        {SIGTTIN, SIG_IGN,     SA_NODEFER  },
        {SIGTTOU, SIG_IGN,     SA_NODEFER  }
    };
    unsigned int i = 0u; /* 繰り返し */
    int retval = 0;      /* 戻り値 */

    for (i = 0u; i < NELEMS(settings); i++) {
        retval = set_sigaction(settings[i].signo, settings[i].handler, settings[i].flags);
        if (retval < 0)
            outlog("set_sigaction: signo=%d", settings[i].signo);
    }
}

/**
 * シグナルハンドラ
 *
 * @param[in] signo シグナル
 */
static void
sig_handler(int signo)
{
    g_sig_handled = 1;

    if (signo == SIGHUP)
        hupflag = 1;
}
