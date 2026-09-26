# 結合テスト項目書

`calcp` (スタンドアロン), `calcd` (サーバ), `calcc` (クライアント) を, 実際のプロセス・ソケット・端末・シグナルで動かして確認する項目.
単体テスト (`tests/`, `make test`) は, 関数やプログラムの一部を, モックに置き換えて確認しているので, ここでは, 置き換えずに, プログラム全体の動作を確認する.

## 1. 前提

### 1.1 対象

| プログラム | 内容 | 版 |
|---|---|---|
| `calcp` | 電卓 (標準入力から式を読んで, 答えを標準出力に出す) | 0.05 |
| `calcd` | 電卓サーバ (TCP で式を受け取り, 答えを返す. デーモンになる) | 0.05 |
| `calcc` | 電卓クライアント (標準入力の式を, サーバに送り, 答えを標準出力に出す) | 0.05 |
| `thcalcc` | 負荷確認用のクライアント (複数スレッドで, 同時に接続する. `tests/calcc/thread_client.c`) | 0.05 |

### 1.2 環境

- Linux (glibc 2.34 以降), GNU readline (`libreadline-dev`), CMake 3.10 以降, gcc
- 確認に使う道具: `pgrep` `pkill` `ss` (ポートの確認), `python3` (プロトコルの直接確認), `script` または端末 (readline の確認)
- ドキュメント生成の確認には, doxygen (と, グラフ用の graphviz)
- カバレッジの確認には, gcovr

### 1.3 準備

