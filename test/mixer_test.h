/**
 * mixer_test.h
 * ミキサー品質テスト用シングルヘッダ
 *
 * 計測する3観点:
 *   1. 雪崩効果     (Avalanche Effect)   : 1bit変化で出力の何bit変わるか。理想50%
 *   2. ビット独立性 (Bit Independence)   : 入力の各bitが出力の各bitに独立して影響するか
 *   3. 均一性       (Uniformity)         : 出力のbit分布が偏ってないか
 *
 * 使い方:
 *   #define MIXER_TEST_IMPL
 *   #include "mixer_test.h"
 *
 *   // ミキサー関数を差し替える
 *   mt_result_t result = mt_run_all(my_mixer, MT_TRIALS_DEFAULT);
 *   mt_print_result(&result);
 */

#ifndef MIXER_TEST_H
#define MIXER_TEST_H

// clock_gettime を使うために必要 (POSIX環境のみ、Windows では不要)
#if !defined(_WIN32) && !defined(_WIN64)
#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// -----------------------------------------------------------------------
// 定数
// -----------------------------------------------------------------------

#define MT_BLOCK_SIZE 32            // ミキサーの入出力サイズ (byte)
#define MT_BITS (MT_BLOCK_SIZE * 8) // = 256

// 試行回数。執拗気味に。
#define MT_TRIALS_DEFAULT (1 << 20) // 約100万
#define MT_TRIALS_QUICK (1 << 16)   // 約6.5万 (軽量確認用)

// 雪崩効果の合格ライン
#define MT_AVALANCHE_OK_MIN 0.45      // avg反転率の下限 (理想50% ± 5%)
#define MT_AVALANCHE_OK_MAX 0.55      // avg反転率の上限
#define MT_AVALANCHE_MIN_FLOOR 0.40   // 最悪入力bitの下限 (1bitでも弱ければNG)
#define MT_AVALANCHE_STDDEV_MAX 0.020 // 反転率stddevの上限 (均質性)

// ビット独立性の合格ライン
#define MT_BIC_AVG_MIN 0.95 // 平均独立性スコアの下限 (対角除外)
#define MT_BIC_MIN_MIN 0.70 // 最悪ペアの独立性スコアの下限

// 均一性の合格ライン (各bitの1出現率が50% ± 3%)
#define MT_UNIFORMITY_OK_MIN 0.47
#define MT_UNIFORMITY_OK_MAX 0.53
#define MT_UNIFORMITY_STDDEV_MAX 0.005 // 全bit stddevの上限

// -----------------------------------------------------------------------
// 型定義
// -----------------------------------------------------------------------

typedef struct
{
    uint8_t data[MT_BLOCK_SIZE];
} mt_block_t;

// ミキサー関数の型: ブロックを破壊的に変換する
typedef void (*mt_mixer_fn)(mt_block_t *block);

// 雪崩効果の結果
typedef struct
{
    double avg_flip_rate; // 全入力bitについての平均反転率 (理想: 0.50)
    double min_flip_rate; // 最悪の入力bit (低いほど弱い)
    double max_flip_rate; // 最大の入力bit
    double stddev;        // 標準偏差 (小さいほど均質)
    bool passed;
} mt_avalanche_result_t;

// ビット独立性の結果
typedef struct
{
    double avg_independence; // 平均独立性スコア (理想: 1.0)
    double min_independence; // 最低スコア
    bool passed;
} mt_bit_independence_result_t;

// 均一性の結果
typedef struct
{
    double avg_one_rate; // 全出力bitの平均1出現率 (理想: 0.50)
    double min_one_rate; // 最低の出力bit
    double max_one_rate; // 最高の出力bit
    double stddev;
    bool passed;
} mt_uniformity_result_t;

// 総合結果
typedef struct
{
    mt_avalanche_result_t avalanche;
    mt_bit_independence_result_t bit_independence;
    mt_uniformity_result_t uniformity;
    uint32_t trials;
    double elapsed_sec; // テスト合計時間(参考)
    double bench_ns;    // ミキサー単体ns/call
    bool all_passed;
} mt_result_t;

// -----------------------------------------------------------------------
// 単体速度計測
// -----------------------------------------------------------------------

#define MT_BENCH_TRIALS 10000000 // 1000万回

typedef struct
{
    double total_sec;
    double ns_per_call;
    uint64_t trials;
} mt_bench_result_t;

mt_bench_result_t mt_bench(mt_mixer_fn mixer, uint64_t trials);
void mt_bench_all_targets(uint64_t trials);
void mt_print_bench(const mt_bench_result_t *r, const char *name);

