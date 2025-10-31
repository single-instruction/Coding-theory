/**
 * gf2.c - Binary field GF(2) operations implementation
 *
 * Simple but correct implementations for bit vectors and matrices over GF(2).
 * Optimization opportunities noted in comments for future work.
 */

#include "../include/gf2.h"
#include <stdlib.h>
#include <string.h>

/* Popcount lookup table for fast Hamming weight computation */
static const uint8_t popcount_table[256] = {
  0,1,1,2,1,2,2,3,1,2,2,3,2,3,3,4,1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  1,2,2,3,2,3,3,4,2,3,3,4,3,4,4,5,2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  2,3,3,4,3,4,4,5,3,4,4,5,4,5,5,6,3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,
  3,4,4,5,4,5,5,6,4,5,5,6,5,6,6,7,4,5,5,6,5,6,6,7,5,6,6,7,6,7,7,8
};

/* Vector operations */

int gf2_vec_init(gf2_vec *v, size_t n_bits) {
  if (!v) return -1;

  v->n_bits = n_bits;
  v->n_bytes = (n_bits + 7) / 8;
  v->bits = (uint8_t*)calloc(v->n_bytes, 1);

  if (!v->bits) {
    v->n_bits = 0;
    v->n_bytes = 0;
    return -1;
  }

  return 0;
}

void gf2_vec_free(gf2_vec *v) {
  if (v && v->bits) {
    free(v->bits);
    v->bits = NULL;
    v->n_bits = 0;
    v->n_bytes = 0;
  }
}

void gf2_vec_xor(gf2_vec *dst, const gf2_vec *src) {
  if (!dst || !src || dst->n_bytes != src->n_bytes) return;

  /* Optimization note: could use word-sized XOR (uint64_t) for better performance */
  for (size_t i = 0; i < dst->n_bytes; i++) {
    dst->bits[i] ^= src->bits[i];
  }
}

void gf2_vec_copy(gf2_vec *dst, const gf2_vec *src) {
  if (!dst || !src || dst->n_bytes != src->n_bytes) return;
  memcpy(dst->bits, src->bits, src->n_bytes);
}

void gf2_vec_zero(gf2_vec *v) {
  if (!v) return;
  memset(v->bits, 0, v->n_bytes);
}

int gf2_vec_dot(const gf2_vec *a, const gf2_vec *b) {
  if (!a || !b || a->n_bytes != b->n_bytes) return 0;

  /* Compute a AND b, then count 1-bits */
  int result = 0;
  for (size_t i = 0; i < a->n_bytes; i++) {
    uint8_t and_byte = a->bits[i] & b->bits[i];
    result ^= popcount_table[and_byte];
  }

  /* Result mod 2 */
  return result & 1;
}

size_t gf2_vec_weight(const gf2_vec *v) {
  if (!v) return 0;

  size_t weight = 0;

  /* Optimization note: could use POPCNT instruction on x86-64 if available */
  for (size_t i = 0; i < v->n_bytes; i++) {
    weight += popcount_table[v->bits[i]];
  }

  return weight;
}

/* Matrix operations */

int gf2_mat_init(gf2_mat *m, size_t n_rows, size_t n_cols) {
  if (!m) return -1;

  m->n_rows = n_rows;
  m->n_cols = n_cols;
  m->rows = (gf2_vec*)calloc(n_rows, sizeof(gf2_vec));

  if (!m->rows) {
    m->n_rows = 0;
    m->n_cols = 0;
    return -1;
  }

  /* Initialize each row */
  for (size_t i = 0; i < n_rows; i++) {
    if (gf2_vec_init(&m->rows[i], n_cols) != 0) {
      /* Cleanup on failure */
      for (size_t j = 0; j < i; j++) {
        gf2_vec_free(&m->rows[j]);
      }
      free(m->rows);
      m->rows = NULL;
      m->n_rows = 0;
      m->n_cols = 0;
      return -1;
    }
  }

  return 0;
}

void gf2_mat_free(gf2_mat *m) {
  if (!m) return;

  if (m->rows) {
    for (size_t i = 0; i < m->n_rows; i++) {
      gf2_vec_free(&m->rows[i]);
    }
    free(m->rows);
    m->rows = NULL;
  }

  m->n_rows = 0;
  m->n_cols = 0;
}

/**
 * Gaussian elimination over GF(2).
 * Converts matrix to row echelon form in-place.
 *
 * This is a straightforward implementation suitable for small to medium matrices.
 * For very large matrices (n > 10000), consider:
 * - Method of Four Russians (M4RI)
 * - Strassen-like divide and conquer
 * - GPU parallelization
 */
size_t gf2_mat_row_reduce(gf2_mat *m) {
  if (!m || !m->rows) return 0;

  size_t rank = 0;
  size_t pivot_col = 0;

  for (size_t pivot_row = 0; pivot_row < m->n_rows && pivot_col < m->n_cols; pivot_col++) {
    /* Find pivot */
    size_t found_pivot = pivot_row;
    int has_pivot = 0;

    for (size_t r = pivot_row; r < m->n_rows; r++) {
      if (gf2_vec_get(&m->rows[r], pivot_col)) {
        found_pivot = r;
        has_pivot = 1;
        break;
      }
    }

    if (!has_pivot) {
      continue; /* No pivot in this column, try next */
    }

    /* Swap rows if needed */
    if (found_pivot != pivot_row) {
      gf2_vec temp = m->rows[pivot_row];
      m->rows[pivot_row] = m->rows[found_pivot];
      m->rows[found_pivot] = temp;
    }

    /* Eliminate all other 1s in this column */
    for (size_t r = 0; r < m->n_rows; r++) {
      if (r != pivot_row && gf2_vec_get(&m->rows[r], pivot_col)) {
        gf2_vec_xor(&m->rows[r], &m->rows[pivot_row]);
      }
    }

    rank++;
    pivot_row++;
  }

  return rank;
}

void gf2_mat_mul_vec(gf2_vec *result, const gf2_mat *m, const gf2_vec *v) {
  if (!result || !m || !v) return;
  if (result->n_bits != m->n_rows || v->n_bits != m->n_cols) return;

  gf2_vec_zero(result);

  for (size_t i = 0; i < m->n_rows; i++) {
    int dot = gf2_vec_dot(&m->rows[i], v);
    gf2_vec_set(result, i, dot);
  }
}
