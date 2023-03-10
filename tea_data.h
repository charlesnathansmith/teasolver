#pragma once

/*************************
*
* TEA Variant Key Solver
* TEA input data management and related calculations
*
*************************/

#include <stdint.h>
#include <vector>

// Stores info about a single half-round equation collected from TEA routine data tracing
struct half_round
{
    uint32_t a, b, sum, b_prime;
    uint32_t diff_term;
    bool diff_term_set;

    half_round(uint32_t a, uint32_t b, uint32_t sum, uint32_t b_prime) :
        a(a), b(b), sum(sum), b_prime(b_prime), diff_term(0), diff_term_set(false) {}

    // Calculates and stores (key2 ^ a) + key3
    void calc_diff_term();

    // Calculates key3 given key2 using the data from this round
    uint32_t key3_from_key2(uint32_t key2);

    // Verifies a key pair
    bool verify_keys(uint32_t key2, uint32_t key3);
};

typedef std::vector<half_round> input_data;
