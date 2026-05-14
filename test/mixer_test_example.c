/**
 * mixer_test_example.c
 * コンパイル:
 */
/*
gcc -O2 -o ./compiled/mixer_test ./test/mixer_test_example.c \
    ./vendor/sha256/sha256.c \
    ./vendor/xxhash/xxhash.c \
    -lm && ./compiled/mixer_test


gcc -O2 -o ./compiled/mixer_test ./test/mixer_test_example.c -lm && ./compiled/mixer_test

*/

#define MIXER_TEST_IMPL
#include "mixer_test.h"


#include <stdint.h>
#include <string.h>

// ちょっと変わったmurmur風ミキサー。ビットの回転とキャリーを入れてみたもの。雪崩効果もある安定版。
static void mixer_carried_murmur_2026_05_13(mt_block_t *value_ref)
{
    // 借りる
    mt_block_t tmp = *value_ref;

    // uint64_tのバッファにコピー。上位32bitは0になる。
    uint64_t buf[8];
    // 回転しながら飛び飛びにコピーして行く。
    for (int shift = 0; shift < 4; shift++)
    {
        for (int idx = 0; idx < 8; idx++)
        {
            buf[idx] |= (uint64_t)tmp.data[shift * 8 + idx] << (shift * 8);
        }
    }

    // murmurハッシュ風の変換。
    for (int i = 0; i < 8; i++)
    {
        buf[i] ^= buf[i] >> 33;
        buf[i] *= 0xff51afd7ed558ccdULL;
        buf[i] ^= buf[i] >> 33;
        buf[i] *= 0xc4ceb9fe1a85ec53ULL;
        buf[i] ^= buf[i] >> 33;
    }
    // キャリーを伝播させる。
    for (int i = 0; i < 8; i++)
    {
        buf[(i + 1) % 8] ^= buf[i] >> 32;
    }

    // もとのバッファの形式に書き戻す。
    for (int b = 0; b < 4; b++) // バイト段
    {
        for (int idx = 0; idx < 8; idx++) // ブロック
        {
            tmp.data[b * 8 + idx] ^= (uint8_t)(buf[idx] >> (b * 8));
        }
    }

    // 返す
    *value_ref = tmp;
}

static void mixer_carried_murmur_2026_05_14(mt_block_t *value_ref)
{
    // 借りる
    mt_block_t tmp = *value_ref;

    for (int i = 0; i < 4; i++)
    {
        // murmurハッシュ風
        tmp.word[(i) % 4] ^= tmp.word[(i) % 4] >> 33;
        tmp.word[(i) % 4] *= 0xff51afd7ed558ccdULL;
        tmp.word[(i) % 4] ^= tmp.word[(i) % 4] >> 33;
        tmp.word[(i) % 4] *= 0xc4ceb9fe1a85ec53ULL;
        tmp.word[(i) % 4] ^= tmp.word[(i) % 4] >> 33;
        // 上位に単純に伝播
        tmp.word[(i + 1) % 4] ^= tmp.word[(i) % 4];
        tmp.word[(i + 2) % 4] ^= tmp.word[(i) % 4];
        tmp.word[(i + 3) % 4] ^= tmp.word[(i) % 4];
    }

    // 返す
    *value_ref = tmp;
}

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main(void)
{
    //mt_register(mixer_carried_murmur_2026_05_13, "murmur-like 3rd version");
    mt_register(mixer_carried_murmur_2026_05_14, "murmur-like 4th version");
    mt_run_all_targets(MT_TRIALS_QUICK);
    return 0;
}