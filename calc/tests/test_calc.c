/**
 * @file  calc/tests/test_calc.c
 * @brief 単体テスト
 *
 * @author higashi
 * @date 2011-09-24 higashi 新規作成
 * @version \$Id$
 *
 * Copyright (C) 2011 Tetsuya Higashi. All Rights Reserved.
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

#include <cutter.h> /* cutter library */

#include "def.h"
#include "util.h"
#include "log.h"
#include "calc.h"

#define MAX_STRING  32 /**< 最大文字列 */

/* プロトタイプ */
/** 四則演算テスト */
void test_answer_four(void);
/** 関数テスト */
void test_answer_func(void);
/** 四則演算と関数の組み合わせテスト */
void test_answer_four_func(void);
/** 関数エラー時テスト */
void test_answer_error(void);
/** parse_func_args() 関数テスト */
void test_parse_func_args(void);
/** readch() 関数テスト */
void test_readch(void);
/** expression() 関数テスト */
void test_expression(void);
/** term() 関数テスト */
void test_term(void);
/** factor() 関数テスト */
void test_factor(void);
/** token() 関数テスト */
void test_token(void);
/** number() 関数テスト */
void test_number(void);
/** get_strlen() 関数テスト */
void test_get_strlen(void);

/* 内部変数 */
static testcalc calc; /**< 関数構造体 */

/* 内部関数 */
/** 文字列設定 */
static calcinfo *set_string(const char *str);
/** バッファセット */
static calcinfo *exec_calc(const char *str);

/** テストデータ構造体(answer char) */
struct test_data_char {
    char expr[MAX_STRING];
    char answer[MAX_STRING];
};

/** テストデータ構造体(answer double) */
struct test_data_dbl {
    char expr[MAX_STRING];
    dbl answer;
};

/** 四則演算テスト用データ */
static const struct test_data_char four_data [] = {
    { "(105+312)+2*(5-3)", "421" },
    { "(105+312)+2/(5-3)", "418" },
    { "1+2*(5-3)",         "5"   },
    { "1+2/(5-3)",         "2"   }
};

/** 関数テスト用データ */
static const struct test_data_char func_data [] = {
    { "pi",        "3.14159265359"   },
    { "e",         "2.71828182846"   },
    { "abs(-2)",   "2"               },
    { "sqrt(2)",   "1.41421356237"   },
    { "sin(2)" ,   "0.909297426826"  },
    { "cos(2)",    "-0.416146836547" },
    { "tan(2)",    "-2.18503986326"  },
    { "asin(0.5)", "0.523598775598"  },
    { "acos(0.5)", "1.0471975512"    },
    { "atan(0.5)", "0.463647609001"  },
    { "exp(2)" ,   "7.38905609893"   },
    { "ln(2)",     "0.69314718056"   },
    { "log(2)",    "0.301029995664"  },
    { "deg(2)",    "114.591559026"   },
    { "rad(2)",    "0.0349065850399" },
    { "n(10)",     "3628800"         },
    { "nPr(5,2)",  "20"              },
    { "nCr(5,2)",  "10"              }
};

/** 四則演算と関数の組み合わせテスト用データ */
static const struct test_data_char four_func_data [] = {
    { "5*pi",        "15.7079632679"  },
    { "pi*5",        "15.7079632679"  },
    { "5*e",         "13.5914091423"  },
    { "e*5",         "13.5914091423"  },
    { "5*abs(-2)",   "10"             },
    { "abs(-2)*5",   "10"             },
    { "5*sqrt(2)",   "7.07106781187"  },
    { "sqrt(2)*5",   "7.07106781187"  },
    { "5*sin(2)",    "4.54648713413"  },
    { "sin(2)*5",    "4.54648713413"  },
    { "5*cos(2)",    "-2.08073418274" },
    { "cos(2)*5",    "-2.08073418274" },
    { "5*tan(2)",    "-10.9251993163" },
    { "tan(2)*5",    "-10.9251993163" },
    { "2*asin(0.5)", "1.0471975512"   },
    { "asin(0.5)*2", "1.0471975512"   },
    { "2*acos(0.5)", "2.09439510239"  },
    { "acos(0.5)*2", "2.09439510239"  },
    { "5*atan(0.5)", "2.318238045"    },
    { "atan(0.5)*5", "2.318238045"    },
    { "5*exp(2)",    "36.9452804947"  },
    { "exp(2)*5",    "36.9452804947"  },
    { "5*ln(2)",     "3.4657359028"   },
    { "ln(2)*5",     "3.4657359028"   },
    { "5*log(2)",    "1.50514997832"  },
    { "log(2)*5",    "1.50514997832"  },
    { "5*deg(2)",    "572.957795131"  },
    { "deg(2)*5",    "572.957795131"  },
    { "5*rad(2)",    "0.174532925199" },
    { "rad(2)*5",    "0.174532925199" },
    { "5*n(10)",     "18144000"       },
    { "n(10)*5",     "18144000"       },
    { "5*nPr(5,2)",  "100"            },
    { "nPr(5,2)*5",  "100"            },
    { "5*nCr(5,2)",  "50"             },
    { "nCr(5,2)*5",  "50"             }
};

