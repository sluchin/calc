/**
 * @file lib/log.h
 * @brief ログ出力
 *
 * @author higashi
 * @date 2010-06-22 higashi 新規作成
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

#ifndef _OUTPUTLOG_H_
#define _OUTPUTLOG_H_

#include <stddef.h> /* size_t */

extern char *progname; /* プログラム名 */

#define SYS_PRIO       LOG_INFO
#define LOG_FORMAT     progname, __FILE__, __LINE__, __FUNCTION__

#define outlog(fmt...)         system_log(LOG_FORMAT, fmt)
#define outstd(fmt...)         stderr_log(LOG_FORMAT, fmt)
#define outdump(a, b, fmt...)  dump_sys(LOG_FORMAT, a, b, fmt)

/* デバッグ用ログメッセージ */
#ifdef _DEBUG
#  define dbglog(fmt...)             system_dbg_log(LOG_FORMAT, fmt)
#  define stdlog(fmt...)             stderr_log(LOG_FORMAT, fmt)
#  define dbgdump(a, b, fmt...)      dump_sys(LOG_FORMAT, a, b, fmt)
#  define stddump(a, b, fmt...)      dump_log(a, b, fmt)
#else
#  define dbglog(fmt...)             do { } while (0)
#  define stdlog(fmt...)             do { } while (0)
#  define dbgdump(a, b, fmt...)      do { } while (0)
#  define stddump(a, b, fmt...)      do { } while (0)
#endif /* _DEBUG */

/** シスログ出力 */
void system_log(const char *pname, const char *fname,
                const int line, const char *func,
                const char *format, ...);

/** シスログ出力(デバッグ用) */
void system_dbg_log(const char *pname, const char *fname,
                    const int line, const char *func,
                    const char *format, ...);

/** 標準エラー出力にログ出力 */
void stderr_log(const char *pname, const char *fname,
                const int line, const char *func,
                const char *format, ...);

/** 標準エラー出力にHEXダンプ */
void dump_log(const void *buf, const size_t len, const char *format, ...);

/** シスログにHEXダンプ */
void dump_sys(const char *pname, const char *fname,
              const int line, const char *func,
              const void *buf, const size_t len,
              const char *format, ...);

/** ファイルにバイナリ出力 */
void dump_file(const char *pname, const char *fname, const char *buf,
               const size_t len);

/** バックトレース出力 */
void print_trace(void);

#endif /* _OUTPUTLOG_H_ */

