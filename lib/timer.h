/**
 * @file timer.h
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

#ifndef _TIMER_H_
#define _TIMER_H_

#include <stdio.h>    /* fprintf stderr */
#include <sys/time.h> /* timeval */

inline
unsigned long long get_time(void)
{
    struct timeval tv;
    struct timezone tz;

    gettimeofday(&tv, &tz);
    return (((unsigned long long)tv.tv_sec) * 1000000 + tv.tv_usec);
}

inline
void start_timer(unsigned int *start_time)
{
    *start_time = (unsigned int)get_time();
    return;
}

inline
unsigned int stop_timer(unsigned int *start_time)
{
    unsigned int stop_time = (unsigned int)get_time();
    return ((stop_time >= *start_time) ? (stop_time - *start_time) : stop_time);
}

#define print_timer(te) {                                               \
        (void)fprintf(stderr, "time of %s:%f[msec]\n", #te, te*1.0e-3); \
    }

#endif /* _TIMER_H_ */

