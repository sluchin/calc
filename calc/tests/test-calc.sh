#!/bin/sh
# $Id$

# パス
top_dir=..
result=$top_dir/tests/test_result
program=$top_dir/calcp

# 終了コード
E_SUCCESS=0
E_FAILURE=1

# コマンド
RM='rm -rf'

# プログラムの存在チェック
if [ ! -x $program ]; then
    echo "$program not found"
    exit $E_FAILURE
fi

# ファイル削除
if [ -f $result ]; then
    $RM $result
fi

print_result()
{
    input=$1
    echo -n "$input ---> " >> $result
    echo "$input\nexit" | $program >> $result
}

# 四則演算のテスト
print_result "(105+312)+2*(5-3)"
print_result "(105+312)+2/(5-3)"
print_result "1+2*(5-3)"
print_result "1+2/(5-3)"

# 関数のテスト
print_result "pi"
print_result "e"
print_result "abs(-2)"
print_result "sqrt(2)"
print_result "sin(2)"
print_result "cos(2)"
print_result "tan(2)"
print_result "asin(0.5)"
print_result "acos(0.5)"
print_result "atan(0.5)"
print_result "exp(2)"
print_result "ln(2)"
print_result "log(2)"
print_result "deg(2)"
print_result "rad(2)"
print_result "n(10)"
print_result "nPr(5,2)"
print_result "nCr(5,2)"

# 四則演算と関数の組み合わせ
print_result "5*pi"
print_result "pi*5"
print_result "5*e"
print_result "e*5"
print_result "5*abs(-2)"
print_result "abs(-2)*5"
print_result "5*sqrt(2)"
print_result "sqrt(2)*5"
print_result "5*sin(2)"
print_result "sin(2)*5"
print_result "5*cos(2)"
print_result "cos(2)*5"
print_result "5*tan(2)"
print_result "tan(2)*5"
print_result "2*asin(0.5)"
print_result "asin(0.5)*2"
print_result "2*acos(0.5)"
print_result "acos(0.5)*2"
print_result "5*atan(0.5)"
print_result "atan(0.5)*5"
print_result "5*exp(2)"
print_result "exp(2)*5"
print_result "5*ln(2)"
print_result "ln(2)*5"
print_result "5*log(2)"
print_result "log(2)*5"
print_result "5*deg(2)"
print_result "deg(2)*5"
print_result "5*rad(2)"
print_result "rad(2)*5"
print_result "5*n(10)"
print_result "n(10)*5"
print_result "5*nPr(5,2)"
print_result "nPr(5,2)*5"
print_result "5*nCr(5,2)"
print_result "nCr(50,22)*5"

exit $E_SUCCESS

