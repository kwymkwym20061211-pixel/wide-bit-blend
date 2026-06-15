# CLAUDE.md — wide-bit-blend

このドキュメントは必要とあらば更新してください。次のClaudeが引き継ぎやすいことを目標にすること。

## プロジェクト概要

256bit固定長の**ビットミキサー**を開発するプロジェクト。xxhashのような汎用ハッシュではなく、「256bitブロックを均等に攪拌する」ことだけに特化したニッチなライブラリを目指している。

ミキサーの定義: `void mixer(mt_block_t *block)` — 入力ブロックをインプレースで変換する。

---

## ファイル構成

```
src/
  main.c                        # テスト実行エントリ。ここでミキサーを登録する
  include/
    mixer_test.h                # シングルヘッダ製テストスイート（実装含む）
    murmur_based_2026_05_13.h   # 第3世代（旧版）
    murmur_based_2026_05_14.h   # 第4世代（旧版・PractRand FAIL）
    murmur_based_2026_06_15.h   # 第5世代 ← 現在の安定版
  vendor/
    xxhash/                     # 比較用（xxhash.h / xxhash.c）
    sha256/                     # 比較用

tools/
  prng_stdout.c                 # ミキサーをカウンタモードPRNGとして stdout に流す
                                # PractRand パイプテスト用

docs/
  report-2026-05-12.md          # 第2・第3世代の比較レポート
  report-2026-05-14.md          # 第4世代の成績レポート
  report-2026-06-15.md          # PractRand導入・第5世代修正レポート ← 直近

compiled/                       # ビルド出力（gitignore対象）
```

---

## 現在の安定版

**`src/include/murmur_based_2026_06_15.h`** — `mixer_carried_murmur_2026_06_15()`

- murmur3 ファイナライザ風の変換＋XOR伝播を **2ループ**
- 内製テスト全PASS、PractRand **32GB / 328テスト 異常なし**
- 速度: **44.2 ns/call** (`-O2`)

第4世代 (2026_05_14) はループ1回でPractRand 64MBで即FAIL（下位ビット崩壊）。詳細は `docs/report-2026-06-15.md`。

---

## ビルドと実行

```sh
# 内製テストスイートをビルド・実行
make run

# PractRand用出力バイナリをビルド
make prng_stdout
```

`make run` は `src/main.c` の `mt_register()` に登録されたミキサーを `MT_TRIALS_QUICK`（65536回）でテストする。

---

## テストワークフロー

### 内製テスト（mixer_test.h）

`src/main.c` でミキサーを登録して `make run`:

```c
mt_register(mixer_carried_murmur_2026_06_15, "murmur-like 5th version");
mt_run_all_targets(MT_TRIALS_DEFAULT);  // QUICK(65k) or DEFAULT(1M)
```

測定項目: 雪崩効果 / ビット独立性(BIC) / 均一性 / 衝突耐性 / ns/call

### PractRand（外部統計テスト）

PractRandは `/home/user/dev/PractRand/` にある（プロジェクト外）。
ビルドスクリプト: `build_practrand.sh`

```sh
# prng_stdout をビルドしてパイプ
make prng_stdout
./compiled/prng_stdout | /home/user/dev/PractRand/RNG_test stdin -tlmax 4G
```

- `64M` で数秒、`1G` で約15秒、`4G` で約1分
- `[Low4/32]` カテゴリのFAILは下位ビット線形残留の典型的症状
- 内製テストはランダム入力前提のため連続入力の弱点を検出できない → PractRandは必須

---

## 新しいミキサーを試すときの手順

1. `src/include/murmur_based_YYYY_MM_DD.h` を作成
2. `src/main.c` に `#include` と `mt_register()` を追加
3. `make run` で内製テスト
4. `tools/prng_stdout.c` の `#include` を新しいヘッダに差し替えて `make prng_stdout`
5. PractRandで `64M` → `4G` と段階的にテスト
6. `docs/report-YYYY-MM-DD.md` に結果を記録

---

## 注意事項

- **ゼロ固定点**: murmur系ファイナライザは `0→0` の固定点を持つ。`tools/prng_stdout.c` はこれを避けるためカウンタを `{1,0,0,0}` からスタートしている。
- **`tools/prng_stdout.c` のインクルードパス**: `../src/include/...` の相対パスを使用（Makefileの `-I.` に頼らない）。
- **Makefile の SRCS**: `src/` 以下の `.c` を全列挙するので、`tools/` に `.c` を追加しても `mixer_test` バイナリには混入しない（`tools/` は対象外）。
- PractRandはプロジェクトリポジトリ外（`~/dev/PractRand/`）に置いている。

---

## 世代ごとの成績早見表

| バージョン | ns/call | PractRand 4GB | 備考 |
|---|---|---|---|
| 第2世代 (2026-05-12) | 159.5 ns | 未測定 | buf[8] 経由の複雑な変換 |
| 第3世代 (2026-05-13) | 119.5 ns | 未測定 | シンプル化・高速化 |
| 第4世代 (2026-05-14) | 31.4 ns | **FAIL**（64MBで崩壊） | ループ1回・下位ビット弱点 |
| **第5世代 (2026-06-15)** | **44.2 ns** | **PASS（282テスト）** | ループ2回・現安定版 |
