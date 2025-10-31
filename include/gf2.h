/**
 * gf2.h - Binary field GF(2) operations
 *
 * Provides bit-level operations and matrix arithmetic over GF(2).
 * Used for linear algebra in error-correcting codes.
 */

#pragma once
#include <stdint.h>
#include <stddef.h>
#include <string.h>

/**
 * Bitset representation for GF(2) vectors and matrices.
 * Stores bits in little-endian byte order.
 *
 * Example: bits[0] & 1 is the first bit, bits[0] & 2 is the second, etc.
 */
typedef struct {
  uint8_t *bits;
  size_t n_bits;   /* number of meaningful bits */
  size_t n_bytes;  /* allocated bytes (at least ceil(n_bits/8)) */
} gf2_vec;

/**
 * Matrix over GF(2) stored in row-major order.
 * Each row is a gf2_vec of length n_cols.
 */
typedef struct {
  gf2_vec *rows;
  size_t n_rows;
  size_t n_cols;
} gf2_mat;

/* Vector operations */

/**
 * Initialize a GF(2) vector with n_bits capacity.
 * Returns 0 on success, -1 on allocation failure.
 */
int gf2_vec_init(gf2_vec *v, size_t n_bits);

/**
 * Free resources associated with a vector.
 */
void gf2_vec_free(gf2_vec *v);

/**
 * Set bit at position i (0-indexed) to value b (0 or 1).
 */
static inline void gf2_vec_set(gf2_vec *v, size_t i, int b) {
  if (i >= v->n_bits) return;
  size_t byte_idx = i / 8;
  size_t bit_idx = i % 8;
  if (b) {
    v->bits[byte_idx] |= (uint8_t)(1u << bit_idx);
  } else {
    v->bits[byte_idx] &= (uint8_t)(~(1u << bit_idx));
  }
}

/**
 * Get bit at position i (0-indexed).
 * Returns 0 or 1, or 0 if out of bounds.
 */
static inline int gf2_vec_get(const gf2_vec *v, size_t i) {
  if (i >= v->n_bits) return 0;
  size_t byte_idx = i / 8;
  size_t bit_idx = i % 8;
  return (v->bits[byte_idx] >> bit_idx) & 1;
}

/**
 * XOR two vectors: dst = dst XOR src.
 * Both vectors must have the same n_bits.
 */
void gf2_vec_xor(gf2_vec *dst, const gf2_vec *src);

/**
 * Copy src to dst. Both must be initialized with same n_bits.
 */
void gf2_vec_copy(gf2_vec *dst, const gf2_vec *src);

/**
 * Set all bits to zero.
 */
void gf2_vec_zero(gf2_vec *v);

/**
 * Compute dot product (inner product) of two vectors over GF(2).
 * Returns 0 or 1.
 */
int gf2_vec_dot(const gf2_vec *a, const gf2_vec *b);

/**
 * Count number of 1 bits (Hamming weight).
 */
size_t gf2_vec_weight(const gf2_vec *v);

/* Matrix operations */

/**
 * Initialize a GF(2) matrix with given dimensions.
 * Returns 0 on success, -1 on allocation failure.
 */
int gf2_mat_init(gf2_mat *m, size_t n_rows, size_t n_cols);

/**
 * Free resources associated with a matrix.
 */
void gf2_mat_free(gf2_mat *m);

/**
 * Perform Gaussian elimination to row-reduce a matrix.
 * Operates in-place, returns rank.
 *
 * This is a simple but correct implementation. For large matrices,
 * consider block-wise algorithms or Strassen-like methods.
 */
size_t gf2_mat_row_reduce(gf2_mat *m);

/**
 * Multiply matrix by vector: result = m * v.
 * result must be initialized to n_rows bits.
 * v must have n_cols bits.
 */
void gf2_mat_mul_vec(gf2_vec *result, const gf2_mat *m, const gf2_vec *v);

/**
 * Get element at (row, col).
 * Returns 0 or 1.
 */
static inline int gf2_mat_get(const gf2_mat *m, size_t row, size_t col) {
  if (row >= m->n_rows) return 0;
  return gf2_vec_get(&m->rows[row], col);
}

/**
 * Set element at (row, col) to b (0 or 1).
 */
static inline void gf2_mat_set(gf2_mat *m, size_t row, size_t col, int b) {
  if (row >= m->n_rows) return;
  gf2_vec_set(&m->rows[row], col, b);
}
