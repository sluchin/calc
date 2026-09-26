# calc

C 言語で書いた電卓プログラム. スタンドアロン版 (`calcp`) と, クライアントサーバ版 (`calcd` / `calcc`) がある.

## 開発環境

- Ubuntu 11.10
- Linux 3.0.0-15-generic-pae #26-Ubuntu SMP
- gcc バージョン 4.6.1 (Ubuntu/Linaro 4.6.1-9ubuntu3)
- GNU Make 3.81
- GNU gdb (Ubuntu/Linaro 7.3-0ubuntu2) 7.3-2011.08

## 動作確認環境

- Fedora 16 i686 3.1.0-7.fc16.i686 #1 SMP
- Fedora 16 x86_64 3.1.0-7.fc16.x86_64 #1 SMP
- CentOS 6.2 i386 2.6.32-220.el6.i686 #1 SMP
- Debian GNU/Linux 6.0.3 i386 2.6.32-5-686-bigmem

## GNU readline のインストール

Ubuntu の場合:

```sh
sudo apt-get install libreadline6-dev
```

Fedora の場合:

```sh
sudo yum install readline-devel
```

## ビルド

Make の場合:

```sh
make clean all      # リリース
make clean debug    # デバッグ
```

CMake の場合:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

## インストール

```sh
sudo make install
```

## 使い方

### クライアントサーバ

サーバデーモンを起動する.

```sh
cd server
./calcd
```

クライアントプログラムを起動する.

```sh
cd client
./calcc
```

### スタンドアロン

```sh
cd calc
./calcp
```

## テスト

CMake をインストールし, `make test` コマンドを実行する.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build --output-on-failure
```

テストは `tests/` 配下にある. テストランナーは [greatest](https://github.com/silentbicycle/greatest), モックは [FFF](https://github.com/meekrosoft/fff) (`fff.h`) を使用する.
どちらもヘッダのみで, `tests/third_party/` に含まれるので, インストールは不要.

## カバレッジ

gcovr をインストールし, `make coverage` コマンドを実行する.
レポートは `build-coverage/coverage/index.html` に出力される.

## ドキュメント生成

doxygen, graphviz, mscgen をインストールし, `make doc` コマンドを実行する.

## コーディングのコンセプト

1. 戻り値を返す関数は必ず戻り値チェックしてログを出力する.
2. 戻り値を返す標準関数であえて戻り値をチェックしない場合は, `void` でキャストする.
3. 見映えを揃えるために決まった場所に必ずコメントを入れる.
4. エラーハンドラでは, `goto` 文を積極的に使用する.
5. メモリ解放後, `NULL` を代入する.
6. ソケットクローズ後, `-1` を代入する.
