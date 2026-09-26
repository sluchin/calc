# TODO

## 未対応

- [ ] GNU readline から editline を使用するよう変更する. (2011-09-21)
- [ ] 多倍長整数に対応する. GNU gmp, GNU mpfr. (2011-09-21)
- [ ] `calcp` が, 標準入力が端末でないとき (パイプやファイル), 入力の終わり (EOF) で終了せず, CPU を 100% 使い続ける.
  readline 8.3 と `rl_event_hook` (`calc/main.c` の `check_state`) の組み合わせが原因と思われる. `quit` / `exit` では正常に終了する.
  `tests/calcp/test_main.c` は, この不具合を避けるため, 入力を必ず `quit` で終わらせている.
- [ ] `calc/func.c:153` で, `-Wswitch-unreachable` の警告が出る.
- [ ] テストのカバレッジを上げる. (`make coverage` で確認できる)
  - 低いもの: `lib/net.c` (76%), `client/client.c` (75%), `server/server.c` (74%), `client/main.c` (55%), `server/main.c` (63%), `calc/main.c` (65%).
  - FFF で失敗する場合 (`malloc` `socket` `send` など) をモックにして, エラー処理を通す.
- [ ] 標準出力をリダイレクトしている間にテストが失敗すると, メッセージが見えなくなる. (`tests/calcc/test_client.c` など)

## 対応済み

- [x] cutter から greatest と FFF に移行し, テストを `tests/` 配下に移した.
- [x] `main.c` と `option.c` のテストを追加した.
- [x] CMake でビルドできるようにした.
- [x] `lib/timer.h` の関数を `static inline` にした.
