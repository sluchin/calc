# プロジェクト設定

C 言語 (gcc / GNU make) の電卓プログラム。スタンドアロン版 (`calcp`) と、クライアントサーバ版 (`calcd` / `calcc`) がある。

## コマンド

- ビルド: `make clean all` (リリース) または `make clean debug` (デバッグ。`-D_DEBUG -DUNITTEST` を付け、`dbglog` などのデバッグログを有効にする)
  - 最上位の `GNUmakefile` が `lib` → `calc` → `server` → `client` の順に各ディレクトリの `Makefile` を呼ぶ。`lib` を先にビルドしないと、他が `libcalcutil` をリンクできない。
  - `make static`: 静的リンク版。`make strip`: strip。`make install`: `/usr/local` へインストール。
  - GNU readline (`libreadline-dev`) が必要。
- CMake でのビルド: `cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug && cmake --build build` (`Release` も可。`make cmake-build` でも実行できる)
  - 最上位の `CMakeLists.txt` 1 つで全体を扱う。Debug は `-DUNITTEST -D_DEBUG`、それ以外は `-DNDEBUG` を付ける。
  - 最適化は `Makefile` に合わせて `-g -O2` にしている。
  - cutter が見つかれば単体テストも作られ、`ctest --test-dir build --output-on-failure` で実行できる。
  - 生成物は `build/` に出る (`.gitignore` 済み)。ソースファイルを追加したら、`CMakeLists.txt` も更新する。
- 警告: `-Wall` で警告を出さないこと。ビルド出力に警告が出たら修正する。
- テスト: `make clean debug && make test`
  - 単体テストは cutter (`/usr/bin/cutter`) で実行する。cutter が無い環境ではテストを実行できない。その場合は、実行できなかったことを報告する (成功したとは書かない)。
  - 一部だけ実行するときは、対象ディレクトリで実行する (例: `cd lib && make test`)。
- ドキュメント: `make doc` (doxygen, graphviz, mscgen が必要)

## プロジェクト構成

- `lib/`: 共通ライブラリ `libcalcutil` (ログ `log`、ネットワーク `net`、送受信データ `data`、ファイル `fileio`、端末 `term`、`readline`、`memfree`、`timer.h`)。他のディレクトリから使われる。`lib` は他に依存しない。
- `calc/`: 計算処理 `libcalcp` (`calc.c`、`func.c`、`error.c`) と、スタンドアロン版の `calcp` (`main.c`、`option.c`)。
- `server/`: サーバデーモン `calcd`。
- `client/`: クライアント `calcc`。
- 各ディレクトリの `tests/` に、cutter の単体テストを置く。
- `def.h` の `EX_OK` / `EX_NG` を、関数の戻り値に使う。
- 生成物 (`*.o` `*.a` `*.so`、`calcp` `calcc` `calcd`) は `.gitignore` 済み。コミットしない。

## コーディング規約

`README` の「コーディングのコンセプト」に従う。

1. 戻り値を返す関数は、必ず戻り値をチェックして、ログを出力する。
2. 戻り値を返す標準関数で、あえて戻り値をチェックしない場合は、`void` でキャストする (`(void)memset(...)`)。
3. 見映えを揃えるために、決まった場所に必ずコメントを入れる。
4. エラーハンドラでは、`goto` 文を積極的に使用する。
5. メモリ解放後は、`NULL` を代入する。
6. ソケットのクローズ後は、`-1` を代入する。

既存のコードに合わせる。

- **バッファの安全性**: `strcpy` `sprintf` `strcat` などの、長さを見ない関数は使わない。`snprintf` や、長さを指定する関数を使い、サイズや境界を必ず確認する。
- **ネットワーク**: 送受信の長さは、ネットワークバイトオーダー (`htonl` / `ntohl`) で扱う。`send` / `recv` は、部分的な送受信と `EINTR` を考慮する。
- **ログ**: `lib/log.h` のマクロを使う。エラーは `outlog`、デバッグ用は `dbglog` / `dbgdump` (`_DEBUG` のときだけ有効)、標準エラー出力は `outstd` / `stdlog`。`printf` でログを出さない。
- **ファイル先頭のコメント**: Doxygen 形式の `@file` `@brief` `@author` `@date` `@version` と、Copyright、GPL のライセンス表記を付ける。新規ファイルも、既存ファイルに合わせる。
- **関数のコメント**: 全ての関数に、Doxygen 形式で `@param[in/out]` `@return` (必要なら `@retval`) を書く。
- **書式**: インデントは空白 4 つ。関数の戻り型は、関数名と別の行に書く。中括弧は、関数定義では次の行、制御構文では同じ行に置く。ローカル変数は、宣言時に初期化して、行末にコメントを付ける。
- **ヘッダ**: インクルードガード (`_DEF_H_` の形式) を付ける。標準ヘッダには、使う関数をコメントで添える (`#include <string.h> /* memset memcpy */`)。
- **移植性**: 古い環境 (gcc 4.6 など) でもビルドできる、標準的な C (GNU 拡張は既存のものだけ) で書く。

## テストの規約

- テストは cutter で書き、`<ディレクトリ>/tests/test_<ソース名>.c` に置く (`lib/data.c` → `lib/tests/test_data.c`)。
- 関数名は `test_<テスト対象の関数名>` (`set_client_data` → `test_set_client_data`)。`cut_startup` / `cut_shutdown` で、初期化と後始末を行う。
- テスト関数には、プロトタイプ宣言と Doxygen コメントを付ける (既存のテストに合わせる)。
- テストを追加したら、`tests/Makefile` のターゲットにも追加する。
- テストから、外部のネットワークや、実際の環境に影響する処理に接続しない。

## 開発ルール

- コードの修正後は、必ず `make clean debug` (警告が出ないこと) と、cutter が使える環境では `make test` を実行し、エラーが出なくなるまで修正を繰り返す。
  - **例外**: 変更が `CLAUDE.md` や `README` などの文書のみの場合は、ビルドとテストを実行しない。
- 機能追加・修正には、単体テストを追加する。
- オプションやコマンドを変更したら、`README` も更新する。
- ソースファイルを追加したら、該当ディレクトリの `Makefile` (`OBJECTS` / `OBJCALC` と、依存関係) と `CMakeLists.txt` も更新する。

### 完了条件・タスク完了時の動作

- 修正や機能追加が完了し、ビルドやテストが全て成功したら、以下の手順を実施してタスクを終了すること。
  1. `git diff --stat` を実行して、必要に応じて主要ファイルの差分を確認する。
  2. 修正内容のサマリー (変更点と理由の簡潔なまとめ) を、ユーザーに報告する。
- ユーザーに依頼されない限り、`git commit` / `git push` は行わない。
