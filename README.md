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

ライブラリは, 静的ライブラリ (`.a`) と動的ライブラリ (`.so`) の, どちらか一方だけを作る. 既定は, 静的ライブラリで, 実行ファイルは, 静的ライブラリにリンクする (単独で動く).
動的ライブラリにするには, `DYNAMIC` を指定する.

Make の場合:

```sh
make clean all                # リリース (静的ライブラリ)
make clean debug              # デバッグ
make clean all DYNAMIC=1      # 動的ライブラリ
```

(GNU make の `-D` は, 変数の定義ではないので, `DYNAMIC=1` と書く.)
静的と動的を切り替えるときは, `make clean` してから, ビルドし直す.

CMake の場合:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release                  # 静的ライブラリ
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DDYNAMIC=ON     # 動的ライブラリ
cmake --build build
```

## インストール

```sh
sudo make install                # 静的ライブラリ (既定)
sudo make install DYNAMIC=1      # 動的ライブラリ
```

- 静的ライブラリ: 実行ファイルは, 単独で動く. `ldconfig` は不要.
- 動的ライブラリ: 環境によって, ライブラリの検索パスが異なる. インストール後に, `sudo ldconfig` を実行する.
  (`/usr/local/lib` が検索パスに含まれない場合は, `/etc/ld.so.conf.d/` にファイルを作って追加する.)
- どちらも, `make install` の最後に, 上の内容がメッセージとして表示される.
- CMake では, `cmake -S . -B build -DDYNAMIC=ON` で構成して, `cmake --install build` する.

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
cmake -S . -B build
cd build
make          # 単体テストは, ビルドされない
make test     # 単体テストをビルドして, 実行する (ctest)
```

`make test` は, 単体テストのビルドも, ctest のテスト (`build_tests`) として実行する. ビルドの出力と, 各テストの出力を, 全て表示する (`ctest --verbose`).
`ctest --output-on-failure` でも, 同じように, ビルドしてから実行する.
単体テストは, 内部関数を公開する `UNITTEST` を有効にしてビルドする. 配布用のビルドでは, `-DBUILD_TESTS=OFF` を指定する.

トップレベルの `make test` (GNU make) は, Debug ビルドを `build-test/` に作って, ctest を実行する.

テストは `tests/` 配下にある. テストランナーは [greatest](https://github.com/silentbicycle/greatest), モックは [FFF](https://github.com/meekrosoft/fff) (`fff.h`) を使用する.
どちらもヘッダのみで, `tests/third_party/` に含まれるので, インストールは不要.

## 結合テスト

`calcp` `calcd` `calcc` を, 実際に動かして確認する項目は, `INTEGRATION_TESTS.md` にある.

## カバレッジ

gcovr をインストールし (`sudo apt install gcovr`), `make coverage` コマンドを実行する.
レポートは `build-coverage/coverage/index.html` に出力される.

CMake のビルドディレクトリでも, `make coverage` で実行できる. カバレッジ用に, `build/coverage-build/` に別にビルドして, レポートを `build/coverage-build/coverage/index.html` に出力する.

```sh
cd build
make coverage
```

`-DENABLE_COVERAGE=ON` で構成したディレクトリでは, そのディレクトリの中でテストを実行して, `build/coverage/index.html` に出力する.

## ドキュメント生成

doxygen と graphviz をインストールし (`sudo apt install doxygen graphviz`), `make doc` コマンドを実行する.

```sh
cd build
make doc     # build/docs/html/index.html
```

リポジトリ直下で `make doc` (GNU make) を実行すると, `docs/html/index.html` に出力される.
graphviz (dot) がなくても生成できる (グラフは出力されない).

## コーディングのコンセプト

1. 戻り値を返す関数は必ず戻り値チェックしてログを出力する.
2. 戻り値を返す標準関数であえて戻り値をチェックしない場合は, `void` でキャストする.
3. 見映えを揃えるために決まった場所に必ずコメントを入れる.
4. エラーハンドラでは, `goto` 文を積極的に使用する.
5. メモリ解放後, `NULL` を代入する.
6. ソケットクローズ後, `-1` を代入する.

MISRA-C に対する適合方針および例外事項 (Deviations) については, [MISRA.md](MISRA.md) を参照.