以降の手順の `$B` は, ビルドディレクトリ. リリースと同じ設定 (単体テストなし) でビルドする.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=OFF
cmake --build build
B=$PWD/build
```

負荷確認用の `thcalcc` は, `assert` で答えを確認するので, デバッグビルド (`NDEBUG` なし) でビルドする. (リリースビルドでは, `assert` が無効になり, 答えを確認しない)

```sh
cmake -S . -B build-it -DCMAKE_BUILD_TYPE=Debug
cmake --build build-it --target thcalcc     # build-it/tests/thcalcc
```

- サーバ (`calcd`) のテストには, 既定のポート (12345) と重ならないように, ポート **23456** を使う (`-p 23456`).
- `calcd` は, デーモンになるので, 確認後に `pkill -x calcd` で止める.
- 次の項目は, 前の項目の状態 (起動しているサーバなど) を前提にするものがある. 各節の先頭に書いた準備をしてから実施する.

### 1.4 結果欄

| 記号 | 意味 |
|---|---|
| ○ | 期待どおり |
| × | 期待と異なる (内容を, 備考に書く) |
| - | 実施しない (理由を書く) |

### 1.5 終了ステータス

`calcp` と `calcd` は, 正常な終了が 0, 異常が 1 (`EXIT_FAILURE`). `calcc` は, 次のとおり.

| 値 | 名前 | 意味 |
|---|---|---|
| 0 | `EX_SUCCESS` | 正常 |
| 1 | `EX_FAILURE` | 異常 (`atexit` の登録の失敗, `select` の失敗) |
| 2 | `EX_EMPTY` | 空の入力 (内部でだけ使う) |
| 3 | `EX_QUIT` | `quit` または `exit` が入力された (通常の終了) |
| 4 | `EX_CONNECT_ERR` | サーバに接続できない |
| 5 | `EX_ALLOC_ERR` | 標準入力の終わり (EOF), またはメモリ不足 |
| 6 | `EX_SEND_ERR` | 送信に失敗 |
| 7 | `EX_RECV_ERR` | 受信に失敗 (サーバが答えを返さずに閉じた, 上限を超えるデータ長など) |
| 8 | `EX_SIGNAL` | シグナルを受け取った |

## 2. `calcp` (スタンドアロン)

### 2.1 四則演算と括弧

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CP-A-01 | 乗算を先に計算する | `printf '%s\n' '1+2*3' \| $B/calcp` | `7` | |
| CP-A-02 | 括弧を先に計算する | `printf '%s\n' '(1+2)*3' \| $B/calcp` | `9` | |
| CP-A-03 | 除算は, 小数になる | `printf '%s\n' '10/4' \| $B/calcp` | `2.5` | |
| CP-A-04 | 負の結果 | `printf '%s\n' '7-10' \| $B/calcp` | `-3` | |
| CP-A-05 | 空白は, 無視する | `printf '%s\n' '1 + 2' \| $B/calcp` | `3` | |
| CP-A-06 | 既定の有効桁数は 12 | `printf '%s\n' '1/3' \| $B/calcp` | `0.333333333333` | |
| CP-A-07 | べき乗 | `printf '%s\n' '2^10' \| $B/calcp` | `1024` | |
| CP-A-08 | べき乗は, 乗算より先に計算する | `printf '%s\n' '2*3^2' \| $B/calcp` | `18` | |
| CP-A-09 | べき乗は, 除算より先に計算する | `printf '%s\n' '4/2^2' \| $B/calcp` | `1` | |
| CP-A-10 | 単項の符号は, べき乗より弱い | `printf '%s\n' '-2^2' \| $B/calcp` | `-4` | |
| CP-A-11 | べき乗は, 左から右に結合する ((2^3)^2) | `printf '%s\n' '2^3^2' \| $B/calcp` | `64` | |
| CP-A-12 | 指数の符号 | `printf '%s\n' '2^-1' \| $B/calcp` | `0.5` | |
| CP-A-13 | 括弧の前の符号 | `printf '%s\n' '-(1+2)' \| $B/calcp` | `-3` | |
| CP-A-14 | 演算子の後の符号つき括弧 | `printf '%s\n' '2*-(3)' \| $B/calcp` | `-6` | |
| CP-A-15 | 括弧の前の + | `printf '%s\n' '+(1+2)' \| $B/calcp` | `3` | |
| CP-A-16 | 小数 | `printf '%s\n' '1.5+2.25' \| $B/calcp` | `3.75` | |


### 2.2 関数

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CP-B-01 | 円周率 | `printf '%s\n' 'pi' \| $B/calcp` | `3.14159265359` | |
| CP-B-02 | ネイピア数 | `printf '%s\n' 'e' \| $B/calcp` | `2.71828182846` | |
| CP-B-03 | 絶対値 | `printf '%s\n' 'abs(-2)' \| $B/calcp` | `2` | |
| CP-B-04 | 平方根 | `printf '%s\n' 'sqrt(2)' \| $B/calcp` | `1.41421356237` | |
| CP-B-05 | 正弦 (ラジアン) | `printf '%s\n' 'sin(2)' \| $B/calcp` | `0.909297426826` | |
| CP-B-06 | 余弦 | `printf '%s\n' 'cos(2)' \| $B/calcp` | `-0.416146836547` | |
| CP-B-07 | 正接 | `printf '%s\n' 'tan(2)' \| $B/calcp` | `-2.18503986326` | |
| CP-B-08 | 逆正弦 | `printf '%s\n' 'asin(0.5)' \| $B/calcp` | `0.523598775598` | |
| CP-B-09 | 逆余弦 | `printf '%s\n' 'acos(0.5)' \| $B/calcp` | `1.0471975512` | |
| CP-B-10 | 逆正接 | `printf '%s\n' 'atan(0.5)' \| $B/calcp` | `0.463647609001` | |
| CP-B-11 | 指数関数 | `printf '%s\n' 'exp(2)' \| $B/calcp` | `7.38905609893` | |
| CP-B-12 | 自然対数 | `printf '%s\n' 'ln(2)' \| $B/calcp` | `0.69314718056` | |
| CP-B-13 | 常用対数 | `printf '%s\n' 'log(2)' \| $B/calcp` | `0.301029995664` | |
| CP-B-14 | ラジアンから度 | `printf '%s\n' 'deg(2)' \| $B/calcp` | `114.591559026` | |
| CP-B-15 | 度からラジアン | `printf '%s\n' 'rad(2)' \| $B/calcp` | `0.0349065850399` | |
| CP-B-16 | 階乗 | `printf '%s\n' 'n(10)' \| $B/calcp` | `3628800` | |
| CP-B-17 | 順列 | `printf '%s\n' 'nPr(5,2)' \| $B/calcp` | `20` | |
| CP-B-18 | 組み合わせ | `printf '%s\n' 'nCr(5,2)' \| $B/calcp` | `10` | |
| CP-B-19 | 関数の引数に式を書ける | `printf '%s\n' 'sqrt(1+3)' \| $B/calcp` | `2` | |
| CP-B-20 | 関数と四則演算の組み合わせ | `printf '%s\n' 'sin(0)+cos(0)' \| $B/calcp` | `1` | |


### 2.3 エラー

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CP-C-01 | ゼロ除算 | `printf '%s\n' '5/0' \| $B/calcp` | `Divide by zero.` | |
| CP-C-02 | 閉じ括弧がない | `printf '%s\n' 'sin(5' \| $B/calcp` | `Syntax error.` | |
| CP-C-03 | 引数が足りない | `printf '%s\n' 'nCr(5)' \| $B/calcp` | `Syntax error.` | |
| CP-C-04 | 式が途中で終わる | `printf '%s\n' '1+' \| $B/calcp` | `Syntax error.` | |
| CP-C-05 | 括弧が閉じない | `printf '%s\n' '(1+2' \| $B/calcp` | `Syntax error.` | |
| CP-C-06 | 余分な閉じ括弧 | `printf '%s\n' '1+2)' \| $B/calcp` | `Syntax error.` | |
| CP-C-07 | 演算子のあとに何もない | `printf '%s\n' '2*' \| $B/calcp` | `Syntax error.` | |
| CP-C-08 | 未定義の関数 | `printf '%s\n' 'nofunc(5)' \| $B/calcp` | `Function not defined.` | |
| CP-C-09 | 英字だけの式は, 関数として扱われる | `printf '%s\n' 'abc' \| $B/calcp` | `Function not defined.` | |
| CP-C-10 | 定義域外 (平方根) | `printf '%s\n' 'sqrt(-5)' \| $B/calcp` | `NaN.` | |
| CP-C-11 | 階乗の引数が自然数でない | `printf '%s\n' 'n(0.5)' \| $B/calcp` | `NaN.` | |
| CP-C-12 | 順列の引数の大小が逆 | `printf '%s\n' 'nPr(3,5)' \| $B/calcp` | `NaN.` | |
| CP-C-13 | オーバーフロー | `printf '%s\n' '10^1000000' \| $B/calcp` | `Infinity.` | |
| CP-C-14 | 171! は, 倍精度で表せない | `printf '%s\n' 'n(171)' \| $B/calcp` | `Infinity.` | |
| CP-C-15 | 巨大な引数でも, すぐに終わる (無限ループしない) | `printf '%s\n' 'n(99999999999999999999)' \| $B/calcp` | `Infinity.` | |
| CP-C-16 | アンダーフローは, エラーではなく 0 | `printf '%s\n' 'exp(-1000)' \| $B/calcp` | `0` | |


- エラーの後も, 次の式を計算できること (`printf '5/0\n1+1\n' | $B/calcp` は, `Divide by zero.` と `2` を出力する).
- 手順の `printf '%s\n' '<式>'` は, 式が `-` で始まるとき, `printf` が, 式をオプションと見なさないようにするための書き方.
- `n(171)` などの巨大な引数の手順には, `timeout 5` を付けて実施する (終わらなければ失敗).


### 2.4 オプション

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CP-D-01 | 有効桁数 (`-d`) | `printf '1/3\n' \\| $B/calcp -d 5` | `0.33333` | |
| CP-D-02 | 有効桁数の最大 | `printf '1/3\n' \\| $B/calcp -d 15` | `0.333333333333333` | |
| CP-D-03 | 有効桁数が範囲外 | `$B/calcp -d 0`, `-d 16`, `-d -1`, `-d abc` をそれぞれ実行 | `Digits is 1-15.` を標準エラー出力に出して, 終了ステータス 1 | |
| CP-D-04 | ロングオプション | `printf '1/3\n' \\| $B/calcp --digit=3` | `0.333` | |
| CP-D-05 | 処理時間 (`-t`) | `printf '1+2\n' \\| $B/calcp -t` | `time of calc_time: <数値>[msec]` の行と, `3` | |
| CP-D-06 | ヘルプ (`-h`) | `$B/calcp -h` | `Usage: calcp [OPTION]...` と, 4 つのオプションの説明を標準エラー出力に出して, 終了ステータス 0 | |
| CP-D-07 | バージョン (`-V`) | `$B/calcp -V` | `calcp version 0.05` を標準エラー出力に出して, 終了ステータス 0 | |
| CP-D-08 | 不正なオプション | `$B/calcp -x` | `invalid option -- 'x'` と ``Try `getopt --help' for more information`` を出して, 終了ステータス 1 | |
| CP-D-09 | オプション以外の引数 | `printf 'quit\n' \\| $B/calcp abc def` | `non-option ARGV-elements: abc def ` を出力する | |


