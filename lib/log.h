/**
 * @file lib/log.h
 * @brief ログ出力
 *
 * @author higashi
 * @date 2010-06-22 higashi 新規作成
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

#ifndef _OUTPUTLOG_H_
#define _OUTPUTLOG_H_

#include <stddef.h> /* size_t */
#include <syslog.h> /* syslog */

#define SYS_FACILITY  LOG_SYSLOG
#define LOGARGS       LOG_INFO, LOG_PID, get_progname(),      \
        __FILE__, __LINE__, __FUNCTION__

/* エラー時ログメッセージ出力 */
#define outlog(fmt, ...)        system_log(LOGARGS, fmt, ## __VA_ARGS__)
#define outstd(fmt, ...)        stderr_log(LOGARGS, fmt, ## __VA_ARGS__)
#define outdump(a, b, fmt, ...) dump_sys(LOGARGS, a, b, fmt, ## __VA_ARGS__)
/* デバッグ用ログメッセージ */
#ifdef _DEBUG
#  define dbglog(fmt, ...)        system_dbg_log(LOGARGS, fmt, ## __VA_ARGS__)
#  define stdlog(fmt, ...)        stderr_log(LOGARGS, fmt, ## __VA_ARGS__)
#  define dbgdump(a, b, fmt, ...) dump_sys(LOGARGS, a, b, fmt, ## __VA_ARGS__)
#  define stddump(a, b, fmt, ...) dump_log(a, b, fmt, ## __VA_ARGS__)
#  define dbgtrace()              systrace(LOGARGS)
#  define dbgterm(fd)             sys_print_termattr(LOGARGS, fd)
#else
#  define dbglog(fmt, ...)        do { } while (0)
#  define stdlog(fmt, ...)        do { } while (0)
#  define dbgdump(a, b, fmt, ...) do { } while (0)
#  define stddump(a, b, fmt, ...) do { } while (0)
#  define dbgtrace()              do { } while (0)
#  define dbgterm(fd)             do { } while (0)
#endif /* _DEBUG */

/** プログラム名設定 */
void set_progname(char *name);

/** プログラム名取得 */
char *get_progname(void);

/** シスログ出力 */
void system_log(const int level, const int option, const char *pname,
                const char *fname, const int line, const char *func,
                const char *format, ...);

/** シスログ出力(デバッグ用) */
void system_dbg_log(const int level, const int option, const char *pname,
                    const char *fname, const int line, const char *func,
                    const char *format, ...);

/** 標準エラー出力にログ出力 */
void stderr_log(const char *pname, const char *fname,
                const int line, const char *func,
                const char *format, ...);

/** 標準エラー出力にHEXダンプ */
int dump_log(const void *buf, const size_t len, const char *format, ...);

/** シスログにHEXダンプ */
int dump_sys(const int level, const int option, const char *pname,
             const char *fname, const int line, const char *func,
             const void *buf, const size_t len, const char *format, ...);

/** ファイルにバイナリ出力 */
int dump_file(const char *pname, const char *fname, const char *buf,
              const size_t len);

/** バックトレースシスログ出力 */
void systrace(const int level, const int option, const char *pname,
              const char *fname, const int line, const char *func);

/** バックトレース出力 */
void print_trace(void);

void sys_print_termattr(const int level, const int option,
                        const char *pname, const char *fname,
                        const int line, const char *func, int fd);

#ifdef UNITTEST
struct _testlog {
   char *(*strmon)(int mon);
   void (*destroy_progname)(void);
   char *progname;
};
typedef struct _testlog testlog;

void test_init_log(testlog *log);
#endif /* UNITTEST */

#endif /* _OUTPUTLOG_H_ */

