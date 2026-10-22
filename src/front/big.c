/* Big Nat arithmetic (S3 chunk B). See big.h. Each product of two 32-bit
   limbs fits in 64 bits, so the loops need no wider type. */
#include <string.h>

#include "front/big.h"

static uint32_t *limbs_alloc(Arena *arena, uint32_t count) {
  uint32_t *limbs = arena_alloc(arena, (size_t)count * sizeof *limbs);
  if (limbs != NULL)
    memset(limbs, 0, (size_t)count * sizeof *limbs);
  return limbs;
}

/* The normal Big over LIMBS: the zero limbs at the top go. */
static void trim(const uint32_t *limbs, uint32_t count, Big *out) {
  while (count > 1u && limbs[count - 1u] == 0u)
    count--;
  out->limbs = limbs;
  out->count = count;
}

int big_from_u64(Arena *arena, uint64_t n, Big *out) {
  uint32_t *limbs = limbs_alloc(arena, 2u);
  if (limbs == NULL)
    return 0;
  limbs[0] = (uint32_t)n;
  limbs[1] = (uint32_t)(n >> 32);
  trim(limbs, 2u, out);
  return 1;
}

int big_parse(Arena *arena, const char *digits, size_t len, Big *out) {
  /* A span of LEN digits needs at most LEN / 9 + 2 limbs. */
  uint32_t *limbs = limbs_alloc(arena, (uint32_t)(len / 9u + 2u));
  uint32_t n = 1;
  size_t i;
  if (limbs == NULL)
    return 0;
  for (i = 0; i < len; i++) {
    uint64_t carry = (uint64_t)(digits[i] - '0');
    uint32_t j;
    for (j = 0; j < n; j++) {
      uint64_t p = (uint64_t)limbs[j] * 10u + carry;
      limbs[j] = (uint32_t)p;
      carry = p >> 32;
    }
    if (carry != 0u)
      limbs[n++] = (uint32_t)carry;
  }
  trim(limbs, n, out);
  return 1;
}

int big_cmp(Big x, Big y) {
  uint32_t i;
  if (x.count != y.count)
    return x.count < y.count ? -1 : 1;
  for (i = x.count; i > 0u; i--) {
    if (x.limbs[i - 1u] != y.limbs[i - 1u])
      return x.limbs[i - 1u] < y.limbs[i - 1u] ? -1 : 1;
  }
  return 0;
}

int big_fits_u64(Big x, uint64_t *out) {
  if (x.count > 2u)
    return 0;
  *out = (uint64_t)x.limbs[0] | (x.count == 2u ? (uint64_t)x.limbs[1] << 32 : 0u);
  return 1;
}

int big_add(Arena *arena, Big x, Big y, Big *out) {
  uint32_t n = x.count > y.count ? x.count : y.count;
  uint32_t *r = limbs_alloc(arena, n + 1u);
  uint64_t carry = 0;
  uint32_t i;
  if (r == NULL)
    return 0;
  for (i = 0; i < n; i++) {
    uint64_t s = carry + (i < x.count ? x.limbs[i] : 0u) + (i < y.count ? y.limbs[i] : 0u);
    r[i] = (uint32_t)s;
    carry = s >> 32;
  }
  r[n] = (uint32_t)carry;
  trim(r, n + 1u, out);
  return 1;
}

int big_sub(Arena *arena, Big x, Big y, Big *out) {
  uint32_t *r;
  uint64_t borrow = 0;
  uint32_t i;
  if (big_cmp(x, y) < 0)
    return big_from_u64(arena, 0u, out);
  r = limbs_alloc(arena, x.count);
  if (r == NULL)
    return 0;
  for (i = 0; i < x.count; i++) {
    uint64_t sub = borrow + (i < y.count ? y.limbs[i] : 0u);
    uint64_t d = (uint64_t)x.limbs[i] - sub;
    r[i] = (uint32_t)d;
    borrow = (uint64_t)x.limbs[i] < sub ? 1u : 0u;
  }
  trim(r, x.count, out);
  return 1;
}

int big_mul(Arena *arena, Big x, Big y, Big *out) {
  uint32_t *r = limbs_alloc(arena, x.count + y.count);
  uint32_t i;
  uint32_t j;
  if (r == NULL)
    return 0;
  for (i = 0; i < x.count; i++) {
    uint64_t carry = 0;
    for (j = 0; j < y.count; j++) {
      /* At most (2^32 - 1)^2 + 2 (2^32 - 1) = 2^64 - 1. */
      uint64_t p = (uint64_t)x.limbs[i] * y.limbs[j] + r[i + j] + carry;
      r[i + j] = (uint32_t)p;
      carry = p >> 32;
    }
    r[i + y.count] = (uint32_t)carry;
  }
  trim(r, x.count + y.count, out);
  return 1;
}

/* X = X / D in place over COUNT limbs. Returns X % D. D is not 0. */
static uint32_t div_small(uint32_t *x, uint32_t count, uint32_t d) {
  uint64_t rem = 0;
  uint32_t i;
  for (i = count; i > 0u; i--) {
    uint64_t cur = (rem << 32) | x[i - 1u];
    x[i - 1u] = (uint32_t)(cur / d);
    rem = cur % d;
  }
  return (uint32_t)rem;
}

int big_text(Arena *arena, Big x, const char **out, size_t *len) {
  /* Each limb adds under 10 decimal digits. */
  size_t cap = (size_t)x.count * 10u + 2u;
  char *text = arena_alloc(arena, cap);
  uint32_t *work = limbs_alloc(arena, x.count);
  uint32_t n = x.count;
  size_t at = cap - 1u;
  if (text == NULL || work == NULL)
    return 0;
  memcpy(work, x.limbs, (size_t)x.count * sizeof *work);
  text[at] = '\0';
  do {
    uint32_t group = div_small(work, n, 1000000000u);
    int digits;
    while (n > 1u && work[n - 1u] == 0u)
      n--;
    /* The last group has no leading zeros; each other group has 9 digits. */
    for (digits = 0; digits < 9; digits++) {
      text[--at] = (char)('0' + group % 10u);
      group /= 10u;
      if (group == 0u && n == 1u && work[0] == 0u)
        break;
    }
  } while (n > 1u || work[0] != 0u);
  *out = text + at;
  *len = cap - 1u - at;
  return 1;
}
