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
#include <string.h>   /* strndup memcpy memset */
#include <ctype.h>    /* isdigit isalpha */
#include <math.h>     /* powl */
#include <pthread.h>  /* pthread_once */
#ifdef _DEBUG
#  include <limits.h> /* INT_MAX */
#  include <float.h>  /* DBL_MAX */
#endif

#include "timer.h"
#include "log.h"
#include "util.h"
#include "data.h"
#include "option.h"
#include "function.h"
#include "error.h"
#include "calc.h"

/* 外部変数 */
bool g_tflag = false; /**< tオプションフラグ */

/* 内部変数 */
static const dbl EX_ERROR = 0.0; /**< エラー戻り値 */
static pthread_key_t calc_key; /**< スレッド固有バッファのキー */
static pthread_once_t calc_once = PTHREAD_ONCE_INIT; /**< キー初期化 */

/* 内部関数 */
/** キー確保 */
static void alloc_key(void);
/** バッファ固有のキー確保 */
static void destroy_thread(void *ptr);
/** 式 */
static dbl expression(calcinfo *tsd);
/** 項 */
static dbl term(calcinfo *tsd);
/** 因子 */
static dbl factor(calcinfo *tsd);
/** 数または関数 */
static dbl token(calcinfo *tsd);
/** 文字列を数値に変換 */
static dbl number(calcinfo *tsd);
/** 文字数取得 */
static int get_strlen(const dbl val, const char *fmt);
/** バッファ読込 */
static void readch(calcinfo *tsd);

/**
 * 初期化
 *
 * @param[in] expr 式
 * @param[in] digit 桁数
 * @return calcinfo構造体
 */
calcinfo *
init_calc(void *expr, long digit)
{
    int retval = 0;       /* 戻り値 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    /* 1 回限りのキー初期化 */
    pthread_once(&calc_once, alloc_key);
    /* スレッド固有のバッファ取得 */
    tsd = pthread_getspecific(calc_key);
    if (!tsd) { /* 取得できない場合 */
        /* スレッド固有のバッファ確保 */
        tsd = malloc(sizeof(calcinfo));
        if (!tsd) {
            outlog("tsd=%p", tsd);
            return NULL;
        }
        dbglog("tsd=%p", tsd);
        pthread_setspecific(calc_key, tsd);
    }

    (void)memset(tsd, 0, sizeof(calcinfo));

    tsd->ptr = (uchar *)expr; /* 走査用ポインタ */
    dbglog("expr=%p, ptr=%p", expr, tsd->ptr);

    /* フォーマット設定 */
    retval = snprintf(tsd->fmt, sizeof(tsd->fmt),
                      "%s%ld%s", "%.", digit, "g");
    if (retval < 0) {
        outlog("snprintf=%d", retval);
        return NULL;
    }
    dbglog("fmt=%s", tsd->fmt);

    readch(tsd);

    return tsd;
}


/**
 * メモリ解放
 *
 * @param[in] tsd calcinfo構造体
 * @return なし
 */
void
destroy_calc(calcinfo *tsd)
{
    dbglog("start: result=%p", tsd->result);
    memfree((void **)&tsd->result, NULL);
}

/**
 * 計算結果
 *
 * @param[in] tsd calcinfo構造体
 * @return 新たに領域確保された結果文字列ポインタ
 * @retval NULL エラー
 */