// テスト対象
#define MT_NAME_BUF_SIZE 128
#define MT_MAX_TARGETS 16

typedef struct
{
    mt_mixer_fn mixer;
    char name[MT_NAME_BUF_SIZE];
} mt_target_t;

// ターゲット登録・一括実行
void mt_register(mt_mixer_fn mixer, const char *name);
void mt_run_all_targets(uint32_t trials);

// -----------------------------------------------------------------------
// 関数プロトタイプ
// -----------------------------------------------------------------------

mt_avalanche_result_t mt_test_avalanche(mt_mixer_fn mixer, uint32_t trials);
mt_bit_independence_result_t mt_test_bit_independence(mt_mixer_fn mixer, uint32_t trials);
mt_uniformity_result_t mt_test_uniformity(mt_mixer_fn mixer, uint32_t trials);

mt_result_t mt_run_all(mt_mixer_fn mixer, uint32_t trials);

void mt_print_result(const mt_result_t *r);
void mt_print_result_named(const mt_result_t *r, const char *name);
void mt_print_avalanche(const mt_avalanche_result_t *r);
void mt_print_bit_independence(const mt_bit_independence_result_t *r);
void mt_print_uniformity(const mt_uniformity_result_t *r);

// -----------------------------------------------------------------------
// 実装 (MIXER_TEST_IMPL を define したファイルにだけ展開)
// -----------------------------------------------------------------------

#ifdef MIXER_TEST_IMPL

#include <math.h>

// ---- プラットフォーム別タイマー -----------------------------------------
// Windows : QueryPerformanceCounter (分解能 ~100ns 以下)
// POSIX   : clock_gettime(CLOCK_MONOTONIC)

#if defined(_WIN32) || defined(_WIN64)
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
typedef LARGE_INTEGER mt__timestamp_t;
static inline mt__timestamp_t mt__now(void)
{
    LARGE_INTEGER t;
    QueryPerformanceCounter(&t);
    return t;
}
static inline double mt__elapsed_sec(mt__timestamp_t start, mt__timestamp_t end)
{
    LARGE_INTEGER freq;
    QueryPerformanceFrequency(&freq);
    return (double)(end.QuadPart - start.QuadPart) / (double)freq.QuadPart;
}
#else
#include <time.h>
typedef struct timespec mt__timestamp_t;
static inline mt__timestamp_t mt__now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t;
}
static inline double mt__elapsed_sec(mt__timestamp_t start, mt__timestamp_t end)
{
    return (double)(end.tv_sec - start.tv_sec) + (double)(end.tv_nsec - start.tv_nsec) * 1e-9;
}
#endif

// ---- 内部ユーティリティ ------------------------------------------------

// xorshift64 による軽量乱数 (テスト入力生成用)
// シードは mt_run_all / mt_run_all_targets で実行ごとに設定される
static uint64_t mt__rng_state = 0;

static inline uint64_t mt__rand64(void)
{
    uint64_t x = mt__rng_state;
    x ^= x << 13;
    x ^= x >> 7;
    x ^= x << 17;
    mt__rng_state = x;
    return x;
}

static inline void mt__rand_block(mt_block_t *b)
{
    uint64_t *p = (uint64_t *)(void *)b->data; // 構造体なのでアライメントOK
    p[0] = mt__rand64();
    p[1] = mt__rand64();
    p[2] = mt__rand64();
    p[3] = mt__rand64();
}

// bit i を反転させたブロックを返す
static inline mt_block_t mt__flip_bit(const mt_block_t *src, int bit)
{
    mt_block_t dst = *src;
    dst.data[bit / 8] ^= (uint8_t)(1u << (bit % 8));
    return dst;
}

// 2ブロック間の異なるbit数を返す
static inline int mt__popcount_diff(const mt_block_t *a, const mt_block_t *b)
{
    int count = 0;
    for (int i = 0; i < MT_BLOCK_SIZE; i++)
    {
        uint8_t diff = a->data[i] ^ b->data[i];
        // popcount
        uint8_t v = diff;
        v = v - ((v >> 1) & 0x55u);
        v = (v & 0x33u) + ((v >> 2) & 0x33u);
        count += (v + (v >> 4)) & 0x0Fu;
    }
    return count;
}

// 標準偏差計算
static inline double mt__stddev(const double *arr, int n, double mean)
{
    double sum = 0.0;
    for (int i = 0; i < n; i++)
    {
        double d = arr[i] - mean;
        sum += d * d;
    }
    return sqrt(sum / n);
}