/** 関数エラー時テスト用データ */
static const struct test_data_char error_data [] = {
    { "5/0",        "Divide by zero."       },
    { "sin(5",      "Syntax error."         },
    { "nCr(5)",     "Syntax error."         },
    { "nofunc(5)",  "Function not defined." },
    { "n(0.5)",     "NaN."                  },
    { "nPr(-1,-2)", "NaN."                  },
    { "nPr(1,-2)",  "NaN."                  },
    { "nPr(3,5)",   "NaN."                  },
    { "nCr(-1,-2)", "NaN."                  },
    { "nCr(1,-2)",  "NaN."                  },
    { "nCr(3,5)",   "NaN."                  },
    { "sqrt(-5)",   "NaN."                  },
    { "10^1000000", "Infinity."             },
    { "n(5000)",    "Infinity."             },
    { "n(-5000)",   "Infinity."             }
};

/** expression() 関数テスト用データ */
static const struct test_data_dbl expression_data [] = {
    { "5+7", 12 },
    { "5-1", 4  }
};

/** term() 関数テスト用データ */
static const struct test_data_dbl term_data [] = {
    { "5*7", 35 },
    { "6/2", 3  },
    { "6/0", 0  },
    { "6^2", 36 }
};

/** factor() 関数テスト用データ */
static const struct test_data_dbl factor_data [] = {
    { "(5+4)", 9 },
    { "(5+4",  0 }
};

/** token() 関数テスト用データ */
static const struct test_data_dbl token_data [] = {
    { "+54321",   54321  },
    { "-54321",   -54321 },
    { "54231",    54231  },
    { "nCr(5,2)", 10     },
    { "テスト",   0      }
};

/** number() 関数テスト用データ */
static const struct test_data_dbl number_data [] = {
    { "54231",   54231  },
    { "54.231",  54.231 },
};

/**
 * 初期化処理
 *
 * @return なし
 */
void cut_startup(void)
{
    test_init_calc(&calc);
}

/**
 * 四則演算テスト
 *
 * @return なし
 */
void
test_answer_four(void)
{
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(four_data); i++) {
        tsd = exec_calc(four_data[i].expr);
        cut_assert_equal_string(four_data[i].answer,
                                (char *)tsd->result,
                                cut_message("%s == %s",
                                            four_data[i].answer,
                                            four_data[i].expr));
        destroy_calc(tsd);
    }
}

/**
 * 関数テスト
 *
 * @return なし
 */
void
test_answer_func(void)
{
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(func_data); i++) {
        tsd = exec_calc(func_data[i].expr);
        cut_assert_equal_string(func_data[i].answer,
                                (char *)tsd->result,
                                cut_message("%s == %s",
                                            func_data[i].answer,
                                            func_data[i].expr));
        destroy_calc(tsd);
    }
}

/**
 * 四則演算と関数の組み合わせテスト
 *
 * @return なし
 */
void
test_answer_four_func(void)
{
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(four_func_data); i++) {
        tsd = exec_calc(four_func_data[i].expr);
        cut_assert_equal_string(four_func_data[i].answer,
                                (char *)tsd->result,
                                cut_message("%s == %s",
                                            four_func_data[i].answer,
                                            four_func_data[i].expr));
        destroy_calc(tsd);
    }
}

/**
 * 関数エラー時テスト
 *
 * @return なし
 */
