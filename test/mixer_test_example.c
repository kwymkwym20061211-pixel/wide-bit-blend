/**
 * mixer_test_example.c
 * コンパイル:
 */
/*
gcc -O2 -o ./compiled/mixer_test ./test/mixer_test_example.c \
    ./vendor/sha256/sha256.c \
    ./vendor/xxhash/xxhash.c \
    -lm && ./compiled/mixer_test

*/

#define MIXER_TEST_IMPL
#include "mixer_test.h"
#include "./../vendor/sha256/sha256.h"
#include "./../vendor/xxhash/xxhash.h"

#include <stdint.h>
#include <string.h>

static void mixer_sha256(mt_block_t *value_ref)
{
    SHA256_CTX ctx;
    uint8_t out[32];
    sha256_init(&ctx);
    sha256_update(&ctx, value_ref->data, MT_BLOCK_SIZE);
    sha256_final(&ctx, out);
    memcpy(value_ref->data, out, 32);
}

static void mixer_xxhash(mt_block_t *value_ref)
{
    XXH128_hash_t h1 = XXH3_128bits(value_ref->data, MT_BLOCK_SIZE);
    XXH128_hash_t h2 = XXH3_128bits(value_ref->data + 16, MT_BLOCK_SIZE - 16);
    h1.low64 ^= h2.high64;
    h1.high64 ^= h2.low64;
    h2.low64 ^= h1.high64;
    h2.high64 ^= h1.low64;
    memcpy(value_ref->data, &h1, 16);
    memcpy(value_ref->data + 16, &h2, 16);
}
static void mixer_murmur_1(mt_block_t *value_ref)
{
    uint64_t v[4];
    memcpy(v, value_ref->data, 32);

    v[0] ^= v[3];
    v[0] ^= v[2];
    v[1] ^= v[0];
    v[1] ^= v[3];
    v[2] ^= v[1];
    v[2] ^= v[0];
    v[3] ^= v[2];
    v[3] ^= v[1];

    v[0] ^= v[0] >> 33;
    v[0] *= 0xff51afd7ed558ccdULL;
    v[0] ^= v[0] >> 33;
    v[0] *= 0xc4ceb9fe1a85ec53ULL;
    v[0] ^= v[0] >> 33;
    v[1] ^= v[1] >> 33;
    v[1] *= 0xff51afd7ed558ccdULL;
    v[1] ^= v[1] >> 33;
    v[1] *= 0xc4ceb9fe1a85ec53ULL;
    v[1] ^= v[1] >> 33;
    v[2] ^= v[2] >> 33;
    v[2] *= 0xff51afd7ed558ccdULL;
    v[2] ^= v[2] >> 33;
    v[2] *= 0xc4ceb9fe1a85ec53ULL;
    v[2] ^= v[2] >> 33;
    v[3] ^= v[3] >> 33;
    v[3] *= 0xff51afd7ed558ccdULL;
    v[3] ^= v[3] >> 33;
    v[3] *= 0xc4ceb9fe1a85ec53ULL;
    v[3] ^= v[3] >> 33;

    v[0] ^= v[1];
    v[0] ^= v[2];
    v[1] ^= v[2];
    v[1] ^= v[3];
    v[2] ^= v[3];
    v[2] ^= v[0];
    v[3] ^= v[0];
    v[3] ^= v[1];

    memcpy(value_ref->data, v, 32);
}

// ちょっと変わったmurmur風ミキサー。ビットの回転とキャリーを入れてみたもの。雪崩効果もある安定版。
static void mixer_carried_murmur_2026_05_12(mt_block_t *value_ref)
{
    // 借りる
    mt_block_t tmp = *value_ref;

    // uint64_tのバッファにコピー。上位32bitは0になる。
    uint64_t buf[8];
    int shift_count = 0;
    // 回転しながら飛び飛びにコピーして行く。
    for (int i = 0; i < 32; i++)
    {
        buf[i % 8] |= (uint32_t)(tmp.data[i] << shift_count);
        if (i % 8 == 7)
        {
            shift_count += 8;
        }
    }

    // murmurハッシュ風の変換を二周。キャリーの上位32bitは1個上に伝播。
    // メモ：ループをそれぞれのハッシュとキャリーで分割したところ高速になった。
    for (int i = 0; i < 8; i++)
    {
        int this_idx = i;
        buf[this_idx] ^= buf[this_idx] >> 33;
        buf[this_idx] *= 0xff51afd7ed558ccdULL;
        buf[this_idx] ^= buf[this_idx] >> 33;
        buf[this_idx] *= 0xc4ceb9fe1a85ec53ULL;
        buf[this_idx] ^= buf[this_idx] >> 33;
    }
    for (int i = 0; i < 8; i++)
    {
        buf[(i + 1) % 8] ^= buf[i] >> 32;
    }
    for (int i = 0; i < 8; i++)
    {
        int this_idx = i;
        buf[this_idx] ^= buf[this_idx] >> 33;
        buf[this_idx] *= 0xff51afd7ed558ccdULL;
        buf[this_idx] ^= buf[this_idx] >> 33;
        buf[this_idx] *= 0xc4ceb9fe1a85ec53ULL;
        buf[this_idx] ^= buf[this_idx] >> 33;
    }
    for (int i = 0; i < 8; i++)
    {
        buf[(i + 1) % 8] ^= buf[i] >> 32;
    }

    // もとのバッファの形式に書き戻す。
    for (int i = 0; i < 32; i++)
    {
        int idx = i % 8;
        tmp.data[i] ^= (int8_t)(buf[idx] >> ((i / 8) * 8)); // 回転して書き戻す
        // ここで対角線上のバイトとXORしても精度に大差ないことを確認。
    }
    // 返す
    *value_ref = tmp;
}

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

// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main(void)
{
    mt_register(mixer_carried_murmur_2026_05_12, "murmur-like 2nd version");
    mt_register(mixer_carried_murmur_2026_05_13, "murmur-like 3rd version");
    mt_run_all_targets(MT_TRIALS_QUICK);
    return 0;
}