// ---- 雪崩効果 ----------------------------------------------------------

mt_avalanche_result_t mt_test_avalanche(mt_mixer_fn mixer, uint32_t trials)
{
    // 各入力bitについて、そのbitを反転したとき出力の何bitが変化するかを計測
    // flip_rates[i] = 入力bit i を反転したときの平均反転率
    double flip_rates[MT_BITS] = {0};

    for (uint32_t t = 0; t < trials; t++)
    {
        mt_block_t base;
        mt__rand_block(&base);

        // ベースの出力
        mt_block_t base_out = base;
        mixer(&base_out);

        // 各bitを1つずつ反転
        for (int i = 0; i < MT_BITS; i++)
        {
            mt_block_t flipped = mt__flip_bit(&base, i);
            mixer(&flipped);
            int diff = mt__popcount_diff(&base_out, &flipped);
            flip_rates[i] += (double)diff / MT_BITS;
        }
    }

    // 平均化
    for (int i = 0; i < MT_BITS; i++)
    {
        flip_rates[i] /= trials;
    }

    // 統計
    double sum = 0.0, mn = 1.0, mx = 0.0;
    for (int i = 0; i < MT_BITS; i++)
    {
        sum += flip_rates[i];
        if (flip_rates[i] < mn)
            mn = flip_rates[i];
        if (flip_rates[i] > mx)
            mx = flip_rates[i];
    }
    double avg = sum / MT_BITS;
    double sd = mt__stddev(flip_rates, MT_BITS, avg);

    mt_avalanche_result_t r;
    r.avg_flip_rate = avg;
    r.min_flip_rate = mn;
    r.max_flip_rate = mx;
    r.stddev = sd;
    r.passed = (avg >= MT_AVALANCHE_OK_MIN && avg <= MT_AVALANCHE_OK_MAX) && (mn >= MT_AVALANCHE_MIN_FLOOR) && (sd <= MT_AVALANCHE_STDDEV_MAX);
    return r;
}

// ---- ビット独立性 -------------------------------------------------------

mt_bit_independence_result_t mt_test_bit_independence(mt_mixer_fn mixer, uint32_t trials)
{
    // 入力bit i を反転したとき、出力bit j が変化する確率を計測
    // 理想は全ての(i,j)ペアで50%
    // メモリ節約のため、256x256の行列は動的確保
    uint32_t *counts = (uint32_t *)calloc(MT_BITS * MT_BITS, sizeof(uint32_t));
    if (!counts)
    {
        mt_bit_independence_result_t r = {0};
        r.passed = false;
        return r;
    }

    for (uint32_t t = 0; t < trials; t++)
    {
        mt_block_t base;
        mt__rand_block(&base);

        mt_block_t base_out = base;
        mixer(&base_out);

        for (int i = 0; i < MT_BITS; i++)
        {
            mt_block_t flipped = mt__flip_bit(&base, i);
            mixer(&flipped);

            // 出力のどのbitが変化したか
            for (int byte = 0; byte < MT_BLOCK_SIZE; byte++)
            {
                uint8_t diff = base_out.data[byte] ^ flipped.data[byte];
                for (int b = 0; b < 8; b++)
                {
                    if (diff & (1u << b))
                    {
                        counts[i * MT_BITS + byte * 8 + b]++;
                    }
                }
            }
        }
    }

    // 独立性スコア: 各(i,j)の変化率が0.5に近いほど1.0に近い
    // score(i,j) = 1 - |rate(i,j) - 0.5| * 2
    // i==j (入力bitと同位置の出力bit) は自己相関が高くなりやすいため除外
    double sum = 0.0, mn = 1.0;
    int pair_count = 0;
    for (int i = 0; i < MT_BITS; i++)
    {
        for (int j = 0; j < MT_BITS; j++)
        {
            if (i == j)
                continue; // 対角除外
            double rate = (double)counts[i * MT_BITS + j] / trials;
            double score = 1.0 - fabs(rate - 0.5) * 2.0;
            sum += score;
            if (score < mn)
                mn = score;
            pair_count++;
        }
    }

    free(counts);

    mt_bit_independence_result_t r;
    r.avg_independence = sum / pair_count;
    r.min_independence = mn;
    r.passed = (r.avg_independence >= MT_BIC_AVG_MIN) && (r.min_independence >= MT_BIC_MIN_MIN);
    return r;
}

// ---- 均一性 ------------------------------------------------------------

