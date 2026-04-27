/*
 * math.c – custom integer math library for MiniOS.
 *
 * No <math.h>, no libc, no floating-point.
 * Every function operates on integers or size_t / unsigned long.
 */

#include "math.h"

/* ================================================================== */
/*  [SCALAR]  abs / min / max                                          */
/* ================================================================== */

int my_abs(int x) {
    return (x < 0) ? -x : x;
}

long my_labs(long x) {
    return (x < 0L) ? -x : x;
}

int my_min(int a, int b) {
    return (a < b) ? a : b;
}

int my_max(int a, int b) {
    return (a > b) ? a : b;
}

size_t my_min_sz(size_t a, size_t b) {
    return (a < b) ? a : b;
}

size_t my_max_sz(size_t a, size_t b) {
    return (a > b) ? a : b;
}

/* ================================================================== */
/*  [ALIGN]  Memory alignment                                          */
/* ================================================================== */

/*
 * my_align_up: round x up to the next multiple of `align`.
 * `align` MUST be a power of two.
 *
 * Algorithm: (x + align - 1) & ~(align - 1)
 * Works because (align - 1) is all-ones in the low bits for any pow2 align.
 */
size_t my_align_up(size_t x, size_t align) {
    if (align == 0) return x;
    return (x + align - 1u) & ~(align - 1u);
}

/*
 * my_align_down: round x down to the nearest multiple of `align`.
 * `align` MUST be a power of two.
 */
size_t my_align_down(size_t x, size_t align) {
    if (align == 0) return x;
    return x & ~(align - 1u);
}

/* ================================================================== */
/*  [POW2]  Power-of-two utilities                                     */
/* ================================================================== */

/*
 * my_is_pow2: returns 1 if x is a power of two.
 * Classic bit trick: a power of two has exactly one bit set,
 * so (x & (x-1)) == 0 for all such values (and x must be > 0).
 */
int my_is_pow2(size_t x) {
    return (x > 0u) && ((x & (x - 1u)) == 0u);
}

/*
 * my_next_pow2: smallest power of two >= x.
 * Uses repeated left-shift until the value covers x.
 * Special-cased: next_pow2(0) = 1, next_pow2(pow2) = same pow2.
 *
 * Optimised path: fill all lower bits via OR-shift, then add 1.
 * E.g. x = 0b0001_0011  →  fill → 0b0001_1111  →  +1 = 0b0010_0000
 */
size_t my_next_pow2(size_t x) {
    if (x == 0u) return 1u;
    if (my_is_pow2(x)) return x;
    x--;
    x |= x >> 1;
    x |= x >> 2;
    x |= x >> 4;
    x |= x >> 8;
    x |= x >> 16;
#if defined(__LP64__) || defined(_LP64) || (defined(__SIZEOF_SIZE_T__) && __SIZEOF_SIZE_T__ == 8)
    x |= x >> 32;
#endif
    return x + 1u;
}

/*
 * my_log2_floor: floor(log2(x)).
 * Finds the position of the highest set bit (0-indexed).
 * Returns -1 for x == 0.
 */
int my_log2_floor(size_t x) {
    int n = -1;
    while (x) { x >>= 1; n++; }
    return n;
}

/* ================================================================== */
/*  [BITS]  Bit-manipulation primitives                                */
/* ================================================================== */

/*
 * my_clz: count leading zeros in an unsigned long.
 * Result is undefined if x == 0 (matches GCC __builtin_clzl behaviour).
 *
 * Implementation: binary-search over bit positions.
 */
int my_clz(unsigned long x) {
    int n = 0;
    unsigned long bits = (unsigned long)sizeof(unsigned long) * 8UL;

    if (x == 0UL) return (int)bits;

#if defined(__LP64__) || defined(_LP64) || (defined(__SIZEOF_LONG__) && __SIZEOF_LONG__ == 8)
    if ((x & 0xFFFFFFFF00000000UL) == 0UL) { n += 32; x <<= 32; }
#endif
    if ((x & 0xFFFF000000000000UL) == 0UL) { n += 16; x <<= 16; }
    if ((x & 0xFF00000000000000UL) == 0UL) { n +=  8; x <<=  8; }
    if ((x & 0xF000000000000000UL) == 0UL) { n +=  4; x <<=  4; }
    if ((x & 0xC000000000000000UL) == 0UL) { n +=  2; x <<=  2; }
    if ((x & 0x8000000000000000UL) == 0UL) { n +=  1; }
    return n;
}

/*
 * my_ctz: count trailing zeros in an unsigned long.
 * Uses the classic bit-trick: (x & -x) isolates the lowest set bit;
 * then log2 gives its position.
 */
int my_ctz(unsigned long x) {
    unsigned long isolated;
    int n;

    if (x == 0UL) return (int)(sizeof(unsigned long) * 8UL);
    isolated = x & (unsigned long)(-(long)x);
    n = 0;
    while (isolated > 1UL) { isolated >>= 1; n++; }
    return n;
}

/*
 * my_popcount: number of set bits (Hamming weight).
 * Uses Brian Kernighan's algorithm: each iteration clears the lowest
 * set bit, so it runs in O(set bits) time.
 */
int my_popcount(unsigned long x) {
    int count = 0;
    while (x) { x &= x - 1UL; count++; }
    return count;
}

/* ================================================================== */
/*  [ARITH]  Arithmetic helpers                                        */
/* ================================================================== */

/*
 * my_sqrt_int: integer square root (floor(sqrt(x))).
 * Uses Newton-Raphson integer method which converges in O(log x) steps.
 */
size_t my_sqrt_int(size_t x) {
    size_t r;
    size_t r1;

    if (x == 0u) return 0u;
    if (x < 4u)  return 1u;

    /* Initial estimate: shift right by half the bit width */
    r = x >> (my_log2_floor(x) / 2);

    /* Newton-Raphson iterations: r_{n+1} = (r_n + x/r_n) / 2 */
    for (;;) {
        r1 = (r + x / r) / 2u;
        if (r1 >= r) break;
        r = r1;
    }
    return r;
}

/*
 * my_gcd: greatest common divisor via Euclidean algorithm.
 * Runs in O(log(min(a,b))) iterations.
 */
size_t my_gcd(size_t a, size_t b) {
    size_t t;
    while (b) { t = b; b = a % b; a = t; }
    return a;
}

/*
 * my_lcm: least common multiple.
 * lcm(a,b) = a / gcd(a,b) * b  (divide first to avoid overflow).
 */
size_t my_lcm(size_t a, size_t b) {
    size_t g;
    if (a == 0u || b == 0u) return 0u;
    g = my_gcd(a, b);
    return (a / g) * b;
}

/*
 * my_udiv / my_umod: safe unsigned division.
 * Returns 0 when the divisor is 0 instead of trapping.
 */
unsigned long my_udiv(unsigned long a, unsigned long b) {
    return (b == 0UL) ? 0UL : a / b;
}

unsigned long my_umod(unsigned long a, unsigned long b) {
    return (b == 0UL) ? 0UL : a % b;
}