### 2.5 入力の方法 (標準入力が端末でない場合)

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CP-E-01 | パイプ | `printf '1+1\n2+2\n3+3\n' \\| $B/calcp` | `2` `4` `6` を, 1 行ずつ出力する. 入力の終わりで, 終了ステータス 0 で終了する (CPU を使い続けない) | |
| CP-E-02 | ファイル | `printf '1+1\n2+2\n' > in.txt; $B/calcp < in.txt` | `2` `4`. 終了ステータス 0 | |
| CP-E-03 | 最後の行に改行がない | `printf '1+2' \\| $B/calcp` | `3` (最後の行も計算する) | |
| CP-E-04 | 何も入力しない | `$B/calcp < /dev/null; echo $?` | 何も出力せず, 終了ステータス 0 | |
| CP-E-05 | 空行 | `printf '\n\n1+1\n' \\| $B/calcp` | 空行は無視して, `2` だけを出力する | |
| CP-E-06 | quit | `printf '1+1\nquit\n2+2\n' \\| $B/calcp` | `2` だけを出力する (quit の後は, 実行しない). 終了ステータス 0 | |
| CP-E-07 | exit | `printf '1+1\nexit\n2+2\n' \\| $B/calcp` | `2` だけを出力する | |
| CP-E-08 | 長い式 | `python3 -c "print('1+'*2000+'1')" \\| $B/calcp` | `2001` | |


