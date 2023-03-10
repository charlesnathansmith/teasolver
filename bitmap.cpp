/*************************
*
* TEA Variant Key Solver
* Manages pairs of input data entries needed for differential solving
*
*************************/

#include <stdint.h>
#include "bitmap.h"

bitmap::bitmap(input_data& in)
{
    ones_mask = zeros_mask = 0;

    // Fill in needed iterators into input data
    // For each bit position, we need one data entry with an 'a' that has a 1 in that position
    // and one entry with an 'a' that has a 0 in that position in order to differentially solve key2
    for (auto it = in.begin(); it != in.end(); it++)
    {
        uint32_t a = it->a;

        for (size_t bit = 0; bit < 31; bit++)
        {
            if (a & 1)
            {
                if (!((ones_mask >> bit) & 1))  // No iter saved for this place yet
                {
                    // Save the iterator, mark as saved
                    input_iters[bit].one = it;
                    ones_mask |= ((uint32_t)1 << bit);

                    //Calculate differential term we'll need for solving
                    it->calc_diff_term();
                }
            }
            else
            {
                if (!((zeros_mask >> bit) & 1))  // No iter saved for this place yet
                {
                    // Save the iterator, mark as saved
                    input_iters[bit].zero = it;
                    zeros_mask |= ((uint32_t)1 << bit);

                    //Calculate differential term we'll need for solving key2
                    it->calc_diff_term();
                }
            }

            a >>= 1;
        }

        if ((ones_mask & zeros_mask) == 0x7fffffff) //Found all values needed to recover max bits
            break;
    }
}