void
test_answer_error(void)
{
    calcinfo *tsd = NULL; /* calc情報構造体 */

    int i;
    for (i = 0; i < arraysize(error_data); i++) {
        tsd = exec_calc(error_data[i].expr);
        cut_assert_equal_string(error_data[i].answer,
                                (char *)tsd->result,
                                cut_message("%s == %s",
                                            error_data[i].answer,
                                            error_data[i].expr));
        destroy_calc(tsd);
    }
}

/**
 * parse_func_args() 関数テスト
 *
 * @return なし
 */
void
test_parse_func_args(void)
{
    dbl x = 0, y = 0;     /* 値 */
    calcinfo *tsd = NULL; /* calc情報構造体 */

    tsd = set_string("(235)");
    if (!tsd) {
        outlog("tsd=%p", tsd);
        return;
    }
    dbglog("ch=%p", tsd->ch);
    parse_func_args(tsd, ARG_1, &x, &y);
    cut_assert_equal_double(235, 0.0, x);
    destroy_calc(tsd);

    tsd = set_string("(123,235)");
    if (!tsd) {
        outlog("tsd=%p", tsd);
        return;
    }
    dbglog("ch=%p", tsd->ch);
    parse_func_args(tsd, ARG_2, &x, &y);
    cut_assert_equal_double(123, 0.0, x);
    cut_assert_equal_double(235, 0.0, y);
    destroy_calc(tsd);
}

/**
 * readch() 関数テスト
 *
 * @return なし
 */
void
test_readch(void)
{
    uchar *ptr = NULL;    /* ポインタ */
    calcinfo *tsd = NULL; /* calc情報構造体 */

    tsd = set_string("test");
    if (!tsd) {
        outlog("tsd=%p", tsd);
        return;
    }
    dbglog("ch=%p", tsd->ch);
    cut_assert_equal_int('t', tsd->ch,
                         cut_message("%c == %c", 't', tsd->ch));
    calc.readch(tsd);
    cut_assert_equal_int('e', tsd->ch,
                         cut_message("%c == %c", 'e', tsd->ch));
    calc.readch(tsd);
    cut_assert_equal_int('s', tsd->ch,
                         cut_message("%c == %c", 's', tsd->ch));
    calc.readch(tsd);
    cut_assert_equal_int('t', tsd->ch,
                         cut_message("%c == %c", 't', tsd->ch));
    calc.readch(tsd);
    cut_assert_equal_int('\0', tsd->ch,
                         cut_message("ptr=%p", tsd->ptr));
    ptr = tsd->ptr; /* アドレス保持 */
    calc.readch(tsd);
    cut_assert_equal_int('\0', tsd->ch,
                         cut_message("ptr=%p", tsd->ptr));
    /* アドレスが変わらないことを確認 */
    cut_assert_equal_memory(ptr, sizeof(ptr), tsd->ptr, sizeof(tsd->ptr));
}

/**
 * expression() 関数テスト
 *
 * @return なし
 */