uchar *
answer(calcinfo *tsd)
{
    dbl val = 0;       /* 値 */
    size_t length = 0; /* 文字数 */
    int retval = 0;    /* 戻り値 */
    uint start = 0;    /* タイマ開始 */

    dbglog("start: %p", answer);
//    dbglog("sizeof(dbl)=%u, DBL_MAX=%f", sizeof(dbl), DBL_MAX);
//    dbglog("sizeof(ldbl)=%u, LDBL_MAX=%Lf", sizeof(ldbl), LDBL_MAX);

    if (g_tflag)
        start_timer(&start);

    val = expression(tsd);
    dbglog(tsd->fmt, val);
    dbglog("ptr=%p, ch=%c", tsd->ptr, tsd->ch);

    check_validate(tsd, val);
    if (tsd->ch != '\0') /* エラー */
        set_errorcode(tsd, E_SYNTAX);

    if (g_tflag)
        print_timer(stop_timer(&start));

    if (is_error(tsd)) { /* エラー */
        tsd->errormsg = get_errormsg(tsd);
        if (!tsd->errormsg)
            return NULL;
        length = strlen((char *)tsd->errormsg); /* 文字数 */
        tsd->result = (uchar *)strndup((char *)tsd->errormsg, length);
        clear_error(tsd);
        dbglog("errormsg=%p", tsd->errormsg);
        if (!tsd->result) {
            outlog("strndup=%p", tsd->result);
            return NULL;
        }
        dbglog("result=%p, length=%u", tsd->result, length);
    } else {
        /* 文字数取得 */
        retval = get_strlen(val, tsd->fmt);
        dbglog("retval=%d, INT_MAX=%d", retval, INT_MAX);
        if (retval <= 0) { /* エラー */
            outlog("retval=%d", retval);
            return NULL;
        }
        length = (size_t)retval + 1; /* 文字数 + 1 */

        /* メモリ確保 */
        tsd->result = (uchar *)malloc(length * sizeof(uchar));
        if (!tsd->result) {
            outlog("calloc=%p", tsd->result);
            return NULL;
        }
        (void)memset(tsd->result, 0, length * sizeof(uchar));

        /* 値を文字列に変換 */
        retval = snprintf((char *)tsd->result, length, tsd->fmt, val);
        if (retval < 0) {
            outlog("snprintf=%d, result=%p, length=%u",
                   retval, tsd->result, length);
            return NULL;
        }
        dbglog(tsd->fmt, val);
        dbglog("result=%s, length=%u", tsd->result, length);
    }
    return tsd->result;
}

/**
 * 引数解析
 *
 * @param[in] tsd calcinfo構造体
 * @param[in] type 引数のタイプ
 * @param[out] x 値
 * @param[out] y 値
 * @return なし
 */
void
parse_func_args(calcinfo *tsd, const argtype type, dbl *x, dbl *y)
{
    dbglog("start");

    if (is_error(tsd))
        return;

    if (tsd->ch != '(') {
        set_errorcode(tsd, E_SYNTAX);
        return;
    }

    readch(tsd);
    *x = expression(tsd);
    dbglog(tsd->fmt, *x);

    if (type == ARG_2) {
        if (tsd->ch != ',') {
            set_errorcode(tsd, E_SYNTAX);
            return;
        }
        readch(tsd);
        *y = expression(tsd);
        dbglog(tsd->fmt, *y);
    }

    if (tsd->ch != ')') {
        set_errorcode(tsd, E_SYNTAX);
        return;
    }
    readch(tsd);

}

/**
 * キー確保
 *
 * @return なし
 */
static void
alloc_key(void)
{
    dbglog("start");
    pthread_key_create(&calc_key, destroy_thread);
}

/**
 * スレッド固有バッファ解放
 *
 * @param[in] ptr 解放するポインタ
 * @return なし
 */
static void
destroy_thread(void *ptr)
{
    dbglog("start: ptr=%p", ptr);
    memfree((void **)&ptr, NULL);
}

/**
 * バッファ読込
 *
 * バッファから一文字読み込む.
 * 空白, タブは読み飛ばす.
 *
 * @param[in] tsd calcinfo構造体
 * @return 文字
 */
static void
readch(calcinfo *tsd)
{
    dbglog("start");

    do {
        tsd->ch = (int)*tsd->ptr;
        dbglog("ptr=%p, ch=%c", tsd->ptr, tsd->ch);
        if (tsd->ch == '\0')
            break;
        tsd->ptr++;
    } while (isblank(tsd->ch));
}

/**
 * 式
 *
 * @param[in] tsd calcinfo構造体
 * @return 値
 */
static dbl
expression(calcinfo *tsd)
{
    dbl x = 0; /* 値 */

    dbglog("start");

    if (is_error(tsd))
        return EX_ERROR;

    x = term(tsd);
    dbglog(tsd->fmt, x);

    while (true) {
        if (tsd->ch == '+') {
            readch(tsd);
            x += term(tsd);
        } else if (tsd->ch == '-') {
            readch(tsd);
            x -= term(tsd);
        } else {
            break;
        }
    }

    dbglog(tsd->fmt, x);
    return x;
}

