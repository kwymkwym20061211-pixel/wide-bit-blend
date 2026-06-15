/**
 * mixer_test_example.c
 */

#define MIXER_TEST_IMPL
#include "./include/mixer_test.h"
#include "./include/murmur_based_2026_06_15.h"


#include <stdint.h>
// -----------------------------------------------------------------------
// main
// -----------------------------------------------------------------------

int main(void)
{
    //mt_register(mixer_carried_murmur_2026_05_13, "murmur-like 3rd version");
    mt_register(mixer_carried_murmur_2026_06_15, "murmur-like");
    mt_run_all_targets(MT_TRIALS_QUICK);
    return 0;
}