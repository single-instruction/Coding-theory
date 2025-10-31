/**
 * poly.c - Polynomial arithmetic implementation
 *
 * Implements operations for polynomials over GF(2) and GF(2^m).
 * Simple but correct algorithms suitable for error-correcting codes.
 */

#include "../include/poly.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/*
 * ============================================================================
 * GF(2) Polynomial Implementation
 * ============================================================================
 */

/* Helper: compute actual degree by finding highest nonzero coefficient */
static void poly_gf2_update_degree(poly_gf2_t *p) {
  int d = p->capacity - 1;
  while (d >= 0) {
    int word_idx = d / 64;
    int bit_idx = d % 64;
    if (p->coeff[word_idx] & (1ULL << bit_idx)) {
      break;
    }
    d--;
  }
  p->deg = d;
}

int poly_gf2_init(poly_gf2_t *p, int capacity) {
  if (!p || capacity < 0) return -1;

  p->capacity = capacity;
  p->deg = -1;
  int n_words = (capacity + 63) / 64;
  p->coeff = (uint64_t*)calloc((size_t)n_words, sizeof(uint64_t));

  if (!p->coeff) {
    p->capacity = 0;
    return -1;
  }

  return 0;
}

void poly_gf2_free(poly_gf2_t *p) {
  if (p && p->coeff) {
    free(p->coeff);
    p->coeff = NULL;
    p->capacity = 0;
    p->deg = -1;
  }
}

void poly_gf2_zero(poly_gf2_t *p) {
  if (!p) return;
  int n_words = (p->capacity + 63) / 64;
  memset(p->coeff, 0, (size_t)n_words * sizeof(uint64_t));
  p->deg = -1;
}

void poly_gf2_set_coeff(poly_gf2_t *p, int i, int value) {
  if (!p || i < 0 || i >= p->capacity) return;

  int word_idx = i / 64;
  int bit_idx = i % 64;

  if (value) {
    p->coeff[word_idx] |= (1ULL << bit_idx);
  } else {
    p->coeff[word_idx] &= ~(1ULL << bit_idx);
  }

  /* Update degree conservatively */
  if (value && i > p->deg) {
    p->deg = i;
  } else if (!value && i == p->deg) {
    poly_gf2_update_degree(p);
  }
}

int poly_gf2_get_coeff(const poly_gf2_t *p, int i) {
  if (!p || i < 0 || i >= p->capacity) return 0;

  int word_idx = i / 64;
  int bit_idx = i % 64;
  return (p->coeff[word_idx] >> bit_idx) & 1;
}

void poly_gf2_copy(poly_gf2_t *dst, const poly_gf2_t *src) {
  if (!dst || !src) return;

  poly_gf2_zero(dst);
  int max_deg = (src->deg < dst->capacity) ? src->deg : (dst->capacity - 1);

  for (int i = 0; i <= max_deg; i++) {
    if (poly_gf2_get_coeff(src, i)) {
      poly_gf2_set_coeff(dst, i, 1);
    }
  }
}

void poly_gf2_add(poly_gf2_t *result, const poly_gf2_t *a, const poly_gf2_t *b) {
  if (!result || !a || !b) return;

  poly_gf2_zero(result);
  int max_deg = (a->deg > b->deg) ? a->deg : b->deg;

  for (int i = 0; i <= max_deg && i < result->capacity; i++) {
    int ca = poly_gf2_get_coeff(a, i);
    int cb = poly_gf2_get_coeff(b, i);
    poly_gf2_set_coeff(result, i, ca ^ cb);
  }
}

void poly_gf2_mul(poly_gf2_t *result, const poly_gf2_t *a, const poly_gf2_t *b) {
  if (!result || !a || !b) return;

  poly_gf2_zero(result);

  /* Simple schoolbook multiplication.
   * Optimization note: for large polynomials, consider Karatsuba or FFT methods. */
  for (int i = 0; i <= a->deg && i < a->capacity; i++) {
    if (!poly_gf2_get_coeff(a, i)) continue;

    for (int j = 0; j <= b->deg && j < b->capacity; j++) {
      if (!poly_gf2_get_coeff(b, j)) continue;

      int k = i + j;
      if (k < result->capacity) {
        int old_val = poly_gf2_get_coeff(result, k);
        poly_gf2_set_coeff(result, k, old_val ^ 1);
      }
    }
  }
}