/**
 * 項
 *
 * @param[in] tsd calcinfo構造体
 * @return 結果
 */
static dbl
term(calcinfo *tsd)
{
    dbl x = 0, y = 0;     /* 値 */

    dbglog("start");

    if (is_error(tsd))
        return EX_ERROR;

    x = factor(tsd);
    dbglog(tsd->fmt, x);

    while (true) {
        if (tsd->ch == '*') {
            readch(tsd);
            x *= factor(tsd);
        } else if (tsd->ch == '/') {
            readch(tsd);
            y = factor(tsd);
            if (y == 0) { /* ゼロ除算エラー */
                set_errorcode(tsd, E_DIVBYZERO);
                return EX_ERROR;
            }
            x /= y;
        } else if (tsd->ch == '^') {
            readch(tsd);
            y = factor(tsd);
            x = get_pow(tsd, x, y);
        } else {
            break;
        }
    }
    dbglog(tsd->fmt, x);
    return x;
}

/**
 * 因子
 *
 * @param[in] tsd calcinfo構造体
 * @return 値
 */
static dbl
factor(calcinfo *tsd)
{
    dbl x = 0; /* 値 */

    dbglog("start");

    if (is_error(tsd))
        return EX_ERROR;

    if (tsd->ch != '(')
        return token(tsd);

    readch(tsd);
    x = expression(tsd);

    if (tsd->ch != ')') { /* シンタックスエラー */
        set_errorcode(tsd, E_SYNTAX);
        return EX_ERROR;
    }
    readch(tsd);

    dbglog(tsd->fmt, x);
    return x;
}

/**
 * 数または関数
 *
 * @param[in] tsd calcinfo構造体
 * @return 結果
 */
static dbl
token(calcinfo *tsd)
{
    dbl result = 0;                 /* 結果 */
    int sign = '+';                 /* 単項+- */
    char func[MAX_FUNC_STRING + 1]; /* 関数文字列 */
    int pos = 0;                    /* 配列位置 */

    dbglog("start");

    if (is_error(tsd))
        return EX_ERROR;

    /* 初期化 */
    (void)memset(func, 0, sizeof(func));

    if (tsd->ch == '+' || tsd->ch == '-') { /* 単項+- */
        sign = tsd->ch;
        readch(tsd);
    }

    if (isdigit(tsd->ch)) { /* 数値 */
        result = number(tsd);
    } else if (isalpha(tsd->ch)) { /* 関数 */
        while (isalpha(tsd->ch) && tsd->ch != '\0' &&
               pos <= MAX_FUNC_STRING) {
            func[pos++] = tsd->ch;
            readch(tsd);
        }
        dbglog("func=%s", func);

        result = exec_func(tsd, func);

    } else { /* エラー */
        dbglog("ch=%c", tsd->ch);
        set_errorcode(tsd, E_SYNTAX);
    }

    dbglog(tsd->fmt, result);
    return (sign == '+') ? result : -result;
}

/**
 * 文字列を数値に変換
 *
 * @param[in] tsd calcinfo構造体
 * @return 数値
 */
static dbl
number(calcinfo *tsd)
{
    dbl x = 0, y = 1; /* 値 */

    dbglog("start");

    x = tsd->ch - '0';
    while (readch(tsd), isdigit(tsd->ch)) /* 整数 */
        x = (x * 10) + (tsd->ch - '0');
    dbglog(tsd->fmt, x);

    if (tsd->ch == '.') { /* 小数 */
        while (readch(tsd), isdigit(tsd->ch))
            x += (y /= 10) * (tsd->ch - '0');
    }
    dbglog(tsd->fmt, x);

    check_validate(tsd, x);

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
    int result = 0;  /* 文字数 */

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

#ifdef UNITTEST
void
test_init_calc(testcalc *calc)
{
    calc->expression = expression;
    calc->term = term;
    calc->factor = factor;
    calc->token = token;
    calc->number = number;
    calc->get_strlen = get_strlen;
    calc->readch = readch;
}
#endif /* UNITTEST */