### 2.6 標準入力が端末の場合 (readline)

端末 (または `script -qc "$B/calcp" /dev/null`) で実施する.

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CP-F-01 | 計算 | `1+1` を入力して, Enter | 次の行に `2` を表示する | |
| CP-F-02 | 履歴 | `1+1` を Enter で実行した後に, 上矢印, Enter | `1+1` が呼び出されて, 再び `2` を表示する | |
| CP-F-03 | 行の編集 | `1+1` を入力してから, Backspace で `1` を消して `2` を入力し, Enter | `1+2` として, `3` を表示する | |
| CP-F-04 | Ctrl-D | 空の行で, Ctrl-D | 終了する (終了ステータス 0) | |
| CP-F-05 | quit | `quit` を入力して Enter | 終了する (終了ステータス 0) | |
| CP-F-06 | Ctrl-C | 入力待ちのときに, Ctrl-C (SIGINT) | 入力中の文字を捨てて, 終了する (終了ステータス 0) | |
| CP-F-07 | SIGTERM | 入力待ちのときに, 別の端末から `pkill -TERM -x calcp` | 終了する (終了ステータス 0) | |
| CP-F-08 | 履歴の上限 | 1 から 150 までの式を, 続けて入力する | エラーにならず, 計算を続けられる (履歴は, 古いものから消える. 上限は 100) | |


## 3. `calcd` (サーバ)

この節の各項目の前後に, `pkill -x calcd` で, 起動しているサーバを止める.

### 3.1 オプション

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CD-A-01 | ヘルプ (`-h`) | `$B/calcd -h` | `Usage: calcd [OPTION]...` と, `-p` `-d` `-g` `-h` `-V` の説明 (既定のポートは `12345`). 終了ステータス 0 | |
| CD-A-02 | バージョン (`-V`) | `$B/calcd -V` | `calcd version 0.05`. 終了ステータス 0 | |
| CD-A-03 | 不正なオプション | `$B/calcd -x` | `invalid option -- 'x'`. 終了ステータス 1 | |
| CD-A-04 | ポート番号が長すぎる | `$B/calcd -p 1234567; echo $?` | `Portno string length 5` を出力して, 終了ステータス 1 | |
| CD-A-05 | ポート番号が範囲外 | `$B/calcd -p 0`, `-p 65536`, `-p -1`, `-p abc` | 起動せず (プロセスが残らない), 終了ステータス 1 | |
| CD-A-06 | ポート番号の最大 | `$B/calcd -p 65535; ss -ltn \\| grep 65535` | 65535 番を LISTEN する (`pkill -x calcd` で止める) | |
| CD-A-07 | サービス名 | `$B/calcd -p ftp` (ポート 21 は, 権限がなければ, 失敗する) | 権限があれば 21 番を LISTEN し, なければ, 起動せずに終了ステータス 1 | |


