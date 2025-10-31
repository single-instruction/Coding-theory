/**
 * poly.h - Polynomial arithmetic over GF(2) and GF(2^m)
 *
 * Provides two separate polynomial types for type safety:
 * - poly_gf2_t:  Polynomials with binary coefficients (GF(2))
 * - poly_gf2m_t: Polynomials with GF(2^m) coefficients
 *
 * Used in BCH and Goppa code construction and decoding.
 */

#pragma once
#include "gf2m.h"
#include <stdint.h>
#include <stddef.h>

/*
 * ============================================================================
 * Polynomials over GF(2) - Binary coefficients
 * ============================================================================
 */

/**
 * Polynomial over GF(2).
 * Coefficients stored as bits in little-endian order.
 * coeff[0] is the constant term, coeff[deg] is the leading coefficient.
 */
typedef struct {
  uint64_t *coeff;  /* bit-packed coefficients: each uint64_t holds 64 coeffs */
  int deg;          /* degree (-1 for zero polynomial) */
  int capacity;     /* allocated capacity in bits */
} poly_gf2_t;

/**
 * Initialize a GF(2) polynomial with given capacity.
 * Returns 0 on success, -1 on failure.
 */
int poly_gf2_init(poly_gf2_t *p, int capacity);

/**
 * Free resources associated with polynomial.
 */
void poly_gf2_free(poly_gf2_t *p);

/**
 * Set polynomial to zero.
 */
void poly_gf2_zero(poly_gf2_t *p);

/**
 * Set coefficient at degree i to value (0 or 1).
 * Automatically adjusts degree if needed.
 */
void poly_gf2_set_coeff(poly_gf2_t *p, int i, int value);

/**
 * Get coefficient at degree i (0 or 1).
 */
int poly_gf2_get_coeff(const poly_gf2_t *p, int i);

/**
 * Copy src to dst. Both must be initialized.
 */
void poly_gf2_copy(poly_gf2_t *dst, const poly_gf2_t *src);

/**
 * Add (XOR) two polynomials: result = a + b.
 */
void poly_gf2_add(poly_gf2_t *result, const poly_gf2_t *a, const poly_gf2_t *b);

/**
 * Multiply two polynomials: result = a * b.
 * Simple schoolbook multiplication. For large degrees, consider Karatsuba or FFT.
 */
void poly_gf2_mul(poly_gf2_t *result, const poly_gf2_t *a, const poly_gf2_t *b);

/**
 * Divide polynomials with remainder: a = q * b + r.
 * Returns 0 on success, -1 if b is zero.
 */
int poly_gf2_div_rem(poly_gf2_t *q, poly_gf2_t *r, const poly_gf2_t *a, const poly_gf2_t *b);

/**
 * Compute GCD of two polynomials using Euclidean algorithm.
 */
void poly_gf2_gcd(poly_gf2_t *result, const poly_gf2_t *a, const poly_gf2_t *b);

/*
 * ============================================================================
 * Polynomials over GF(2^m) - Field element coefficients
 * ============================================================================
 */

/**
 * Polynomial over GF(2^m).
 * Coefficients are field elements (uint16_t).
 */
typedef struct {
  uint16_t *coeff;    /* coefficients: coeff[0] is constant term */
  int deg;            /* degree (-1 for zero polynomial) */
  int capacity;       /* allocated capacity */
  const gf2m_ctx *ctx; /* field context (borrowed reference) */
} poly_gf2m_t;

/**
 * Initialize a GF(2^m) polynomial with given field context and capacity.
 * The context is borrowed (not owned) and must outlive the polynomial.
 * Returns 0 on success, -1 on failure.
 */
int poly_gf2m_init(poly_gf2m_t *p, const gf2m_ctx *ctx, int capacity);

/**
 * Free resources associated with polynomial.
 */
void poly_gf2m_free(poly_gf2m_t *p);

/**
 * Set polynomial to zero.
 */
void poly_gf2m_zero(poly_gf2m_t *p);

/**
 * Set coefficient at degree i to value (field element).
 */
void poly_gf2m_set_coeff(poly_gf2m_t *p, int i, uint16_t value);

/**
 * Get coefficient at degree i.
 */
uint16_t poly_gf2m_get_coeff(const poly_gf2m_t *p, int i);

/**
 * Copy src to dst. Both must be initialized with same field.
 */
void poly_gf2m_copy(poly_gf2m_t *dst, const poly_gf2m_t *src);

/**
 * Add two polynomials: result = a + b (coefficient-wise).
 */
void poly_gf2m_add(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *b);

/**
 * Multiply two polynomials: result = a * b.
 * Simple schoolbook multiplication.
 */
void poly_gf2m_mul(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *b);

/**
 * Divide polynomials with remainder: a = q * b + r.
 * Returns 0 on success, -1 if b is zero.
 */
int poly_gf2m_div_rem(poly_gf2m_t *q, poly_gf2m_t *r,
                       const poly_gf2m_t *a, const poly_gf2m_t *b);

/**
 * Compute polynomial modulo another: result = a mod m.
 * Returns 0 on success, -1 if m is zero.
 */
int poly_gf2m_mod(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *m);

/**
 * Compute GCD of two polynomials using Euclidean algorithm.
 */
void poly_gf2m_gcd(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *b);

/**
 * Evaluate polynomial at a point x in the field.
 * Uses Horner's method for efficiency.
 */
uint16_t poly_gf2m_eval(const poly_gf2m_t *p, uint16_t x);

/**
 * Compute formal derivative of polynomial.
 * In characteristic 2, derivative of x^n is x^(n-1) if n is odd, 0 if n is even.
 */
void poly_gf2m_deriv(poly_gf2m_t *result, const poly_gf2m_t *p);

/**
 * Compute modular inverse: result * a a 1 (mod m).
 * Uses extended Euclidean algorithm.
 * Returns 0 on success, -1 if a and m are not coprime.
 */
int poly_gf2m_inv_mod(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *m);
