/*************************
*
* TEA Variant Key Solver
* TEA input data management and related calculations
*
*************************/

#include <stdint.h>
#include "tea_data.h"

// Calculates and stores (key2 ^ a) + key3
void half_round::calc_diff_term()
{
    if (!diff_term_set)
    {
        // (key2 ^ a) + key3 = b - b_prime - (a << 4) - ((a >> 5) ^ sum)
        diff_term = b - b_prime - (a << 4) - ((a >> 5) ^ sum);
        diff_term_set = true;
    }
}

// Calculates key3 given key2 using the data from this round
uint32_t half_round::key3_from_key2(uint32_t key2)
{
    calc_diff_term();

    //key3 = (key2 ^ a) + key3 - (key2 ^ a)
    return diff_term - (key2 ^ a);
}

// Verifies a key pair
bool half_round::verify_keys(uint32_t key2, uint32_t key3)
{
    uint32_t trial_b_prime = b - ((a << 4) + (key2 ^ a) + ((a >> 5) ^ sum) + key3);

    if (trial_b_prime == b_prime)
        return true;

    return false;
}
