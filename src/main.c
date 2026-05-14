/**
 * mixer_test_example.c
 */

#define MIXER_TEST_IMPL
#include "mixer_test.h"


#include <stdint.h>
#include <string.h>



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