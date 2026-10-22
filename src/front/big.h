/* Big Nat arithmetic (S3 chunk B): a Nat at or above 2^64 as little-endian
   base 2^32 limbs in the arena. ADAPT of the orbit-lang u256 helpers to a
   variable limb count. No __int128 (tcc has none), no shift by 64 or more.

   A Big is normal when its top limb is not 0, except the number 0, which is
   the one limb 0. Each operation returns a normal Big. A result below 2^64
   has at most two limbs: the caller demotes it to a VAL_NAT (Q3 (a): one
   normal form per number). Each function returns 1, or 0 when the arena is
   exhausted. */
#ifndef LANG_FRONT_BIG_H
#define LANG_FRONT_BIG_H

#include <stddef.h>
#include <stdint.h>

#include "front/base.h"

typedef struct {
  const uint32_t *limbs;
  uint32_t count;
} Big;

/* The Big of a machine word. */
int big_from_u64(Arena *arena, uint64_t n, Big *out);
/* The Big of a span of LEN decimal digits (the parse_limbs of check.c). */
int big_parse(Arena *arena, const char *digits, size_t len, Big *out);
int big_add(Arena *arena, Big x, Big y, Big *out);
/* X - Y, or 0 when Y is larger (natSub truncates). */
int big_sub(Arena *arena, Big x, Big y, Big *out);
int big_mul(Arena *arena, Big x, Big y, Big *out);
/* -1, 0 or 1. */
int big_cmp(Big x, Big y);
/* 1 and *OUT when X is below 2^64. */
int big_fits_u64(Big x, uint64_t *out);
/* The decimal digits of X in the arena (repeated division by 10^9). */
int big_text(Arena *arena, Big x, const char **out, size_t *len);

#endif
