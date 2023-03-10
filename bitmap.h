#pragma once

/*************************
*
* TEA Variant Key Solver
* Manages pairs of input data entries needed for differential solving
*
*************************/

#include <stdint.h>
#include "tea_data.h"

struct bit_info
{
    input_data::iterator one, zero;
};

class bitmap
{
    bit_info input_iters[31];       // Holds iterators to input entries that have 1s or 0s in the necessary bit positions
    uint32_t ones_mask, zeros_mask; // Keeps up with whether we've found valid entries for each of the bits

public:
    bitmap(input_data& in);

    // Bit mask indicating bits recoverable by fast solver
    uint32_t solvable_bits() const { return ones_mask & zeros_mask; }

    // Access to discovered input entries needed for solving
    // Caller responsible for requesting an in-bounds element with corresponding solvable bit set
    // If you ask for garbage, garbage you shall receive
    const bit_info& operator[](size_t i) const { return input_iters[i];  }
};