int poly_gf2_div_rem(poly_gf2_t *q, poly_gf2_t *r,
                      const poly_gf2_t *a, const poly_gf2_t *b) {
  if (!q || !r || !a || !b || b->deg < 0) return -1;

  poly_gf2_zero(q);
  poly_gf2_copy(r, a);

#ifdef DEBUG_POLY_DIV
  fprintf(stderr, "DIV: a.deg=%d, b.deg=%d, r.deg=%d\n", a->deg, b->deg, r->deg);
#endif

  int iter = 0;
  while (r->deg >= b->deg) {
    int shift = r->deg - b->deg;

#ifdef DEBUG_POLY_DIV
    fprintf(stderr, "  Iter %d: r.deg=%d, shift=%d\n", iter++, r->deg, shift);
#endif

    /* Subtract b shifted by 'shift' from r */
    for (int i = 0; i <= b->deg; i++) {
      if (poly_gf2_get_coeff(b, i)) {
        int pos = i + shift;
        int old_val = poly_gf2_get_coeff(r, pos);
        poly_gf2_set_coeff(r, pos, old_val ^ 1);
      }
    }

    /* Set quotient bit */
    poly_gf2_set_coeff(q, shift, 1);

    poly_gf2_update_degree(r);

#ifdef DEBUG_POLY_DIV
    fprintf(stderr, "    After update: r.deg=%d\n", r->deg);
#endif
  }

#ifdef DEBUG_POLY_DIV
  fprintf(stderr, "  Final: r.deg=%d\n", r->deg);
#endif

  return 0;
}

void poly_gf2_gcd(poly_gf2_t *result, const poly_gf2_t *a, const poly_gf2_t *b) {
  if (!result || !a || !b) return;

  /* Euclidean algorithm */
  poly_gf2_t u, v, temp_q, temp_r;
  poly_gf2_init(&u, a->capacity);
  poly_gf2_init(&v, b->capacity);
  poly_gf2_init(&temp_q, a->capacity);
  poly_gf2_init(&temp_r, a->capacity);

  poly_gf2_copy(&u, a);
  poly_gf2_copy(&v, b);

  while (v.deg >= 0) {
    poly_gf2_div_rem(&temp_q, &temp_r, &u, &v);
    poly_gf2_copy(&u, &v);
    poly_gf2_copy(&v, &temp_r);
  }

  poly_gf2_copy(result, &u);

  poly_gf2_free(&u);
  poly_gf2_free(&v);
  poly_gf2_free(&temp_q);
  poly_gf2_free(&temp_r);
}

/*
 * ============================================================================
 * GF(2^m) Polynomial Implementation
 * ============================================================================
 */

/* Helper: compute actual degree */
static void poly_gf2m_update_degree(poly_gf2m_t *p) {
  int d = p->capacity - 1;
  while (d >= 0 && p->coeff[d] == 0) {
    d--;
  }
  p->deg = d;
}

int poly_gf2m_init(poly_gf2m_t *p, const gf2m_ctx *ctx, int capacity) {
  if (!p || !ctx || capacity < 0) return -1;

  p->capacity = capacity;
  p->deg = -1;
  p->ctx = ctx;
  p->coeff = (uint16_t*)calloc((size_t)capacity, sizeof(uint16_t));

  if (!p->coeff) {
    p->capacity = 0;
    return -1;
  }

  return 0;
}

void poly_gf2m_free(poly_gf2m_t *p) {
  if (p && p->coeff) {
    free(p->coeff);
    p->coeff = NULL;
    p->capacity = 0;
    p->deg = -1;
    p->ctx = NULL;
  }
}

void poly_gf2m_zero(poly_gf2m_t *p) {
  if (!p) return;
  memset(p->coeff, 0, (size_t)p->capacity * sizeof(uint16_t));
  p->deg = -1;
}

void poly_gf2m_set_coeff(poly_gf2m_t *p, int i, uint16_t value) {
  if (!p || i < 0 || i >= p->capacity) return;

  p->coeff[i] = value;

  if (value && i > p->deg) {
    p->deg = i;
  } else if (!value && i == p->deg) {
    poly_gf2m_update_degree(p);
  }
}

uint16_t poly_gf2m_get_coeff(const poly_gf2m_t *p, int i) {
  if (!p || i < 0 || i >= p->capacity) return 0;
  return p->coeff[i];
}

