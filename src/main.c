/**
 * mixer_test_example.c
 */

#define MIXER_TEST_IMPL
#include "mixer_test.h"
#include "murmur_based_2026_05_14.h"


#include <stdint.h>
#include <string.h>




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