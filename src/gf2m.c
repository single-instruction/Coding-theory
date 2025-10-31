/**
 * gf2m.c - Galois field GF(2^m) arithmetic implementation
 *
 * Uses logarithm tables for efficient multiplication and inversion.
 * Tables are generated at runtime based on the irreducible polynomial.
 */

#include "../include/gf2m.h"
#include <stdlib.h>
#include <string.h>

gf2m_vtbl gf2m_backend;

/**
 * Multiply two polynomials in GF(2) modulo an irreducible polynomial.
 * Used during table generation.
 */
static uint16_t poly_mul_mod(uint16_t a, uint16_t b, uint16_t mod_poly, unsigned m) {
  uint16_t result = 0;
  uint16_t mask = (uint16_t)((1u << m) - 1u);

  while (b) {
    if (b & 1) {
      result ^= a;
    }
    b >>= 1;
    a <<= 1;
    if (a & (1u << m)) {
      a ^= mod_poly;
    }
  }

  return result & mask;
}

/**
 * Initialize GF(2^m) context by generating log/antilog tables.
 *
 * The algorithm:
 * 1. Start with primitive element (usually 0x02)
 * 2. Compute successive powers: α^0, α^1, α^2, ..., α^(2^m-2)
 * 3. Build antilog table: alog[i] = α^i
 * 4. Build log table: log[alog[i]] = i
 *
 * This gives us: alog[log[x]] = x for all nonzero x
 * And: x * y = alog[(log[x] + log[y]) mod (2^m - 1)]
 */
int gf2m_ctx_init(gf2m_ctx *ctx, unsigned m, uint16_t mod_poly) {
  if (!ctx) return -1;
  if (m < 2 || m > 16) return -1;

  size_t field_size = 1u << m;
  size_t order = field_size - 1; /* multiplicative group order */

  /* Allocate tables */
  ctx->m = m;
  ctx->mod_poly = mod_poly;
  ctx->prim = 2; /* standard primitive element */
  ctx->owns_tables = 1;

  ctx->alog = (uint16_t*)malloc(field_size * sizeof(uint16_t));
  ctx->log = (uint16_t*)malloc(field_size * sizeof(uint16_t));

  if (!ctx->alog || !ctx->log) {
    gf2m_ctx_free(ctx);
    return -1;
  }

  /* Initialize log table to sentinel values */
  memset(ctx->log, 0xff, field_size * sizeof(uint16_t));

  /* Generate antilog table by computing successive powers of primitive element */
  uint16_t x = 1;
  for (size_t i = 0; i < order; i++) {
    ctx->alog[i] = x;
    ctx->log[x] = (uint16_t)i;
    x = poly_mul_mod(x, ctx->prim, mod_poly, m);
  }

  /* Extend antilog table for wraparound (simplifies modulo operations) */
  for (size_t i = order; i < field_size; i++) {
    ctx->alog[i] = ctx->alog[i - order];
  }

  /* Verify we got a full cycle (primitive polynomial check) */
  if (x != 1) {
    gf2m_ctx_free(ctx);
    return -1; /* mod_poly is not primitive */
  }

  return 0;
}

void gf2m_ctx_free(gf2m_ctx *ctx) {
  if (!ctx) return;

  if (ctx->owns_tables) {
    if (ctx->alog) {
      free(ctx->alog);
      ctx->alog = NULL;
    }
    if (ctx->log) {
      free(ctx->log);
      ctx->log = NULL;
    }
  }

  ctx->m = 0;
  ctx->owns_tables = 0;
}

/* Core field operations */

uint16_t gf2m_mul_c(const gf2m_ctx *ctx, uint16_t a, uint16_t b) {
  if (!a || !b) return 0;
  unsigned la = ctx->log[a];
  unsigned lb = ctx->log[b];
  unsigned order = (1u << ctx->m) - 1u;
  return ctx->alog[(la + lb) % order];
}

uint16_t gf2m_inv_c(const gf2m_ctx *ctx, uint16_t a) {
  if (!a) return 0; /* undefined; caller must guard */
  unsigned la = ctx->log[a];
  unsigned order = (1u << ctx->m) - 1u;
  return ctx->alog[order - la];
}

uint16_t gf2m_sqr_c(const gf2m_ctx *ctx, uint16_t a) {
  /* Optimization note: squaring in characteristic 2 is linear (Frobenius map).
   * Could precompute a squaring table for O(1) performance. */
  return gf2m_mul_c(ctx, a, a);
}

uint16_t gf2m_mul(const gf2m_ctx *ctx, uint16_t a, uint16_t b) {
  return gf2m_backend.mul(ctx, a, b);
}

uint16_t gf2m_inv(const gf2m_ctx *ctx, uint16_t a) {
  return gf2m_backend.inv(ctx, a);
}

uint16_t gf2m_sqr(const gf2m_ctx *ctx, uint16_t a) {
  return gf2m_backend.sqr(ctx, a);
}

uint16_t gf2m_pow(const gf2m_ctx *ctx, uint16_t a, unsigned exp) {
  if (!a) return (exp == 0) ? 1 : 0;
  if (exp == 0) return 1;

  /* Square-and-multiply algorithm */
  uint16_t result = 1;
  uint16_t base = a;

  while (exp) {
    if (exp & 1) {
      result = gf2m_mul(ctx, result, base);
    }
    base = gf2m_sqr(ctx, base);
    exp >>= 1;
  }

  return result;
}

/* Initialize backend to C fallback on library load */
__attribute__((constructor))
static void init_backend(void) {
  gf2m_backend.mul = gf2m_mul_c;
  gf2m_backend.inv = gf2m_inv_c;
  gf2m_backend.sqr = gf2m_sqr_c;
}