void poly_gf2m_copy(poly_gf2m_t *dst, const poly_gf2m_t *src) {
  if (!dst || !src) return;

  poly_gf2m_zero(dst);
  int max_deg = (src->deg < dst->capacity) ? src->deg : (dst->capacity - 1);

  for (int i = 0; i <= max_deg; i++) {
    dst->coeff[i] = src->coeff[i];
  }

  dst->deg = (src->deg < dst->capacity) ? src->deg : (dst->capacity - 1);
}

void poly_gf2m_add(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *b) {
  if (!result || !a || !b || !result->ctx) return;

  poly_gf2m_zero(result);
  int max_deg = (a->deg > b->deg) ? a->deg : b->deg;

  for (int i = 0; i <= max_deg && i < result->capacity; i++) {
    uint16_t ca = poly_gf2m_get_coeff(a, i);
    uint16_t cb = poly_gf2m_get_coeff(b, i);
    result->coeff[i] = gf2m_add(ca, cb);
  }

  poly_gf2m_update_degree(result);
}

void poly_gf2m_mul(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *b) {
  if (!result || !a || !b || !result->ctx) return;

  poly_gf2m_zero(result);

  /* Schoolbook multiplication */
  for (int i = 0; i <= a->deg && i < a->capacity; i++) {
    uint16_t ca = a->coeff[i];
    if (!ca) continue;

    for (int j = 0; j <= b->deg && j < b->capacity; j++) {
      uint16_t cb = b->coeff[j];
      if (!cb) continue;

      int k = i + j;
      if (k < result->capacity) {
        uint16_t prod = gf2m_mul(result->ctx, ca, cb);
        result->coeff[k] = gf2m_add(result->coeff[k], prod);
      }
    }
  }

  poly_gf2m_update_degree(result);
}

int poly_gf2m_div_rem(poly_gf2m_t *q, poly_gf2m_t *r,
                       const poly_gf2m_t *a, const poly_gf2m_t *b) {
  if (!q || !r || !a || !b || b->deg < 0) return -1;

  poly_gf2m_zero(q);
  poly_gf2m_copy(r, a);

  const gf2m_ctx *ctx = b->ctx;

  while (r->deg >= b->deg) {
    int shift = r->deg - b->deg;
    uint16_t r_lead = r->coeff[r->deg];
    uint16_t b_lead = b->coeff[b->deg];

    if (!b_lead) return -1; /* Should not happen if deg is correct */

    uint16_t factor = gf2m_mul(ctx, r_lead, gf2m_inv(ctx, b_lead));

    /* Subtract factor * b * x^shift from r */
    for (int i = 0; i <= b->deg; i++) {
      uint16_t term = gf2m_mul(ctx, factor, b->coeff[i]);
      int pos = i + shift;
      r->coeff[pos] = gf2m_add(r->coeff[pos], term);
    }

    q->coeff[shift] = factor;

    poly_gf2m_update_degree(r);
  }

  poly_gf2m_update_degree(q);
  return 0;
}

int poly_gf2m_mod(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *m) {
  if (!result || !a || !m) return -1;

  poly_gf2m_t temp_q, temp_r;
  poly_gf2m_init(&temp_q, a->ctx, a->capacity);
  poly_gf2m_init(&temp_r, a->ctx, a->capacity);

  int ret = poly_gf2m_div_rem(&temp_q, &temp_r, a, m);
  if (ret == 0) {
    poly_gf2m_copy(result, &temp_r);
  }

  poly_gf2m_free(&temp_q);
  poly_gf2m_free(&temp_r);

  return ret;
}

void poly_gf2m_gcd(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *b) {
  if (!result || !a || !b) return;

  poly_gf2m_t u, v, temp_q, temp_r;
  poly_gf2m_init(&u, a->ctx, a->capacity);
  poly_gf2m_init(&v, a->ctx, b->capacity);
  poly_gf2m_init(&temp_q, a->ctx, a->capacity);
  poly_gf2m_init(&temp_r, a->ctx, a->capacity);

  poly_gf2m_copy(&u, a);
  poly_gf2m_copy(&v, b);

  while (v.deg >= 0) {
    poly_gf2m_div_rem(&temp_q, &temp_r, &u, &v);
    poly_gf2m_copy(&u, &v);
    poly_gf2m_copy(&v, &temp_r);
  }

  poly_gf2m_copy(result, &u);

  poly_gf2m_free(&u);
  poly_gf2m_free(&v);
  poly_gf2m_free(&temp_q);
  poly_gf2m_free(&temp_r);
}

