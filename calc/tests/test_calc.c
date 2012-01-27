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

#include <fcntl.h>  /* open */
#include <unistd.h> /* close dup2 */
#include <errno.h>  /* errno */
#include <cutter.h> /* cutter library */

#include "def.h"
#include "log.h"
#include "error.h"
#include "memfree.h"
#include "calc.h"
#include "test_common.h"

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
/** set_digit() 関数テスト */
void test_set_digit(void);
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
static testcalc st_calc; /**< 関数構造体 */

/* 内部関数 */
/** バッファセット */
static void exec_calc(calcinfo *calc, const char *str);

/** テストデータ構造体(answer char) */
struct test_data_char {
    char expr[MAX_STRING];
    char answer[MAX_STRING];
};

/** テストデータ構造体(answer double) */
struct test_data_dbl {
    char expr[MAX_STRING];
    dbl answer;
    ER errorcode;
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
    { "5+7", 12, E_NONE },
    { "5-1",  4, E_NONE }
};

/** term() 関数テスト用データ */
static const struct test_data_dbl term_data [] = {
    { "5*7", 35,   E_NONE      },
    { "6/2",  3,   E_NONE      },
    { "6^2", 36,   E_NONE      },
    { "6/0",  0.0, E_DIVBYZERO }
};

/** factor() 関数テスト用データ */
static const struct test_data_dbl factor_data [] = {
    { "(5+4)", 9,   E_NONE   },
    { "(5+4",  0.0, E_SYNTAX }
};

/** token() 関数テスト用データ */
static const struct test_data_dbl token_data [] = {
    { "+54321",    54321,   E_NONE   },
    { "-54321",   -54321,   E_NONE   },
    { "54231",     54231,   E_NONE   },
    { "nCr(5,2)",     10,   E_NONE   },
    { "テスト",        0.0, E_SYNTAX }
};

/** number() 関数テスト用データ */
static const struct test_data_dbl number_data [] = {
    { "54321",  54321,     E_NONE },
    { "543.21",   543.21,  E_NONE },
};

/**
 * 初期化処理
 *
 * @return なし
 */
void
cut_startup(void)
{
    (void)memset(&st_calc, 0, sizeof(testcalc));
    test_init_calc(&st_calc);
}

/**
 * 四則演算テスト
 *
 * @return なし
 */
