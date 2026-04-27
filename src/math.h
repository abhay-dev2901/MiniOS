#ifndef MATH_H
#define MATH_H

/*
 * math.h – custom integer math library for MiniOS.
 *
 * No floating-point, no <math.h>, no libc.
 * All operations are purely integer / bitwise.
 *
 * Categories
 * ----------
 *  [SCALAR]   abs, min, max
 *  [ALIGN]    align_up, align_down          – memory-alignment helpers
 *  [POW2]     is_pow2, next_pow2, log2_floor – allocator / VFS helpers
 *  [BITS]     clz, ctz, popcount            – bit-manipulation primitives
 *  [ARITH]    sqrt_int, gcd, udiv, umod     – arithmetic without libgcc dep
 */

#include "minios_types.h"

/* ------------------------------------------------------------------ */
/*  [SCALAR]  Absolute value, min, max                                 */
/* ------------------------------------------------------------------ */

int    my_abs(int x);
long   my_labs(long x);

int    my_min(int a, int b);
int    my_max(int a, int b);
size_t my_min_sz(size_t a, size_t b);
size_t my_max_sz(size_t a, size_t b);

/* ------------------------------------------------------------------ */
/*  [ALIGN]  Memory alignment helpers                                  */
/* ------------------------------------------------------------------ */

/* Round x UP to the nearest multiple of align (align must be pow2). */
size_t my_align_up(size_t x, size_t align);

/* Round x DOWN to the nearest multiple of align (align must be pow2). */
size_t my_align_down(size_t x, size_t align);

/* ------------------------------------------------------------------ */
/*  [POW2]  Power-of-two utilities                                     */
/* ------------------------------------------------------------------ */

/* Returns 1 if x is a power of two (and x > 0). */
int    my_is_pow2(size_t x);

/* Returns the smallest power of two >= x.  Returns 1 if x == 0. */
size_t my_next_pow2(size_t x);

/* Returns floor(log2(x)).  Returns -1 if x == 0. */
int    my_log2_floor(size_t x);

/* ------------------------------------------------------------------ */
/*  [BITS]  Bit-manipulation primitives                                */
/* ------------------------------------------------------------------ */

/* Count leading zeros in an unsigned long (result undefined for x==0). */
int my_clz(unsigned long x);

/* Count trailing zeros in an unsigned long (result undefined for x==0). */
int my_ctz(unsigned long x);

/* Count number of set bits (population count). */
int my_popcount(unsigned long x);

/* ------------------------------------------------------------------ */
/*  [ARITH]  Arithmetic helpers                                        */
/* ------------------------------------------------------------------ */

/* Integer square root: floor(sqrt(x)). */
size_t my_sqrt_int(size_t x);

/* Greatest common divisor (Euclidean algorithm). */
size_t my_gcd(size_t a, size_t b);

/* Least common multiple. */
size_t my_lcm(size_t a, size_t b);

/* Unsigned divide a/b; returns 0 if b==0. */
unsigned long my_udiv(unsigned long a, unsigned long b);

/* Unsigned modulo a%b; returns 0 if b==0. */
unsigned long my_umod(unsigned long a, unsigned long b);

#endif /* MATH_H */
