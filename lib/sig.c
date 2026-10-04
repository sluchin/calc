/**
 * @file  lib/sig.c
 * @brief シグナル設定
 *
 * @author higashi
 * @date 2026-10-04 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2026 Tetsuya Higashi. All Rights Reserved.
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

#include <signal.h> /* sigaction sigfillset sigset_t */
#include <string.h> /* memset */

#include "def.h"
#include "log.h"
#include "sig.h"

/**
 * シグナルのハンドラ設定
 *
 * 現在の設定を取得して, ハンドラとフラグだけを変更する.
 * ハンドラの実行中は, 全てのシグナルをブロックする.
 *
 * @param[in] signo シグナル番号 (SIGINT など)
 * @param[in] handler ハンドラ (SIG_IGN で, 無視する)
 * @param[in] flags 追加するフラグ (SA_NOCLDWAIT, SA_NODEFER など. 不要なら 0)
 * @retval EX_OK 正常
 * @retval EX_NG エラー (設定は変わらない)
 */
int
set_sigaction(const int signo, void (*handler)(int), const int flags)
{
    struct sigaction sa; /* sigaction構造体 */
    sigset_t sigmask;    /* シグナルマスク */
    int retval = 0;      /* 戻り値 */

    dbglog("start: signo=%d, flags=0x%x", signo, (unsigned int)flags);

    (void)memset(&sa, 0, sizeof(struct sigaction));

    /* ハンドラの実行中は, 全てのシグナルを受け付けない */
    retval = sigfillset(&sigmask);
    if (retval < 0) {
        outlog("sigfillset: signo=%d", signo);
        return EX_NG;
    }

    /* 現在の設定を取得して, ハンドラとフラグを変更する */
    retval = sigaction(signo, NULL, &sa);
    if (retval < 0) {
        outlog("sigaction: get: signo=%d", signo);
        return EX_NG;
    }
    sa.sa_handler = handler;
    sa.sa_mask = sigmask;
    sa.sa_flags |= flags;

    retval = sigaction(signo, &sa, NULL);
    if (retval < 0) {
        outlog("sigaction: set: signo=%d", signo);
        return EX_NG;
    }

    return EX_OK;
}
