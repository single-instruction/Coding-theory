/**
 * gf2m.h - Galois field GF(2^m) arithmetic
 *
 * Implements finite field operations using logarithm tables.
 * Supports m from 2 to 16 (field sizes 4 to 65536).
 */

#pragma once
#include <stdint.h>

/**
 * GF(2^m) field context.
 * Contains precomputed log/antilog tables for efficient multiplication.
 *
 * The tables satisfy: alog[log[x]] = x for all nonzero x.
 * log[0] is undefined and must be guarded in all operations.
 */
typedef struct {
  unsigned m;           /* field extension degree (2 <= m <= 16) */
  uint16_t *alog;       /* antilog table, size 2^m */
  uint16_t *log;        /* log table, size 2^m */
  uint16_t prim;        /* primitive element (generator) */
  uint16_t mod_poly;    /* irreducible polynomial for field construction */
  int owns_tables;      /* 1 if tables were allocated by init, 0 if static */
} gf2m_ctx;

/**
 * Addition in GF(2^m) is just XOR (commutative and associative).
 */
static inline uint16_t gf2m_add(uint16_t a, uint16_t b) {
  return a ^ b;
}

/**
 * Initialize a GF(2^m) context with given parameters.
 *
 * @param ctx        Context to initialize
 * @param m          Field extension degree (2 <= m <= 16)
 * @param mod_poly   Irreducible polynomial (e.g., 0x11b for AES field GF(2^8))
 *                   Must be primitive for correct operation
 * @return 0 on success, -1 on failure (invalid params or allocation failure)
 *
 * The tables are dynamically allocated and must be freed with gf2m_ctx_free().
 */
int gf2m_ctx_init(gf2m_ctx *ctx, unsigned m, uint16_t mod_poly);

/**
 * Free resources associated with a field context.
 * Safe to call multiple times or on uninitialized contexts.
 */
void gf2m_ctx_free(gf2m_ctx *ctx);

/* Core field operations (C fallback; can be swapped by ASM backend) */

/**
 * Multiply two field elements using log/antilog tables.
 * Returns 0 if either operand is 0.
 */
uint16_t gf2m_mul(const gf2m_ctx *ctx, uint16_t a, uint16_t b);

/**
 * Multiplicative inverse of a field element.
 * Returns 0 if a is 0 (caller must guard against this).
 */
uint16_t gf2m_inv(const gf2m_ctx *ctx, uint16_t a);

/**
 * Square a field element.
 * Equivalent to gf2m_mul(ctx, a, a) but can be optimized.
 *
 * Optimization note: In characteristic 2, squaring is a linear operation
 * (Frobenius endomorphism). Can be implemented as table lookup for speed.
 */
uint16_t gf2m_sqr(const gf2m_ctx *ctx, uint16_t a);

/**
 * Raise a field element to an integer power.
 * Uses square-and-multiply for efficiency.
 */
uint16_t gf2m_pow(const gf2m_ctx *ctx, uint16_t a, unsigned exp);

/* Backend vtable for ASM acceleration */

typedef struct {
  uint16_t (*mul)(const gf2m_ctx*, uint16_t, uint16_t);
  uint16_t (*inv)(const gf2m_ctx*, uint16_t);
  uint16_t (*sqr)(const gf2m_ctx*, uint16_t);
} gf2m_vtbl;

extern gf2m_vtbl gf2m_backend;