uint16_t poly_gf2m_eval(const poly_gf2m_t *p, uint16_t x) {
  if (!p || p->deg < 0) return 0;

  /* Horner's method: p(x) = c0 + x(c1 + x(c2 + ... )) */
  uint16_t result = p->coeff[p->deg];

  for (int i = p->deg - 1; i >= 0; i--) {
    result = gf2m_mul(p->ctx, result, x);
    result = gf2m_add(result, p->coeff[i]);
  }

  return result;
}

void poly_gf2m_deriv(poly_gf2m_t *result, const poly_gf2m_t *p) {
  if (!result || !p) return;

  poly_gf2m_zero(result);

  /* In characteristic 2: d/dx(x^n) = n*x^(n-1) = x^(n-1) if n odd, 0 if n even */
  for (int i = 1; i <= p->deg && i - 1 < result->capacity; i++) {
    if (i & 1) { /* odd degree */
      result->coeff[i - 1] = p->coeff[i];
    }
  }

  poly_gf2m_update_degree(result);
}

int poly_gf2m_inv_mod(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *m) {
  if (!result || !a || !m || m->deg < 0) return -1;

  /* Extended Euclidean algorithm: find u such that a*u a 1 (mod m) */
  poly_gf2m_t r0, r1, s0, s1, temp_q, temp_r, temp_s, temp_prod;
  const gf2m_ctx *ctx = a->ctx;

  int cap = (a->capacity > m->capacity) ? a->capacity : m->capacity;
  cap = (cap > 2 * m->capacity) ? cap : (2 * m->capacity);

  poly_gf2m_init(&r0, ctx, cap);
  poly_gf2m_init(&r1, ctx, cap);
  poly_gf2m_init(&s0, ctx, cap);
  poly_gf2m_init(&s1, ctx, cap);
  poly_gf2m_init(&temp_q, ctx, cap);
  poly_gf2m_init(&temp_r, ctx, cap);
  poly_gf2m_init(&temp_s, ctx, cap);
  poly_gf2m_init(&temp_prod, ctx, cap);

  poly_gf2m_copy(&r0, m);
  poly_gf2m_copy(&r1, a);
  poly_gf2m_set_coeff(&s0, 0, 0); /* s0 = 0 */
  poly_gf2m_set_coeff(&s1, 0, 1); /* s1 = 1 */

  while (r1.deg >= 0) {
    poly_gf2m_div_rem(&temp_q, &temp_r, &r0, &r1);

    /* r_new = r0 - q * r1 (already in temp_r) */
    poly_gf2m_copy(&r0, &r1);
    poly_gf2m_copy(&r1, &temp_r);

    /* s_new = s0 - q * s1 */
    poly_gf2m_mul(&temp_prod, &temp_q, &s1);
    poly_gf2m_add(&temp_s, &s0, &temp_prod);
    poly_gf2m_copy(&s0, &s1);
    poly_gf2m_copy(&s1, &temp_s);
  }

  /* r0 should be constant (gcd) */
  if (r0.deg != 0 || r0.coeff[0] == 0) {
    /* Not coprime */
    poly_gf2m_free(&r0);
    poly_gf2m_free(&r1);
    poly_gf2m_free(&s0);
    poly_gf2m_free(&s1);
    poly_gf2m_free(&temp_q);
    poly_gf2m_free(&temp_r);
    poly_gf2m_free(&temp_s);
    poly_gf2m_free(&temp_prod);
    return -1;
  }

  /* Normalize s0 by dividing by gcd coefficient */
  uint16_t gcd_coeff = r0.coeff[0];
  uint16_t inv_gcd = gf2m_inv(ctx, gcd_coeff);

  poly_gf2m_zero(result);
  for (int i = 0; i <= s0.deg && i < result->capacity; i++) {
    result->coeff[i] = gf2m_mul(ctx, s0.coeff[i], inv_gcd);
  }
  poly_gf2m_update_degree(result);

  poly_gf2m_free(&r0);
  poly_gf2m_free(&r1);
  poly_gf2m_free(&s0);
  poly_gf2m_free(&s1);
  poly_gf2m_free(&temp_q);
  poly_gf2m_free(&temp_r);
  poly_gf2m_free(&temp_s);
  poly_gf2m_free(&temp_prod);

  return 0;
}
