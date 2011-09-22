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

#include <stdio.h>    /* FILE */
#include <stdlib.h>   /* realloc */
#include <string.h>   /* strndup */
#include <stdbool.h>  /* bool */
#include <string.h>   /* memcpy memset */
#include <ctype.h>    /* isdigit isalpha */
#include <math.h>     /* powl */
#if defined(_DEBUG)
#  include <limits.h> /* INT_MAX */
#  include <float.h>  /* DBL_MAX */
#endif

#include "timer.h"
#include "log.h"
#include "data.h"
#include "error.h"
#include "option.h"
#include "calc.h"

#define EX_ERROR    0 /* エラー戻り値 */

/* 外部変数 */
int digit = -1;     /**< 桁数 */
bool tflag = false; /**< tオプションフラグ */

/* 内部変数 */
static int ch = 0;        /**< 文字 */
static uchar *ptr = NULL; /**< 文字列ポインタ */

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
    dbglog("x=%.18g", x);

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

    dbglog("x=%.18g", x);
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
    dbglog("x=%.18g", x);

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
    dbglog("x=%.18g", x);
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

    dbglog("x=%.18g", x);
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
    dbl result = 0;                /* 結果 */
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

    dbglog("result=%.18g", result);
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
    dbglog("x=%.18g", x);

    if (ch == '.') { /* 小数 */
        while (readch(), isdigit(ch))
            x += (y /= 10) * (ch - '0');
    }
    dbglog("x=%.18g", x);

    check_validate(1, x);

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

    dbglog("start");

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

    return result;
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
 * 引数解析
 *
 * @param[in] num 引数の数
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
    dbglog("val=%.18g", *x);

    if (type == ARG_2) {
        if (ch != ',') {
            set_errorcode(E_SYNTAX);
            return;
        }
        readch();
        *y = expression();
        dbglog("val=%.18g", *x);
    }

    if (ch != ')') {
        set_errorcode(E_SYNTAX);
        return;
    }
    readch();

}

/**
 * 入力
 *
 * @param[in] buf 文字列バッファポインタ
 * @param[in] len 文字列の長さ
 * @return 新たに領域確保された結果文字列ポインタ
 * @retval NULL エラー
 */
uchar *
input(uchar *buf, const size_t len)
{
    dbl val = 0;               /* 値 */
    size_t length = 0;         /* 文字数 */
    uchar *result = NULL;      /* 結果 */
    uchar *errormsg = NULL;    /* エラーメッセージ */
    int retval = 0;            /* 戻り値 */
    char fmt[sizeof("%.18g")]; /* フォーマット */
    uint t = 0, time = 0;      /* タイマ用変数 */

    dbglog("start: %p", input);
    dbglog("sizeof(double)=%u, DBL_MAX=%g", sizeof(double), DBL_MAX);

    /* 初期値設定 */
    ptr = buf;  /* ポインタ設定 */

    if (tflag)
        start_timer(&t);
    
    readch();
    val = expression();
    dbglog("val=%g, ptr=%p, ch=%c", val, ptr, ch);

    check_validate(1, val);
    if (ch != '\0') /* エラー */
        set_errorcode(E_SYNTAX);

    if (tflag) {
        time = stop_timer(&t);   
        print_timer(time);
    }

    if (is_error()) { /* エラー */
        errormsg = get_errormsg();
        if (!errormsg)
            return NULL;
        length = strlen((char *)errormsg); /* 文字数 */
        result = (uchar *)strndup((char *)errormsg, length);
        clear_error(&errormsg);
        dbglog("errormsg=%p", errormsg);
        if (!result) {
            outlog("strndup=%p", result);
            return NULL;
        }
        dbglog("result=%p, length=%u", result, length);
    } else {
        /* フォーマット設定 */
        if (digit == -1) /* デフォルト */
            digit = DEFAULT_DIGIT;
        retval = snprintf(fmt, sizeof(fmt), "%s%d%s",
                          "%.", digit, "g");

        if (retval < 0) {
            outlog("snprintf=%d", retval);
            return NULL;
        }
        dbglog("fmt=%s", fmt);
        /* 文字数取得 */
        retval = get_strlen(val, fmt);
        dbglog("len=%d, INT_MAX=%d", len, INT_MAX);
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
        retval = snprintf((char *)result, length, fmt, val);
        if (retval < 0) {
            outlog("snprintf=%d, result=%p, length=%u",
                   retval, result, length);
            return NULL;
        }
        dbglog("result=%s, val=%.18g, length=%u", result, val, length);
    }
    return result;
}

#ifdef _UT
int
test_get_strlen(const dbl val, const char *fmt)
{
    return get_strlen(val, fmt);
}
#endif