### 3.2 起動と停止

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CD-B-01 | デーモンとして起動 | `$B/calcd -p 23456; echo $?; pgrep -a calcd` | コマンドはすぐに戻り (終了ステータス 0), `calcd -p 23456` のプロセスが 1 つ残る | |
| CD-B-02 | ポートを待ち受ける | `ss -ltn \\| grep 23456` | `0.0.0.0:23456` が LISTEN | |
| CD-B-03 | 同じポートで, 二重に起動 | CD-B-01 の後に, もう一度 `$B/calcd -p 23456; echo $?` | `Address already in use` を出力して, 終了ステータス 1. 最初のサーバは, 動き続ける | |
| CD-B-04 | SIGTERM で停止 | `pkill -TERM -x calcd; sleep 3; pgrep -c calcd; ss -ltn \\| grep -c 23456` | プロセスが無くなり (`0`), ポートも閉じる (`0`). 停止には, 最大 1 秒ほどかかる | |
| CD-B-05 | SIGINT で停止 | 起動して, `pkill -INT -x calcd` | CD-B-04 と同じ | |
| CD-B-06 | SIGQUIT で停止 | 起動して, `pkill -QUIT -x calcd` | CD-B-04 と同じ | |
| CD-B-07 | 停止したポートで, 再起動 | CD-B-04 の後 (プロセスが無くなってから) に, `$B/calcd -p 23456` | すぐに起動できる (`SO_REUSEADDR` があるので, 待たずに使える) | |
| CD-B-08 | SIGHUP で再起動 | 起動して, `pgrep -x calcd` の PID を控える. `kill -HUP <PID>; sleep 1; pgrep -a calcd` | サーバは動き続け (デーモンになり直すので, PID は変わる), 続けて, 計算を依頼できる (4.1) | |
| CD-B-09 | 相対パスで起動して, SIGHUP で再起動 | `cd $B; ./calcd -p 23456`. 続けて `kill -HUP $(pgrep -x calcd)` | CD-B-08 と同じ (相対パスでも, 再起動できる) | |
| CD-B-10 | SIGPIPE で停止しない | クライアントが, 答えを受け取る前に, 切断する (CD-D-05) | サーバは, 動き続ける | |


## 4. `calcc` (クライアント)

### 4.1 オプションと接続

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CC-A-01 | ヘルプ (`-h`) | `$B/calcc -h` | `Usage: calcc [OPTION]...` と, `-i` `-p` `-g` `-t` `-h` `-V` の説明 (既定は, `127.0.0.1` とポート `12345`). 終了ステータス 0 | |
| CC-A-02 | バージョン (`-V`) | `$B/calcc -V` | `calcc version 0.05`. 終了ステータス 0 | |
| CC-A-03 | 不正なオプション | `$B/calcc -x` | `invalid option -- 'x'`. 終了ステータス 1 | |
| CC-A-04 | サーバが無い | `$B/calcc -p 23457 < /dev/null; echo $?` | `Connect error` を出力して, 終了ステータス 4 | |
| CC-A-05 | ホスト名を解決できない | `$B/calcc -i no-such-host.invalid -p 23456 < /dev/null` | `Connect error`. 終了ステータス 4 | |
| CC-A-06 | ポートのサービス名を解決できない | `$B/calcc -p nosvc < /dev/null` | `Connect error`. 終了ステータス 4 | |
| CC-A-07 | ポート番号が長すぎる | `$B/calcc -p 1234567` | `Portno string length 5` を出力して, 終了ステータス 1 | |
| CC-A-08 | ホスト名が長すぎる | `$B/calcc -i <48 文字以上のホスト名>` | `Hostname string length 47` を出力して, 終了ステータス 1 | |


## 5. `calcc` と `calcd` の組み合わせ

この節の前に, サーバを起動する (`$B/calcd -p 23456`). 以降の `c` は, `$B/calcc -p 23456` を表す.

