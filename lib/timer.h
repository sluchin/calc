/**
 * @file  lib/timer.h
 * @brief 処理時間測定
 *
 * @author higashi
 * @date 2010-06-22 higashi 新規作成
 * @version \$Id$
 *
 * usage:
 *     unsigned int t, time;
 *     start_timer(&t);
 *
 *     .... process .....
 *
 *     time = stop_timer(&t);
 *     print_timer(time);
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

#ifndef TIMER_H
#define TIMER_H

#include <stdio.h>    /* fprintf stderr */
#include <sys/time.h> /* timeval */

#include "log.h"

/** 時間出力 */
#define print_timer(te)                                                    \
    {                                                                      \
        (void)fprintf(stderr, "time of %s: %f[msec]\n", #te, te * 1.0e-3); \
    }

/** タイマースタート */
static inline void start_timer(unsigned int *start_time);
/** タイマーストップ */
static inline unsigned int stop_timer(unsigned int *start_time);
/** 時刻取得 */
static inline unsigned long long get_time(void);

/**
 * タイマースタート
 *
 * @param[out] start_time 開始時刻を保持する変数
 */
static inline void
start_timer(unsigned int *start_time)
{
    *start_time = (unsigned int)get_time();
    return;
}

/**
 * タイマーストップ
 *
 * @param[in] start_time タイマー開始の時刻
 * @return 時間
 */
static inline unsigned int
stop_timer(unsigned int *start_time)
{
    unsigned int stop_time = (unsigned int)get_time(); /* 終了時刻 */
    /* 32 ビットに切り詰めた時刻 (約 71 分で一周する) は, 符号なしの引き算で,
     * 一周しても, 正しい経過時間になる */
    return stop_time - *start_time;
}

/**
 * 時刻取得
 *
 * @return 時刻
 */
static inline unsigned long long
get_time(void)
{
    struct timeval tv; /* timeval構造体 */
    int retval = 0;    /* 戻り値 */

    timerclear(&tv);

    retval = gettimeofday(&tv, NULL);
    if (retval < 0)
        outlog("gettimeofday");

    return ((unsigned long long)tv.tv_sec) * 1000000ULL + (unsigned long long)tv.tv_usec;
}

#endif /* TIMER_H */
