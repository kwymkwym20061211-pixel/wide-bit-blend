/**
 * tools/prng_stdout.c
 *
 * ミキサーをカウンタモードPRNGとして使い、出力を stdout にバイナリで流す。
 * PractRand へのパイプ用:
 *   ./compiled/prng_stdout | RNG_test stdin32
 *
 * カウンタは 256bit 全体をインクリメントし、バイアスを最小化する。
 */

#define MIXER_TEST_IMPL
#include "../src/include/mixer_test.h"
#include "../src/include/murmur_based_2026_06_15.h"

#include <stdio.h>
#include <stdint.h>

static inline void increment_counter(mt_block_t *c)
{
    for (int i = 0; i < 4; i++) {
        c->word[i]++;
        if (c->word[i] != 0)
            break;
    }
}

int main(void)
{
    mt_block_t counter = {{1, 0, 0, 0}};

#if defined(_WIN32) || defined(_WIN64)
    _setmode(_fileno(stdout), _O_BINARY);
#endif

    while (1) {
        mt_block_t out = counter;
        mixer_carried_murmur_2026_06_15(&out);
        if (fwrite(out.data, 1, MT_BLOCK_SIZE, stdout) != MT_BLOCK_SIZE)
            break;
        increment_counter(&counter);
    }
    return 0;
}