void
test_expression(void)
{
    dbl x = 0;            /* 値 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(expression_data); i++) {
        tsd = set_string(expression_data[i].expr);
        if (!tsd)
            return;
        x = calc.expression(tsd);
        cut_assert_equal_double(expression_data[i].answer,
                                0.0,
                                x,
                                cut_message("%g == %s",
                                            expression_data[i].answer,
                                            expression_data[i].expr));
        destroy_calc(tsd);
    }
}

/**
 * term() 関数テスト
 *
 * @return なし
 */
void
test_term(void)
{
    dbl x = 0;            /* 値 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(term_data); i++) {
        tsd = set_string(term_data[i].expr);
        if (!tsd)
            return;
        x = calc.term(tsd);
        cut_assert_equal_double(term_data[i].answer,
                                0.0,
                                x,
                                cut_message("%g == %s",
                                            term_data[i].answer,
                                            term_data[i].expr));
        destroy_calc(tsd);
    }
}

/**
 * factor() 関数テスト
 *
 * @return なし
 */
void
test_factor(void)
{
    dbl x = 0;            /* 値 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(factor_data); i++) {
        tsd = set_string(factor_data[i].expr);
        if (!tsd)
            return;
        x = calc.factor(tsd);
        cut_assert_equal_double(factor_data[i].answer,
                                0.0,
                                x,
                                cut_message("%g == %s",
                                            factor_data[i].answer,
                                            factor_data[i].expr));
        destroy_calc(tsd);
    }
}

/**
 * factor() 関数テスト
 *
 * @return なし
 */
void
test_token(void)
{
    dbl x = 0;            /* 値 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(token_data); i++) {
        tsd = set_string(token_data[i].expr);
        if (!tsd)
            return;
        x = calc.token(tsd);
        cut_assert_equal_double(token_data[i].answer,
                                0.0,
                                x,
                                cut_message("%g == %s",
                                            token_data[i].answer,
                                            token_data[i].expr));
        destroy_calc(tsd);
    }
}

/**
 * number() 関数テスト
 *
 * @return なし
 */
void
test_number(void)
{
    dbl x = 0;            /* 値 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    int i;
    for (i = 0; i < arraysize(number_data); i++) {
        tsd = set_string(number_data[i].expr);
        if (!tsd)
            return;
        x = calc.number(tsd);
        cut_assert_equal_double(number_data[i].answer,
                                0.0,
                                x,
                                cut_message("%g == %s",
                                            number_data[i].answer,
                                            number_data[i].expr));
        destroy_calc(tsd);
    }
}

/**
 * get_strlen() 関数テスト
 *
 * @return なし
 */
void
test_get_strlen(void)
{
    cut_assert_equal_int(5, calc.get_strlen(50000, "%.18g"));
    cut_assert_equal_int(15, calc.get_strlen(123456789012345, "%.18g"));
    // 12345678.9000000004
    cut_assert_equal_int(19, calc.get_strlen(12345678.9, "%.18g"));
    // 1234567.89012344996
    cut_assert_equal_int(19, calc.get_strlen(1234567.89012345, "%.18g"));

    cut_assert_equal_int(5, calc.get_strlen(50000, "%.15g"));
    cut_assert_equal_int(15, calc.get_strlen(123456789012345, "%.15g"));
    // 12345678.9
    cut_assert_equal_int(10, calc.get_strlen(12345678.9, "%.15g"));
    // 1234567.89012345
    cut_assert_equal_int(16, calc.get_strlen(1234567.89012345, "%.15g"));

    // 5e+04
    cut_assert_equal_int(5, calc.get_strlen(50000, "%.1g"));
    // 1e+14
    cut_assert_equal_int(5, calc.get_strlen(123456789012345, "%.1g"));
    // 1e+07
    cut_assert_equal_int(5, calc.get_strlen(12345678.9, "%.1g"));
    // 1e+06
    cut_assert_equal_int(5, calc.get_strlen(1234567.89012345, "%.1g"));

    cut_assert_equal_int(5, calc.get_strlen(50000, "%.30g"));
    cut_assert_equal_int(15, calc.get_strlen(123456789012345, "%.30g"));
    // 12345678.9000000003725290298462
    cut_assert_equal_int(31, calc.get_strlen(12345678.9, "%.30g"));
    // 1234567.89012344996444880962372
    cut_assert_equal_int(31, calc.get_strlen(1234567.89012345, "%.30g"));
}

/**
 * 文字列設定
 *
 * @param[in] str 文字列
 * @return calcinfo構造体
 */
static calcinfo *
set_string(const char *str)
{
    size_t length = 0;    /* 文字列長 */
    uchar *expr = NULL;   /* 式 */
    calcinfo *tsd = NULL; /* calcinfo構造体 */

    length = strlen(str);
    expr = (uchar *)cut_take_strndup(str, length);
    if (!expr) {
        outlog("cut_take_strndup=%p", expr);
        return NULL;
    }

    tsd = init_calc(expr, 12L);
    if (!tsd) {
        outlog("tsd=%p", tsd);
        return NULL;
    }
    dbglog("tsd=%p", tsd);
    dbglog("%p expr=%s, length=%u", expr, expr, length);
    return tsd;
}

/**
 * 計算実行
 *
 * @param[in] str 文字列
 * @return calcinfo構造体
 */
static calcinfo *
exec_calc(const char *str)
{
    calcinfo *tsd = NULL;
    tsd = set_string(str);
    if (!tsd)
        return NULL;
    tsd->result = answer(tsd);
    if (!tsd->result)
        return NULL;
    dbglog("%p result=%s", tsd->result, tsd->result);

    return tsd;
}

