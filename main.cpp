/*************************
*
*  Key solver for a TEA variant with half-rounds of the form:
*
*  b_prime = b - ((a << 4) + (key2 ^ a) + ((a >> 5) ^ sum) + key3);
*
*  Where a, b, sum, and b_prime are known for multiple rounds but not the underlying keys
*
* Generally, the more data provided, the more key bits can be quickly recovered,
* though it depends on the number of entries provided and the relative entropy
* between 'a' values across entries
*
* Key bits not immediately recoverable will be brute forced, though it's usually only 1-2
* bits with any kind of reasonable input data to work with (it's at least 1, as the MSB can't be
* directly solved ,) but having to test 2-4 keys beats having to test 2^32
*
* In the really degenerate case where the entropy is extremely low between different a values
* AND different b values, it's possible that there are multiple valid key pairs for the entire data set
* It's rare enough that it's really not worth anticipating here, but you just don't break after
* finding a valid key pair in order to keep looking for the others
*
* Nathan Smith
* https://github.com/charlesnathansmith/teasolver
* 
*************************/

#include <iostream>
#include <bitset>
#include <vector>
#include "tea_data.h"
#include "bitmap.h"

// Input entries in the form: { a, b, sum, b_prime }
// Replace these with the relevant values recovered when analyzing the TEA rounds
uint32_t raw_inputs[][4] = { { 0xea7895f7, 0x98e346f7, 0x28b7bd67, 0xa0b58ff4 },
                             { 0x715e30c9, 0xa0b58ff4, 0x8a8043ae, 0x4a25987b },
                             { 0x191f3d4f, 0x4a25987b, 0xec48c9f5, 0x7c505d29 },
                             { 0x4c8c5408, 0x7c505d29, 0x4e11503c, 0xe5414c8b },
                             { 0x2cdb7aed, 0xe5414c8b, 0xafd9d683, 0x6ad2182  },
                             { 0x2c2e1c7c, 0x6ad2182,  0x11a25cca, 0xd095d843 },
                             { 0x5e8683ad, 0xd095d843, 0x736ae311, 0xe2c4fe42 } };

int main()
{
    // Input data could be initialized this way to begin with to avoid copying,
    // but it's just tidier to supply it in the grid above for this demo
    input_data inputs;

    for (size_t i = 0; i < sizeof(raw_inputs) / sizeof(raw_inputs[0]); i++)
        inputs.push_back(std::move(half_round(raw_inputs[i][0], raw_inputs[i][1], raw_inputs[i][2], raw_inputs[i][3])));

    printf("Building differential bitmap... ");
    bitmap solver_map(inputs);

    puts("done.\n");

    // Solvability info
    uint32_t solvable_bits = solver_map.solvable_bits();

    std::bitset<32> solvable_mask(solvable_bits);
    printf("key2 bits determinable using initial fast solver: %s\n\n", solvable_mask.to_string().c_str());
    
    // key2 fast solver
    puts("Solving for key2...");
    uint32_t key2 = 0;

    for (size_t bit = 0; bit < 31; bit++)
    {
        if ((solvable_bits >> bit) & 1) //This bit of the key is determinable at this stage
        {
            // Iters must be valid if this bit is solvable
            auto zero_it = solver_map[bit].zero;
            auto  one_it = solver_map[bit].one;
    
            // Likewise, the diff_terms must have been calculated when inputs were added to bitmap
            // c = (key2 ^ a0) - (key2 ^ a1) + (key3 - key3)
            // c = (key2 ^ a0) - (key2 ^ a1)
            uint32_t c = zero_it->diff_term - one_it->diff_term;

            // For an individual bit n, where n=0 is LSB
            //  c[n+1] =   a0[n+1] ^ a1[n+1] ^ ~key[n] //Binary subtraction
            // ~key[n] =   a0[n+1] ^ a1[n+1] ^ c[n+1]
            //  key[n] = ~(a0[n+1] ^ a1[n+1] ^ c[n+1])
            uint32_t a0_np1 = (zero_it->a >> (bit + 1)) & 1;    // a0[n+1]
            uint32_t a1_np1 = ( one_it->a >> (bit + 1)) & 1;    // a1[n+1]
            uint32_t c_np1  = (         c >> (bit + 1)) & 1;    //  c[n+1]

            uint32_t k2_n = ~(a0_np1 ^ a1_np1 ^ c_np1) & 1;     // key[n]

            key2 |= k2_n << bit;    // Drop it into the key at the correct position
        }
    }

    // Try all potential key2 values given known good bits
    // 
    // Credit to MBo for solving the key permutation problem before I ever knew I had it:
    // https://stackoverflow.com/questions/49429896/generate-permutations-with-k-fixed-bits
    // There's one more solution where submask==0 that his solution drops, though, so we have to add that in
    
    uint32_t unknown_mask = ~solvable_bits;      // Mask that's 1 for bits we need to solve, 0 for known
    uint32_t known_bits = key2 & solvable_bits;  // Values of only the known good bits with unknowns masked out
    uint32_t key3;
    bool verified = false;

    // Generate possible keys sequentially and verify them against input data
    for (uint32_t submask = unknown_mask; submask != 0; submask = (submask - 1) & unknown_mask)
    {
        key2 = submask | known_bits;
        key3 = inputs[0].key3_from_key2(key2);

        printf("\nTrying %.8x... ", key2);
        
        verified = true;

        for (auto in = ++inputs.begin(); in != inputs.end(); in++) // ++ Because we're testing value from inputs[0] against the rest
        {
            if (in->key3_from_key2(key2) != key3) // Relatively fast because a lot of the work gets cached in diff_term
            {
                verified = false;
                break;
            }
        }

        if (verified)
        {
            printf("found!\tkey2 key3: %.8x %.8x\n", key2, key3);
            break;
        }
    }

    // Test case where submask == 0 if necessary
    if (!verified)
    {
        key2 = known_bits;
        key3 = inputs[0].key3_from_key2(key2);

        printf("Trying %.8x... \n", key2);

        verified = true;

        for (auto in = ++inputs.begin(); in != inputs.end(); in++)
        {
            if (in->key3_from_key2(key2) != key3)
            {
                verified = false;
                break;
            }
        }

        if (verified)
            printf("found!\tkey2 key3: %.8x %.8x\n", key2, key3);
    }

    if (!verified)
        printf("Valid key not found!\n");

    return 0;
}