mt_uniformity_result_t mt_test_uniformity(mt_mixer_fn mixer, uint32_t trials)
{
    // ランダム入力に対して、出力の各bitが1になる確率を計測
    uint32_t one_counts[MT_BITS] = {0};

    for (uint32_t t = 0; t < trials; t++)
    {
        mt_block_t block;
        mt__rand_block(&block);
        mixer(&block);

        for (int byte = 0; byte < MT_BLOCK_SIZE; byte++)
        {
            for (int b = 0; b < 8; b++)
            {
                if (block.data[byte] & (1u << b))
                {
                    one_counts[byte * 8 + b]++;
                }
            }
        }
    }

    double rates[MT_BITS];
    double sum = 0.0, mn = 1.0, mx = 0.0;
    for (int i = 0; i < MT_BITS; i++)
    {
        rates[i] = (double)one_counts[i] / trials;
        sum += rates[i];
        if (rates[i] < mn)
            mn = rates[i];
        if (rates[i] > mx)
            mx = rates[i];
    }
    double avg = sum / MT_BITS;
    double sd = mt__stddev(rates, MT_BITS, avg);

    mt_uniformity_result_t r;
    r.avg_one_rate = avg;
    r.min_one_rate = mn;
    r.max_one_rate = mx;
    r.stddev = sd;
    r.passed = (mn >= MT_UNIFORMITY_OK_MIN && mx <= MT_UNIFORMITY_OK_MAX) && (sd <= MT_UNIFORMITY_STDDEV_MAX);
    return r;
}

// ---- ターゲット登録・一括実行 -------------------------------------------

static mt_target_t mt__targets[MT_MAX_TARGETS];
static int mt__target_count = 0;

void mt_register(mt_mixer_fn mixer, const char *name)
{
    if (mt__target_count >= MT_MAX_TARGETS)
    {
        printf("[mixer_test] mt_register: full (max %d)\n", MT_MAX_TARGETS);
        return;
    }
    mt__targets[mt__target_count].mixer = mixer;
    int i = 0;
    while (i < MT_NAME_BUF_SIZE - 1 && name[i])
    {
        mt__targets[mt__target_count].name[i] = name[i];
        i++;
    }
    mt__targets[mt__target_count].name[i] = '\0';
    mt__target_count++;
}

void mt_run_all_targets(uint32_t trials)
{
    if (mt__target_count == 0)
    {
        printf("[mixer_test] no targets registered.\n");
        return;
    }
    for (int i = 0; i < mt__target_count; i++)
    {
        mt_result_t r = mt_run_all(mt__targets[i].mixer, trials);
        mt_print_result_named(&r, mt__targets[i].name);
    }
}

// ---- 単体速度計測 -------------------------------------------------------

mt_bench_result_t mt_bench(mt_mixer_fn mixer, uint64_t trials)
{
    mt_block_t block;
    mt__rand_block(&block);

    mt__timestamp_t ts_start = mt__now();
    for (uint64_t i = 0; i < trials; i++)
    {
        mixer(&block);
    }
    mt__timestamp_t ts_end = mt__now();

    mt_bench_result_t r;
    r.trials = trials;
    r.total_sec = mt__elapsed_sec(ts_start, ts_end);
    r.ns_per_call = r.total_sec * 1e9 / (double)trials;
    return r;
}

void mt_print_bench(const mt_bench_result_t *r, const char *name)
{
    printf("--- Bench [%s] ---\n", name);
    printf("  trials        : %llu\n", (unsigned long long)r->trials);
    printf("  total time    : %.3f sec\n", r->total_sec);
    printf("  ns/call       : %.1f ns\n", r->ns_per_call);
}

void mt_bench_all_targets(uint64_t trials)
{
    if (mt__target_count == 0)
    {
        printf("[mixer_test] no targets registered.\n");
        return;
    }
    printf("========================================\n");
    printf("  benchmark (trials=%llu)\n", (unsigned long long)trials);
    printf("========================================\n");
    for (int i = 0; i < mt__target_count; i++)
    {
        mt_bench_result_t r = mt_bench(mt__targets[i].mixer, trials);
        mt_print_bench(&r, mt__targets[i].name);
    }
    printf("========================================\n");
}

// ---- 全テスト一括 -------------------------------------------------------

