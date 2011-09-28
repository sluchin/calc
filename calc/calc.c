/**
 * @file  calc/calc.c
 * @brief 関数電卓インタプリタ
 *
 * 式とは、項を + または - でつないだものであり、項とは因子を * または / で\n
 * つないだものであり、因子とは数または ( ) で囲んだ式である.\n
 * 再帰的下降構文解析を使用する.\n
 * 加減乗除 [+,-,*,/]、括弧 [(,)]、正負符号 [+,-]、べき乗[^]、\n
 * 関数 [abs,sqrt,sin,cos,tan,asin,acos,atan,exp,ln,log,deg,rad,n,nPr,nCr]、\n
 * 定数 [pi,e] が使用できる.
 *
 * @author higashi
 * @date 2010-06-27 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2010 Tetsuya Higashi. All Rights Reserved.
 */ /* This program is free software; you can redistribute it and/or modify
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

#include <stdio.h>    /* FILE */
#include <stdlib.h>   /* realloc */
#include <string.h>   /* strndup */
#include <stdbool.h>  /* bool */
#include <string.h>   /* memcpy memset */
#include <ctype.h>    /* isdigit isalpha */
#include <math.h>     /* powl */
#ifdef _DEBUG
#  include <limits.h> /* INT_MAX */
#  include <float.h>  /* DBL_MAX */
#endif

#include "timer.h"
#include "log.h"
#include "util.h"
#include "data.h"
#include "error.h"
#include "option.h"
#include "calc.h"

/* 外部変数 */
bool g_tflag = false; /**< tオプションフラグ */

/* 内部変数 */
static int ch = 0;                   /**< 文字 */
static uchar *ptr = NULL;            /**< 文字列走査用ポインタ */
static uchar *result = NULL;         /**< 結果 */
static char format[sizeof("%.18g")]; /**< フォーマット */

/* 内部関数 */
/** 式 */
static dbl expression(void);
/** 項 */
static dbl term(void);
/** 因子 */
static dbl factor(void);
/** 数または関数 */
static dbl token(void);
/** 文字列を数値に変換 */
static dbl number(void);
/** 文字数取得 */
static int get_strlen(const dbl val, const char *fmt);
/** バッファ読込 */
static void readch(void);

/**
 * 初期化
 *
 * @param[in] expr 式
 * @param[in] digit 桁数
 * @return なし
 */
void
init_calc(void *expr, long digit)
{
    int retval = 0; /* 戻り値 */

    ptr = (uchar *)expr; /* 走査用ポインタ */

    /* フォーマット設定 */
    retval = snprintf(format, sizeof(format), "%s%ld%s",
                      "%.", digit, "g");
    if (retval < 0) {
        outlog("snprintf=%d", retval);
        return;
    }
    dbglog("format=%s", format);
}

/**
 * メモリ解放
 *
 * @return なし
 */
void
destroy_calc(void)
{
    dbglog("start: result=%p", result);
    memfree(1, &result);
    dbglog("result=%p", result);
}

/**
 * 計算結果
 *
 * @return 新たに領域確保された結果文字列ポインタ
 * @retval NULL エラー
 */
uchar *
answer(void)
{
    dbl val = 0;            /* 値 */
    size_t length = 0;      /* 文字数 */
    uchar *errormsg = NULL; /* エラーメッセージ */
    int retval = 0;         /* 戻り値 */
    uint t = 0, time = 0;   /* タイマ用変数 */

    dbglog("start: %p", answer);
//    dbglog("sizeof(dbl)=%u, DBL_MAX=%f", sizeof(dbl), DBL_MAX);
//    dbglog("sizeof(ldbl)=%u, LDBL_MAX=%Lf", sizeof(ldbl), LDBL_MAX);

    if (g_tflag)
        start_timer(&t);

    readch();
    val = expression();
    dbglog(format, val);
    dbglog("ptr=%p, ch=%c", ptr, ch);

    check_validate(val);
    if (ch != '\0') /* エラー */
        set_errorcode(E_SYNTAX);

    if (g_tflag) {
        time = stop_timer(&t);
        print_timer(time);
    }

    if (is_error()) { /* エラー */
        errormsg = get_errormsg();
        if (!errormsg)
            return NULL;
        length = strlen((char *)errormsg); /* 文字数 */
        result = (uchar *)strndup((char *)errormsg, length);
        clear_error();
        dbglog("errormsg=%p", errormsg);
        if (!result) {
            outlog("strndup=%p", result);
            return NULL;
        }
        dbglog("result=%p, length=%u", result, length);
    } else {
        /* 文字数取得 */
        retval = get_strlen(val, format);
        dbglog("retval=%d, INT_MAX=%d", retval, INT_MAX);
        if (retval <= 0) { /* エラー */
            outlog("retval=%d", retval);
            return NULL;
        }
        length = (size_t)retval + 1; /* 文字数 + 1 */

        /* メモリ確保 */
        result = (uchar *)malloc(length * sizeof(uchar));
        if (!result) {
            outlog("calloc=%p", result);
            return NULL;
        }
        (void)memset(result, 0, length * sizeof(uchar));

        /* 値を文字列に変換 */
        retval = snprintf((char *)result, length, format, val);
        if (retval < 0) {
            outlog("snprintf=%d, result=%p, length=%u",
                   retval, result, length);
            return NULL;
        }
        dbglog(format, val);
        dbglog("result=%s, length=%u", result, length);
    }
    return result;
}

/**
 * 引数解析
 *
 * @param[in] type 引数のタイプ
 * @param[out] x 値
 * @param[out] y 値
 * @return なし
 */
