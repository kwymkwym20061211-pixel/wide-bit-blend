#ifndef MIXER_MURMUR_BASED_2026_05_14_H
#define MIXER_MURMUR_BASED_2026_05_14_H

#include "mixer_test.h"

/**
 * [2026-05-15]
 * 256bitミキサー。murmurハッシュ風の変換を4回行い、各回で上位に単純に伝播させる。雪崩効果もある安定版。
 * ns/call : 31.4 ns
 */ 
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

#endif // MIXER_MURMUR_BASED_2026_05_14_H