void
test_answer_four(void)
{
    calcinfo calc; /* calcinfo構造体 */

    uint i;
    for (i = 0; i < NELEMS(four_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        exec_calc(&calc, four_data[i].expr);

        cut_assert_equal_string(four_data[i].answer,
                                (char *)calc.answer,
                                cut_message("%s=%s",
                                            four_data[i].expr,
                                            four_data[i].answer));
        destroy_answer(&calc);
        memfree((void **)&calc, NULL);
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
    calcinfo calc; /* calcinfo構造体 */

    uint i;
    for (i = 0; i < NELEMS(func_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        exec_calc(&calc, func_data[i].expr);

        cut_assert_equal_string(func_data[i].answer,
                                (char *)calc.answer,
                                cut_message("%s=%s",
                                            func_data[i].expr,
                                            func_data[i].answer));
        destroy_answer(&calc);
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
    calcinfo calc; /* calcinfo構造体 */

    uint i;
    for (i = 0; i < NELEMS(four_func_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        exec_calc(&calc, four_func_data[i].expr);

        cut_assert_equal_string(four_func_data[i].answer,
                                (char *)calc.answer,
                                cut_message("%s=%s",
                                            four_func_data[i].expr,
                                            four_func_data[i].answer));
        destroy_answer(&calc);
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
    calcinfo calc; /* calc情報構造体 */

    uint i;
    for (i = 0; i < NELEMS(error_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        exec_calc(&calc, error_data[i].expr);

        cut_assert_equal_string(error_data[i].answer,
                                (char *)calc.answer,
                                cut_message("%s=%s",
                                            error_data[i].expr,
                                            error_data[i].answer));
        destroy_answer(&calc);
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
    dbl x = 0.0, y = 0.0; /* 値 */
    calcinfo calc;        /* calc情報構造体 */

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "(235)");
    st_calc.readch(&calc);

    dbglog("ch=%c", calc.ch);
    parse_func_args(&calc, &x, NULL);
    cut_assert_equal_double(235, 0.0, x);

    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "(123,235)");
    st_calc.readch(&calc);

    dbglog("ch=%c", calc.ch);
    parse_func_args(&calc, &x, &y, NULL);
    cut_assert_equal_double(123, 0.0, x);
    cut_assert_equal_double(235, 0.0, y);
}

/**
 * set_digit() 関数テスト
 *
 * @return なし
 */
void
test_set_digit(void)
{
    calcinfo calc;                            /* calc情報構造体 */
    const char *expr = "sin(2)";              /* 式 */
    const char *expect = "0.909297426825682"; /* 期待する文字列 */

    set_digit(15L);
    (void)memset(&calc, 0, sizeof(calcinfo));
    exec_calc(&calc, expr);
    cut_assert_equal_string(expect, (char *)calc.answer,
                            cut_message("%s=%s",
                                        expect, calc.answer));
}

/**
 * readch() 関数テスト
 *
 * @return なし
 */
void
test_readch(void)
{
    uchar *ptr = NULL; /* ポインタ */
    calcinfo calc;     /* calc情報構造体 */

    /* スペース・タブを含んだ文字列を設定 */
    (void)memset(&calc, 0, sizeof(calcinfo));
    set_string(&calc, "te s  t");
    st_calc.readch(&calc);

    dbglog("ch=%c", calc.ch);
    cut_assert_equal_int('t', calc.ch,
                         cut_message("%c=%c", 't', calc.ch));
    st_calc.readch(&calc);
    cut_assert_equal_int('e', calc.ch,
                         cut_message("%c=%c", 'e', calc.ch));
    st_calc.readch(&calc);
    cut_assert_equal_int('s', calc.ch,
                         cut_message("%c=%c", 's', calc.ch));
    st_calc.readch(&calc);
    cut_assert_equal_int('t', calc.ch,
                         cut_message("%c=%c", 't', calc.ch));
    st_calc.readch(&calc);
    cut_assert_equal_int('\0', calc.ch,
                         cut_message("ptr=%p", calc.ptr));
    ptr = calc.ptr; /* アドレス保持 */
    st_calc.readch(&calc);
    cut_assert_equal_int('\0', calc.ch,
                         cut_message("ptr=%p", calc.ptr));
    /* アドレスが変わらないことを確認 */
    cut_assert_equal_memory(ptr, sizeof(ptr), calc.ptr, sizeof(calc.ptr));
}

/**
 * expression() 関数テスト
 *
 * @return なし
 */
void
test_expression(void)
{
    dbl result = 0.0; /* 結果 */
    calcinfo calc;    /* calcinfo構造体 */

    uint i;
    for (i = 0; i < NELEMS(expression_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, expression_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.expression(&calc);
        cut_assert_equal_double(expression_data[i].answer,
                                0.0,
                                result,
                                cut_message("%s=%.12g",
                                            expression_data[i].expr,
                                            expression_data[i].answer));
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
    dbl result = 0.0; /* 結果 */
    calcinfo calc;    /* calcinfo構造体 */

    uint i;
    for (i = 0; i < NELEMS(term_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, term_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.term(&calc);
        cut_assert_equal_double(term_data[i].answer,
                                0.0,
                                result,
                                cut_message("%s=%.12g",
                                            term_data[i].expr,
                                            term_data[i].answer));
        cut_assert_equal_int((int)term_data[i].errorcode,
                             (int)calc.errorcode,
                             cut_message("%s error",
                                         term_data[i].expr));
        clear_error(&calc);
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
    dbl result = 0.0; /* 結果 */
    calcinfo calc;    /* calcinfo構造体 */

    uint i;
    for (i = 0; i < NELEMS(factor_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, factor_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.factor(&calc);
        cut_assert_equal_double(factor_data[i].answer,
                                0.0,
                                result,
                                cut_message("%s=%.12g",
                                            factor_data[i].expr,
                                            factor_data[i].answer));
        cut_assert_equal_int((int)factor_data[i].errorcode,
                             (int)calc.errorcode,
                             cut_message("%s error",
                                         factor_data[i].expr));
        clear_error(&calc);
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
    dbl result = 0.0; /* 結果 */
    calcinfo calc;    /* calcinfo構造体 */

    uint i;
    for (i = 0; i < NELEMS(token_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, token_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.token(&calc);
        cut_assert_equal_double(token_data[i].answer,
                                0.0,
                                result,
                                cut_message("%s=%.12g",
                                            token_data[i].expr,
                                            token_data[i].answer));
        cut_assert_equal_int((int)token_data[i].errorcode,
                             (int)calc.errorcode,
                             cut_message("%s error",
                                         token_data[i].expr));
        clear_error(&calc);
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
    dbl result = 0.0; /* 結果 */
    calcinfo calc;    /* calcinfo構造体 */

    uint i;
    for (i = 0; i < NELEMS(number_data); i++) {
        (void)memset(&calc, 0, sizeof(calcinfo));
        set_string(&calc, number_data[i].expr);
        st_calc.readch(&calc);

        result = st_calc.number(&calc);
        cut_assert_equal_double(number_data[i].answer,
                                0.0,
                                result,
                                cut_message("%s=%.12g",
                                            number_data[i].expr,
                                            number_data[i].answer));
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
    cut_assert_equal_int(5, st_calc.get_strlen(50000, "%.18g"));
    cut_assert_equal_int(15, st_calc.get_strlen(123456789012345LL, "%.18g"));
    // 12345678.9000000004
    cut_assert_equal_int(19, st_calc.get_strlen(12345678.9, "%.18g"));
    // 1234567.89012344996
    cut_assert_equal_int(19, st_calc.get_strlen(1234567.89012345, "%.18g"));

    cut_assert_equal_int(5, st_calc.get_strlen(50000, "%.15g"));
    cut_assert_equal_int(15, st_calc.get_strlen(123456789012345LL, "%.15g"));
    // 12345678.9
    cut_assert_equal_int(10, st_calc.get_strlen(12345678.9, "%.15g"));
    // 1234567.89012345
    cut_assert_equal_int(16, st_calc.get_strlen(1234567.89012345, "%.15g"));

    // 5e+04
    cut_assert_equal_int(5, st_calc.get_strlen(50000, "%.1g"));
    // 1e+14
    cut_assert_equal_int(5, st_calc.get_strlen(123456789012345LL, "%.1g"));
    // 1e+07
    cut_assert_equal_int(5, st_calc.get_strlen(12345678.9, "%.1g"));
    // 1e+06
    cut_assert_equal_int(5, st_calc.get_strlen(1234567.89012345, "%.1g"));

    cut_assert_equal_int(5, st_calc.get_strlen(50000, "%.30g"));
    cut_assert_equal_int(15, st_calc.get_strlen(123456789012345LL, "%.30g"));
    // 12345678.9000000003725290298462
    cut_assert_equal_int(31, st_calc.get_strlen(12345678.9, "%.30g"));
    // 1234567.89012344996444880962372
    cut_assert_equal_int(31, st_calc.get_strlen(1234567.89012345, "%.30g"));
}

/**
 * 計算実行
 *
 * @param[in] calc calcinfo構造体
 * @param[in] str 文字列
 * @return calcinfo構造体
 */
static void
exec_calc(calcinfo *calc, const char *str)
{
    if (!create_answer(calc, (uchar *)str))
        cut_error("calc=%p", calc);
    dbglog("%p answer=%s", calc->answer, calc->answer);
}

