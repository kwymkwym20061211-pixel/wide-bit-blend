#ifndef MIX_2026_06_15
#define MIX_2026_06_15

#include "mixer_test.h"

static void mixer_carried_murmur_2026_06_15(mt_block_t *value_ref)
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

#endif