### 5.1 計算

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CS-A-01 | 四則演算 | `printf '1+2*3\nquit\n' \\| c` | `7`. 終了ステータス 3 | |
| CS-A-02 | 複数の式 (パイプで, 続けて入力) | `printf '1+2*3\n5/0\nsqrt(-1)\nnofunc(1)\n\n(1+2)*3\nquit\n' \\| c` | `7` `Divide by zero.` `NaN.` `Function not defined.` `9` を, 入力の順に出力する (空行は無視). 全ての答えを出力してから, 終了ステータス 3 で終了する | |
| CS-A-03 | 関数 | `printf 'sqrt(2)\nnPr(5,2)\nquit\n' \\| c` | `1.41421356237` `20` | |
| CS-A-04 | 有効桁数 | サーバを止めてから, `calcd -p 23456 -d 5` で起動して, `printf '1/3\nquit\n' \\| c` | `0.33333` | |
| CS-A-05 | ホスト名で接続 | `printf '2+2\nquit\n' \\| $B/calcc -i localhost -p 23456` | `4` | |
| CS-A-06 | quit で終了 | `printf '1+1\nquit\n2+2\n' \\| c; echo $?` | `2` だけを出力して, 終了ステータス 3 | |
| CS-A-07 | exit で終了 | `printf '1+1\nexit\n2+2\n' \\| c; echo $?` | `2` だけを出力して, 終了ステータス 3 | |
| CS-A-08 | 入力の終わりで終了 | `printf '1+1\n2+2' \\| c; echo $?` | `2` `4` (改行のない最後の行も送る) を出力して, 終了ステータス 5 | |
| CS-A-09 | 処理時間 (`-t`) | `printf '1+1\nquit\n' \\| c -t` | `time of client_time: <数値>[msec]` の行と, `2` | |
| CS-A-10 | デバッグ (`-g`) | `printf '1+1\nquit\n' \\| c -g` | 標準出力は, `2` だけ (送受信のダンプは, syslog に出る) | |
| CS-A-11 | 対話的な入力 | 端末で `c` を起動し, `7*6` を入力して, 待ってから `quit` | `7*6` を入力した直後に, `42` を表示する (サーバの答えが, すぐに出る). `quit` で終了する | |
| CS-A-12 | 入力が止まっているとき | `( printf '7*6\n'; sleep 2; printf 'quit\n' ) \\| c` の出力に, 時刻を付けて確認する | `42` が, `sleep` の間 (quit を入力する 2 秒前) に出力される | |


### 5.2 複数のクライアント

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CS-B-01 | 2 つのクライアントが同時に接続 | 別々の端末 (または `&`) で, `( printf '1+1\n'; sleep 1; printf '2+2\nquit\n' ) \\| c` と, `( printf '3+3\n'; sleep 1; printf '4+4\nquit\n' ) \\| c` を同時に実行 | それぞれが, 自分の式の答え (`2` `4` と `6` `8`) だけを受け取る | |
| CS-B-02 | 50 のクライアントが同時に接続 | `for i in $(seq 1 50); do ( printf "$i+$i\nquit\n" \\| c > out$i.txt ) & done; wait` | 各 `out$i.txt` に, `$i+$i` の答えが出力されている (`2` から `100` まで, 全て) | |
| CS-B-03 | 負荷 (`thcalcc`) | `build-it/tests/thcalcc -p 23456 -t 10` を, `-t 100`, `-t 500`, `-t 1000` でも実行 | いずれも, 終了ステータス 0 (各スレッドが, 自分の式の答えを受信して, `assert` で確認する. 答えが一致しなければ, 異常終了する). デバッグビルドなので, 詳細なログが, 標準エラー出力に出る | |
| CS-B-04 | 切断したクライアント | クライアントが, 答えを受け取る前に, 強制終了する (`kill -9`) | サーバは, 動き続け, 新しいクライアントの計算に, 答えられる | |


### 5.3 異常な通信

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| CS-C-01 | サーバが, 途中で停止 | `( printf '1+1\n'; sleep 2; printf '2+2\nquit\n' ) \\| c` を実行中に, 1 秒後に `pkill -x calcd` | クライアントは, シグナルで終了せず, 終了する (サーバが答えを返せなかった場合は, 終了ステータス 7) | |
| CS-C-02 | サーバが無い状態で送信 | サーバを止めた後に, 接続済みのクライアントで, 式を入力する | `SIGPIPE` で終了せず, 送信または受信のエラーとして処理して, 終了ステータス 6 または 7 (実測は 7). サーバが動いていれば, 新しいクライアントは, 計算できる | |
| CS-C-03 | 上限を超える長さの式 (70000 文字) | `python3 -c "print('1+'*35000+'1')" \\| c; echo $?` | サーバは, 巨大なメモリを確保せずに, 接続を閉じる. クライアントは, 答えを出力せずに, 終了ステータス 7 | |
| CS-C-04 | 上限内の長い式 (60000 文字) | `python3 -c "print('1+'*30000+'1')" \\| c; echo $?` | `30001` を出力して, 終了ステータス 5 (入力の終わり) | |
| CS-C-05 | サーバに, 巨大なデータ長のヘッダを直接送る | `python3` で, ヘッダ (データ長 `0xFFFFFFF0`, 4 バイトのパディング) だけを送信して, 応答を待つ | サーバは, 接続を閉じる (受信は, 空). サーバは, 動き続け, 次の計算に答える | |
| CS-C-06 | 終端の NUL がない式を直接送る | `python3` で, データ長 8, データ `1+1+1+1+` (NUL なし) を送る | 答え `4` を返す (最後の 1 バイトを NUL として扱う). サーバは, 異常終了しない | |
| CS-C-07 | 途中で切れたデータ | `python3` で, ヘッダだけ送って, 切断する | サーバは, 動き続ける (次の計算に答える) | |


