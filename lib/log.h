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

#ifndef OUTPUTLOG_H
#define OUTPUTLOG_H

#include <stddef.h> /* size_t */
#include <syslog.h> /* syslog LOG_INFO LOG_PID */

#include "term.h"

/** syslog のファシリティ */
#define SYS_FACILITY LOG_SYSLOG
/** ログ関数に渡す共通の引数 (プログラム名, ファイル名, 行番号, 関数名) */
#define LOGARGS      get_progname(), __FILE__, __LINE__, __func__
/** syslog 用のログ関数に渡す共通の引数 (レベル, オプション, LOGARGS) */
#define SYSARGS      LOG_INFO, LOG_PID, LOGARGS

/* エラー時ログメッセージ出力 */
/** エラーメッセージを syslog に出力 */
#define outlog(fmt, ...)        system_log(SYSARGS, fmt, ##__VA_ARGS__)
/** エラーメッセージを標準エラー出力に出力 */
#define outstd(fmt, ...)        stderr_log(LOGARGS, fmt, ##__VA_ARGS__)
/** バッファのダンプを syslog に出力 */
#define outdump(a, b, fmt, ...) dump_sys(SYSARGS, a, b, fmt, ##__VA_ARGS__)
/* デバッグ用ログメッセージ */
#ifdef _DEBUG
#  define dbglog(fmt, ...)        system_dbg_log(SYSARGS, fmt, ##__VA_ARGS__)
#  define stdlog(fmt, ...)        stderr_log(LOGARGS, fmt, ##__VA_ARGS__)
#  define dbgdump(a, b, fmt, ...) dump_sys(SYSARGS, a, b, fmt, ##__VA_ARGS__)
#  define stddump(a, b, fmt, ...) dump_log(a, b, fmt, ##__VA_ARGS__)
#  ifdef HAVE_EXECINFO
#    define dbgtrace() systrace(SYSARGS)
#  else
#    define dbgtrace() \
        do {           \
        } while (0)
#  endif
#  define dbgterm(fd) sys_print_termattr(SYSARGS, fd)
#else
/** デバッグ用メッセージを syslog に出力 (_DEBUG のときだけ有効) */
#  define dbglog(fmt, ...) \
      do {                 \
      } while (0)
/** デバッグ用メッセージを標準エラー出力に出力 (_DEBUG のときだけ有効) */
#  define stdlog(fmt, ...) \
      do {                 \
      } while (0)
/** バッファのダンプを syslog に出力 (_DEBUG のときだけ有効) */
#  define dbgdump(a, b, fmt, ...) \
      do {                        \
      } while (0)
/** バッファのダンプを標準エラー出力に出力 (_DEBUG のときだけ有効) */
#  define stddump(a, b, fmt, ...) \
      do {                        \
      } while (0)
/** バックトレースを syslog に出力 (_DEBUG のときだけ有効) */
#  define dbgtrace() \
      do {           \
      } while (0)
/** 端末属性を syslog に出力 (_DEBUG のときだけ有効) */
#  define dbgterm(fd) \
      do {            \
      } while (0)
#endif /* _DEBUG */

/**
 * printf 形式の書式文字列と引数を, コンパイラに検査させる (GNU 拡張).
 * ほかのコンパイラでは, 何もしない.
 * fmt: 書式文字列の引数の位置, first: 書式の引数の最初の位置 (どちらも 1 から数える)
 */
#ifdef __GNUC__
#  define LOG_FORMAT(fmt, first) __attribute__((format(printf, fmt, first)))
#else
#  define LOG_FORMAT(fmt, first)
#endif

/* プログラム名設定 */
void set_progname(const char *name);

/* プログラム名取得 */
char *get_progname(void);

/* シスログ出力 */
void system_log(const int level,
                const int option,
                const char *pname,
                const char *fname,
                const int line,
                const char *func,
                const char *format,
                ...) LOG_FORMAT(7, 8);

/* シスログ出力(デバッグ用) */
void system_dbg_log(const int level,
                    const int option,
                    const char *pname,
                    const char *fname,
                    const int line,
                    const char *func,
                    const char *format,
                    ...) LOG_FORMAT(7, 8);

/* 標準エラー出力にログ出力 */
void stderr_log(
    const char *pname, const char *fname, const int line, const char *func, const char *format, ...)
    LOG_FORMAT(5, 6);

/* 標準エラー出力にHEXダンプ */
int dump_log(const void *buf, const size_t len, const char *format, ...) LOG_FORMAT(3, 4);

/* シスログにHEXダンプ */
int dump_sys(const int level,
             const int option,
             const char *pname,
             const char *fname,
             const int line,
             const char *func,
             const void *buf,
             const size_t len,
             const char *format,
             ...) LOG_FORMAT(9, 10);

/* ファイルにバイナリ出力 */
int dump_file(const char *pname, const char *fname, const char *buf, const size_t len);

/* バックトレースシスログ出力 */
void systrace(const int level,
              const int option,
              const char *pname,
              const char *fname,
              const int line,
              const char *func);

/* バックトレース出力 */
void print_trace(void);

#endif /* OUTPUTLOG_H */