void
parse_func_args(const enum argtype type, dbl *x, dbl *y)
{
    dbglog("start");

    if (is_error())
        return;

    if (ch != '(') {
        set_errorcode(E_SYNTAX);
        return;
    }

    readch();
    *x = expression();
    dbglog("x=%s", format, *x);
    check_validate(*x);

    if (type == ARG_2) {
        if (ch != ',') {
            set_errorcode(E_SYNTAX);
            return;
        }
        readch();
        *y = expression();
        dbglog("y=%s", format, *y);
        check_validate(*y);
    }

    if (ch != ')') {
        set_errorcode(E_SYNTAX);
        return;
    }
    readch();

}

/**
 * バッファ読込
 *
 * バッファから一文字読み込む.
 * 空白, タブは読み飛ばす.
 *
 * @return なし
 */
static void
readch(void)
{
    dbglog("start");

    do {
        ch = (int)*ptr;
        dbglog("ptr=%p, ch=%c", ptr, ch);
        if (ch == '\0')
            break;
        ptr++;
    } while (isblank(ch));
}

/**
 * 式
 *
 * @return 値
 */
static dbl
expression(void)
{
    dbl x = 0; /* 値 */

    dbglog("start");

    if (is_error())
        return EX_ERROR;

    x = term();
    dbglog(format, x);

    while (true) {
        if (ch == '+') {
            readch();
            x += term();
        } else if (ch == '-') {
            readch();
            x -= term();
        } else {
            break;
        }
    }

    dbglog(format, x);
    return x;
}

/**
 * 項
 *
 * @return 結果
 */
static dbl
term(void)
{
    dbl x = 0, y = 0;     /* 値 */

    dbglog("start");

    if (is_error())
        return EX_ERROR;

    x = factor();
    dbglog(format, x);

    while (true) {
        if (ch == '*') {
            readch();
            x *= factor();
        } else if (ch == '/') {
            readch();
            y = factor();
            if (y == 0) { /* ゼロ除算エラー */
                set_errorcode(E_DIVBYZERO);
                return EX_ERROR;
            }
            x /= y;
        } else if (ch == '^') {
            readch();
            y = factor();
            x = get_pow(x, y);
        } else {
            break;
        }
    }
    dbglog(format, x);
    return x;
}

/**
 * 因子
 *
 * @return 値
 */
static dbl
factor(void)
{
    dbl x = 0; /* 値 */

    dbglog("start");

    if (is_error())
        return EX_ERROR;

    if (ch != '(')
        return token();

    readch();
    x = expression();

    if (ch != ')') { /* シンタックスエラー */
        set_errorcode(E_SYNTAX);
        return EX_ERROR;
    }
    readch();

    dbglog(format, x);
    return x;
}

/**
 * 数または関数
 *
 * @return 結果
 */
static dbl
token(void)
{
    dbl result = 0;                 /* 結果 */
    int sign = '+';                 /* 単項+- */
    char func[MAX_FUNC_STRING + 1]; /* 関数文字列 */
    int pos = 0;                    /* 配列位置 */

    dbglog("start");

    if (is_error())
        return EX_ERROR;

    /* 初期化 */
    (void)memset(func, 0, sizeof(func));

    if (ch == '+' || ch == '-') { /* 単項+- */
        sign = ch;
        readch();
    }

    if (isdigit(ch)) { /* 数値 */
        result = number();
    } else if (isalpha(ch)) { /* 関数 */
        while (isalpha(ch) && ch != '\0' && pos <= MAX_FUNC_STRING) {
            func[pos++] = ch;
            readch();
        }
        dbglog("func=%s", func);

        result = exec_func(func);

    } else { /* エラー */
        dbglog("not isdigit && not isalpha");
        set_errorcode(E_SYNTAX);
    }

    dbglog(format, result);
    return (sign == '+') ? result : -result;
}

/**
 * 文字列を数値に変換
 *
 * @return 数値
 */
static dbl
number(void)
{
    dbl x = 0, y = 1; /* 値 */

    dbglog("start");

    x = ch - '0';
    while (readch(), isdigit(ch)) /* 整数 */
        x = (x * 10) + (ch - '0');
    dbglog(format, x);

    if (ch == '.') { /* 小数 */
        while (readch(), isdigit(ch))
            x += (y /= 10) * (ch - '0');
    }
    dbglog(format, x);

    check_validate(x);

    return x;
}

/**
 * 文字数取得
 *
 * @param[in] val 値
 * @param[in] fmt フォーマット
 * @return 文字数
 * @retval  0 fopenエラー
 * @retval -1 fprintfエラー
 */
static int
get_strlen(const dbl val, const char *fmt)
{
    FILE *fp = NULL; /* ファイルポインタ */
    int retval = 0;  /* fclose戻り値 */
    int result = 0;  /* 文字数数 */

    dbglog("start: fmt=%s", fmt);
    dbglog(fmt, val);

    fp = fopen("/dev/null", "w");
    if (!fp) { /* fopen エラー */
        outlog("fopen=%p", fp);
    } else {
        result = fprintf(fp, fmt, val);
        if (result < 0)
            outlog("fprintf=%d", retval);

        retval = fclose(fp);
        if (retval == EOF) /* fclose エラー */
            outlog("fclose=%d", retval);
    }

    dbglog("result=%d", result);
    return result;
}

#ifdef _UT
void
test_init_calc(struct test_calc_func *func)
{
    func->expression = expression;
    func->term = term;
    func->factor = factor;
    func->token = token;
    func->number = number;
    func->get_strlen = get_strlen;
    func->readch = readch;
}
#endif /* _UT */