- `CS-C-05` `CS-C-06` `CS-C-07` の `python3` は, `struct.pack('!I', length) + b'\0'*4 + data` の形式 (ヘッダは, ネットワークバイトオーダのデータ長 4 バイトと, パディング 4 バイト) で送る.


## 6. ビルド, テスト, 配布

| No | 項目 | 手順 | 期待結果 | 結果 |
|---|---|---|---|---|
| BD-A-01 | CMake でビルド (リリース) | `cmake -S . -B build -DBUILD_TESTS=OFF && cmake --build build` | 警告なしにビルドでき, `build/` に `calcp` `calcd` `calcc` と, `lib*.so` `lib*.a` ができる. 単体テストは, ビルドされない | |
| BD-A-02 | CMake でビルド (デバッグ) | `cmake -S . -B build-debug -DCMAKE_BUILD_TYPE=Debug && cmake --build build-debug` | 警告なしにビルドできる | |
| BD-A-03 | GNU make でビルド | `make clean all` | 警告なしにビルドでき, `Success!! Congratulations!!` が出る. `calc/calcp` `server/calcd` `client/calcc` ができる | |
| BD-A-04 | GNU make でビルド (デバッグ) | `make clean debug` | 警告なしにビルドできる | |
| BD-A-05 | GNU make で静的リンク | `make clean static` | ビルドでき, `ldd calc/calcp` に, `libcalcutil` `libcalcp` が出ない | |
| BD-A-06 | 単体テスト | `cd build && make test` (`BUILD_TESTS=ON` の設定で) | 単体テストをビルドしてから実行し, 全て成功 (20 テスト). `make` (引数なし) では, ビルドされない | |
| BD-A-07 | リポジトリ直下の `make test` | `make test` | Debug ビルドを `build-test/` に作って, 全て成功 | |
| BD-A-08 | カバレッジ | `make coverage` (gcovr が必要) | 行 100%, 分岐 95% 以上を表示し, `build-coverage/coverage/index.html` ができる | |
| BD-A-09 | ドキュメント生成 | `cmake --build build --target doc` (doxygen が必要) | 警告なしに, `build/doc/html/index.html` ができる. 先頭のページに `README.md` の内容が出る | |
| BD-A-10 | doxygen が無い場合 | doxygen が無い環境で, `make doc` | `doc は使えません: doxygen をインストールしてください` を出力して失敗する | |
| BD-A-11 | インストール | `cmake --install build --prefix /tmp/inst` | `/tmp/inst/bin` に `calcp` `calcd` `calcc`, `/tmp/inst/lib` に `lib*.so` `lib*.a` が入る. 実行するには, `LD_LIBRARY_PATH=/tmp/inst/lib` が必要 (標準の `/usr/local` では `ldconfig` を実行する) | |
| BD-A-12 | クリーン | `make clean` | ビルドで作ったファイル (`*.o` `*.a` `*.so` と, 実行ファイル, `doc`, `build`, `build-test`, `build-coverage`) が消える | |


## 7. 実施記録

| 実施日 | 実施者 | 版 (コミット) | 環境 | 結果 (○ / × の数) | 備考 |
|---|---|---|---|---|---|
| | | | | | |

## 8. 結合テストで見つかった不具合

この項目書を作るために実際に動かして, 見つかった不具合は, 修正済み (コミット履歴を参照).

- `calcc` は, パイプやファイルから続けて入力すると, 答えを受信する前に `quit` や入力の終わりになり, 答えを出力しなかった.
  (`FD_ISSET` の修正で, 送信と受信が, 独立になったため. 終了する前に, 未受信の答えを受信するようにした)
