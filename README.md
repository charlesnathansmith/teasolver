# teasolver
This is a key solver for a TEA decryption variant of the following general form

```
for (size_t round = 32; round != 0; round--)
{
    b -= ((a << 4) + (key2 ^ a) + ((a >> 5) ^ sum) + key3);
    a -= ((b << 4) + (key0 ^ b) + ((b >> 5) ^ sum) + key1);

    sum -= delta;
}
```

Where a, b, and sum are 32-bit values that can be determined before and after each half-round.

This problem arose while reverse engineering a program known to use the routine above that was highly obfuscated and used blinding while calculating the half-rounds to the point that finding the operations involving the keys was extremely difficult, but shift operations were easily discoverable (eg. "shl b, 4" and "shr b, 5")

From these it was possible to recover the values of a and b before and after each half round, and to reason about the value of sum for that round.  If enough of these sets of values are recovered, it is possible to mathematically solve for most of the key bits, leaving only a handful of (generally 2-4) possible keys to try against the collected data.

## Solution method

Focusing on the first half-round,

```
b -= ((a << 4) + (key2 ^ a) + ((a >> 5) ^ sum) + key3)
```

Since b is known before and after, we will call the b value after the operation b_prime

```
b_prime = b - ((a << 4) + (key2 ^ a) + ((a >> 5) ^ sum) + key3)
```

Moving known values to the right and labeling them c, we arrive at the following form

```
(key2 ^ a) + key3 = b - b_prime - (a << 4) - ((a >> 5) ^ sum)
(key2 ^ a) + key3 = c
```

If we have data from two such half rounds (a0 and a1, b0 and b1, etc..) then we can substract to isolate key2

```
   (key2 ^ a1) + key3 = c1
- ((key2 ^ a0) + key3 = c0)
---------------------------
(key2 ^ a1) - (key2 ^ a0) = c1 - c0
(key2 ^ a1) - (key2 ^ a0) = C
```

If we were allowed to choose any values we wanted for a1 and a0, the maximum amount of bits of key2 would be recoverable when a1 = ~a2.
Say, for example, a1 = 0 and a0 = 0xffffffff, then the problem would reduce to

```
key2 - ~key2 = C
```

Labeling each bit of C as C[n] and of key2 as key2[n], where n = 0 is the least-significant bit, then by working through the binary subtraction

```
C[0] = key2[0] ^ ~key2[0] = 1
The borrow carried is ~(key2[0]) & ~key2[0] = ~key2[0]

C[1] = key2[1] ^ ~key2[1] ^ ~key2[0] = 1 ^ ~key2[0] = key2[0]
...
C[n] = key2[n-1]

(Since key2[n] ^ ~key2[n] is always 1, we never have to worry about borrows carrying through more than one bit's place)
```

So that overall,

```
C = (key2 << 1) + 1
key2 = C >> 1 
OR
key2 = (C >> 1) & 0x8000000
```
We could easily recover all but the most-significant bit.

In reality, we do not get to control what a0 and a1 are, but if we can find pairs of 'a' values that differ in a certain bit's places, find pairs like that for each bit's place, then we can use a similar method to solve for key2 bit by bit.

Say we find entries in our data set where a0 and a1 differ at bit n, and we'll call the value of a with 0 in that bit's place a0, so that a0[n] = 0 and a1[n] = 1, then

```
Borrow resulting from (key2[n] ^ a0[n]) - (key2[n] ^ a1[n]) = ~key[n]

Calculating next bit:
C[n+1] = a0[n+1] ^ a1[n+1] ^ ~key[n]

Rearranging:
~key[n] =   a0[n+1] ^ a1[n+1] ^ c[n+1]
key[n] = ~(a0[n+1] ^ a1[n+1] ^ c[n+1])
```

Since any two given values of a in our input data will typically differ from each other in multiple bits' places, it typically does not take many input entries to find the maximum number of key2 bits solvable (all but the MSB).  If an a0 and an a1 cannot be found such that a0[n] = ~a1[n] at a particular bit, then key[n] will not be solvable by this method, but from a handful of inputs we are usually able to find all but 1 - 2 bits, giving 2 - 4 candidate keys that can then be checked against the full input data set.

Each candidate key is plugged back into the equation we initially found for each entry

```
(key2 ^ a) + key3 = c
key3 = c - (key2 ^ a)
```

And if every input entry yields the same value of key3, then we know we have found a valid key2, key3 pair

The same procedure can be performed on the other half-round to solve for key0 and key1