mt_result_t mt_run_all(mt_mixer_fn mixer, uint32_t trials)
{
    mt_result_t r;
    r.trials = trials;

    // 実行ごとにシードを変える。再現のためシード値を表示する。
    mt__rng_state = (uint64_t)time(NULL) ^ (uint64_t)(uintptr_t)mixer;
    printf("[mixer_test] rng seed     = 0x%016llx\n", (unsigned long long)mt__rng_state);
    printf("[mixer_test] trials       = %u\n", trials);

    mt__timestamp_t ts_start = mt__now();

    printf("[mixer_test] testing avalanche effect...\n");
    r.avalanche = mt_test_avalanche(mixer, trials);

    printf("[mixer_test] testing bit independence...\n");
    r.bit_independence = mt_test_bit_independence(mixer, trials);

    printf("[mixer_test] testing uniformity...\n");
    r.uniformity = mt_test_uniformity(mixer, trials);

    mt__timestamp_t ts_end = mt__now();
    r.elapsed_sec = mt__elapsed_sec(ts_start, ts_end);

    // 単体ベンチ
    mt_bench_result_t bench = mt_bench(mixer, MT_BENCH_TRIALS);
    r.bench_ns = bench.ns_per_call;

    r.all_passed = r.avalanche.passed && r.bit_independence.passed && r.uniformity.passed;
    return r;
}

// ---- 表示 ---------------------------------------------------------------

void mt_print_avalanche(const mt_avalanche_result_t *r)
{
    printf("--- Avalanche Effect ---\n");
    printf("  avg flip rate : %.4f  (ideal: 0.5000, range: %.2f-%.2f)\n",
           r->avg_flip_rate, MT_AVALANCHE_OK_MIN, MT_AVALANCHE_OK_MAX);
    printf("  min flip rate : %.4f  (floor: %.2f)\n", r->min_flip_rate, MT_AVALANCHE_MIN_FLOOR);
    printf("  max flip rate : %.4f\n", r->max_flip_rate);
    printf("  stddev        : %.4f  (max: %.3f)\n", r->stddev, MT_AVALANCHE_STDDEV_MAX);
    printf("  result        : %s\n", r->passed ? "PASS" : "FAIL");
}

void mt_print_bit_independence(const mt_bit_independence_result_t *r)
{
    printf("--- Bit Independence (diagonal excluded) ---\n");
    printf("  avg score     : %.4f  (min: %.2f)\n", r->avg_independence, MT_BIC_AVG_MIN);
    printf("  min score     : %.4f  (floor: %.2f)\n", r->min_independence, MT_BIC_MIN_MIN);
    printf("  result        : %s\n", r->passed ? "PASS" : "FAIL");
}

void mt_print_uniformity(const mt_uniformity_result_t *r)
{
    printf("--- Uniformity ---\n");
    printf("  avg 1-rate    : %.4f  (ideal: 0.5000, range: %.2f-%.2f)\n",
           r->avg_one_rate, MT_UNIFORMITY_OK_MIN, MT_UNIFORMITY_OK_MAX);
    printf("  min 1-rate    : %.4f\n", r->min_one_rate);
    printf("  max 1-rate    : %.4f\n", r->max_one_rate);
    printf("  stddev        : %.4f  (max: %.3f)\n", r->stddev, MT_UNIFORMITY_STDDEV_MAX);
    printf("  result        : %s\n", r->passed ? "PASS" : "FAIL");
}

void mt_print_result_named(const mt_result_t *r, const char *name)
{
    printf("========================================\n");
    printf("  [%s] (trials=%u)\n", name, r->trials);
    printf("========================================\n");
    mt_print_avalanche(&r->avalanche);
    mt_print_bit_independence(&r->bit_independence);
    mt_print_uniformity(&r->uniformity);
    printf("--- Performance ---\n");
    printf("  ns/call       : %.1f ns  (bench trials=%d)\n", r->bench_ns, MT_BENCH_TRIALS);
    printf("========================================\n");
    printf("  OVERALL: %s\n", r->all_passed ? "PASS" : "FAIL");
    printf("========================================\n");
}

void mt_print_result(const mt_result_t *r)
{
    printf("========================================\n");
    printf("  mixer_test results (trials=%u)\n", r->trials);
    printf("========================================\n");
    mt_print_avalanche(&r->avalanche);
    mt_print_bit_independence(&r->bit_independence);
    mt_print_uniformity(&r->uniformity);
    printf("--- Performance ---\n");
    printf("  ns/call       : %.1f ns  (bench trials=%d)\n", r->bench_ns, MT_BENCH_TRIALS);
    printf("========================================\n");
    printf("  OVERALL: %s\n", r->all_passed ? "PASS" : "FAIL");
    printf("========================================\n");
}

#endif // MIXER_TEST_IMPL
#endif // MIXER_TEST_H