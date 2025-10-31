/**
 * bch.c - BCH (Bose-Chaudhuri-Hocquenghem) error-correcting codes
 *
 * Implements BCH codes over GF(2) with t-error correction capability.
 * Uses:
 * - Generator polynomial from minimal polynomials
 * - Systematic encoding via polynomial division
 * - Berlekamp-Massey algorithm for decoding
 * - Chien search for error location
 */

#include "../include/bch.h"
#include "../include/gf2m.h"
#include "../include/poly.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/**
 * Compute the minimal polynomial of α^i over GF(2).
 * The minimal polynomial is the smallest monic polynomial m(x) such that m(α^i) = 0.
 *
 * For binary fields, the minimal polynomial of β has roots β, β^2, β^4, ..., β^(2^k)
 * where k is the smallest integer such that β^(2^(k+1)) = β.
 *
 * This is the polynomial: m(x) = (x - β)(x - β^2)(x - β^4)...(x - β^(2^k))
 */
static void compute_minimal_poly(poly_gf2m_t *result, const gf2m_ctx *ctx, int i) {
  unsigned m = ctx->m;
  unsigned order = (1U << m) - 1;

  /* Track which conjugates we've seen */
  uint8_t *seen = (uint8_t*)calloc(order + 1, sizeof(uint8_t));
  if (!seen) return;

  /* Start with m(x) = 1 */
  poly_gf2m_zero(result);
  poly_gf2m_set_coeff(result, 0, 1);

  /* Current element to process */
  unsigned current = (unsigned)i % order;

  while (!seen[current]) {
    seen[current] = 1;

    /* Multiply m(x) by (x - α^current) */
    /* We need to multiply: result(x) * (x - β) where β = α^current */
    poly_gf2m_t factor, temp;
    poly_gf2m_init(&factor, ctx, 2);
    poly_gf2m_init(&temp, ctx, result->deg + 2);

    /* factor = x - β = x + β (in GF(2^m), - is +) */
    uint16_t beta = ctx->alog[current];
    poly_gf2m_set_coeff(&factor, 0, beta);  /* constant term */
    poly_gf2m_set_coeff(&factor, 1, 1);     /* x term */

    /* temp = result * factor */
    poly_gf2m_mul(&temp, result, &factor);
    poly_gf2m_copy(result, &temp);

    poly_gf2m_free(&factor);
    poly_gf2m_free(&temp);

    /* Next conjugate is current * 2 (mod order) */
    current = (current * 2) % order;
    if (current == 0) current = order;  /* Avoid 0, wrap to order */
  }

  free(seen);
}

/**
 * Compute LCM of two polynomials: lcm(a, b) = (a * b) / gcd(a, b)
 */
static void poly_gf2m_lcm(poly_gf2m_t *result, const poly_gf2m_t *a, const poly_gf2m_t *b) {
  if (a->deg < 0 || b->deg < 0) {
    poly_gf2m_zero(result);
    return;
  }

  /* Compute gcd(a, b) */
  poly_gf2m_t gcd_poly, product, quotient, remainder;
  poly_gf2m_init(&gcd_poly, a->ctx, a->deg + b->deg + 1);
  poly_gf2m_init(&product, a->ctx, a->deg + b->deg + 1);
  poly_gf2m_init(&quotient, a->ctx, a->deg + b->deg + 1);
  poly_gf2m_init(&remainder, a->ctx, a->deg + b->deg + 1);

  poly_gf2m_gcd(&gcd_poly, a, b);

  /* Compute a * b */
  poly_gf2m_mul(&product, a, b);

  /* Divide by gcd */
  if (gcd_poly.deg >= 0) {
    poly_gf2m_div_rem(&quotient, &remainder, &product, &gcd_poly);
    poly_gf2m_copy(result, &quotient);
  } else {
    poly_gf2m_zero(result);
  }

  poly_gf2m_free(&gcd_poly);
  poly_gf2m_free(&product);
  poly_gf2m_free(&quotient);
  poly_gf2m_free(&remainder);
}

/**
 * Build BCH generator polynomial g(x).
 * g(x) = lcm(m_1(x), m_3(x), ..., m_{2t-1}(x))
 * where m_i(x) is the minimal polynomial of α^i.
 */
static int build_generator(poly_gf2m_t *g, const gf2m_ctx *ctx, unsigned t) {
  /* Start with g(x) = m_1(x) */
  compute_minimal_poly(g, ctx, 1);

  /* LCM with m_3, m_5, ..., m_{2t-1} */
  for (unsigned i = 3; i < 2 * t; i += 2) {
    poly_gf2m_t m_i, new_g;
    poly_gf2m_init(&m_i, ctx, 20);  /* Minimal polys are typically small */
    poly_gf2m_init(&new_g, ctx, g->deg + 20);

    compute_minimal_poly(&m_i, ctx, (int)i);
    poly_gf2m_lcm(&new_g, g, &m_i);
    poly_gf2m_copy(g, &new_g);

    poly_gf2m_free(&m_i);
    poly_gf2m_free(&new_g);
  }

  return 0;
}


/**
 * BCH Encoder
 *
 * Systematic encoding: codeword = [message | parity]
 * 1. Compute g(x) = generator polynomial
 * 2. Compute parity = remainder of (x^r * m(x)) / g(x), where r = deg(g)
 * 3. Output: message || parity
 */
static codectk_err bch_encode(const void *pp, const uint8_t *in, size_t in_bits,
                              uint8_t *out, size_t *out_bits) {
  const bch_params *P = (const bch_params*)pp;

  if (!P || !in || !out || !out_bits) return CODECTK_EINVAL;
  if (P->m < 2 || P->m > 16 || P->t == 0) return CODECTK_EINVAL;

  /* Initialize field context */
  gf2m_ctx ctx;

  /* Use standard primitive polynomials for common field sizes */
  uint16_t prim_polys[] = {
    0, 0, 0x7, 0xB, 0x13, 0x25, 0x43, 0x89,
    0x11D, 0x211, 0x409, 0x805, 0x1053, 0x201B, 0x4443, 0x8003, 0x100B
  };

  if (P->m >= sizeof(prim_polys) / sizeof(prim_polys[0])) {
    return CODECTK_EINVAL;
  }

  if (gf2m_ctx_init(&ctx, P->m, prim_polys[P->m]) != 0) {
    return CODECTK_ENOMEM;
  }

  /* Build generator polynomial g(x) */
  poly_gf2m_t g;
  poly_gf2m_init(&g, &ctx, 100);  /* Conservative capacity */
  build_generator(&g, &ctx, P->t);

  int r = g.deg;  /* Degree of generator = number of parity bits */
  unsigned n = (1U << P->m) - 1;  /* Code length */
  unsigned k = n - (unsigned)r;   /* Message length */

#ifdef DEBUG_BCH_ENCODER
  fprintf(stderr, "Generator deg=%d, n=%u, k=%u, r=%d\n", g.deg, n, k, r);
  fprintf(stderr, "Generator coeffs (GF2m): ");
  for (int i = 0; i <= g.deg; i++) {
    fprintf(stderr, "%u ", poly_gf2m_get_coeff(&g, i));
  }
  fprintf(stderr, "\n");
#endif

  /* Check input size */
  if (in_bits > k) {
    poly_gf2m_free(&g);
    gf2m_ctx_free(&ctx);
    return CODECTK_EINVAL;
  }

  /* Convert generator to GF(2) polynomial */
  poly_gf2_t g_gf2;
  poly_gf2_init(&g_gf2, r + 1);

  /* For binary BCH, g(x) has coefficients in {0, 1} */
  /* Extract just the GF(2) representation */
  for (int i = 0; i <= g.deg; i++) {
    uint16_t c = poly_gf2m_get_coeff(&g, i);
    poly_gf2_set_coeff(&g_gf2, i, c != 0 ? 1 : 0);
  }

#ifdef DEBUG_BCH_ENCODER
  fprintf(stderr, "Generator coeffs (GF2): ");
  for (int i = 0; i <= g_gf2.deg; i++) {
    fprintf(stderr, "%d ", poly_gf2_get_coeff(&g_gf2, i));
  }
  fprintf(stderr, "\n");
#endif

  /* Build message polynomial m(x) from input bits */
  poly_gf2_t m;
  poly_gf2_init(&m, (int)k);

  for (size_t i = 0; i < in_bits; i++) {
    int byte_idx = (int)(i / 8);
    int bit_idx = (int)(i % 8);
    int bit = (in[byte_idx] >> bit_idx) & 1;
    poly_gf2_set_coeff(&m, (int)i, bit);
  }

  /* Compute x^r * m(x) */
  poly_gf2_t shifted;
  poly_gf2_init(&shifted, (int)(k + (unsigned)r));

#ifdef DEBUG_BCH_ENCODER
  fprintf(stderr, "Message m deg=%d: ", m.deg);
  for (int i = 0; i <= m.deg; i++) {
    fprintf(stderr, "%d", poly_gf2_get_coeff(&m, i));
  }
  fprintf(stderr, "\n");
#endif

  for (int i = 0; i <= m.deg; i++) {
    if (poly_gf2_get_coeff(&m, i)) {
      poly_gf2_set_coeff(&shifted, i + r, 1);
    }
  }

  /* Compute remainder: parity = (x^r * m(x)) mod g(x) */
  poly_gf2_t quotient, parity;
  poly_gf2_init(&quotient, (int)k);
  poly_gf2_init(&parity, (int)(k + (unsigned)r));  /* Need capacity for full shifted poly */

#ifdef DEBUG_BCH_ENCODER
  fprintf(stderr, "Shifted deg=%d, g_gf2 deg=%d\n", shifted.deg, g_gf2.deg);
  fprintf(stderr, "Shifted: ");
  for (int i = 0; i <= shifted.deg && i < 20; i++) {
    fprintf(stderr, "%d", poly_gf2_get_coeff(&shifted, i));
  }
  fprintf(stderr, "\n");
#endif

  poly_gf2_div_rem(&quotient, &parity, &shifted, &g_gf2);

#ifdef DEBUG_BCH_ENCODER
  fprintf(stderr, "Parity deg=%d: ", parity.deg);
  for (int i = 0; i <= parity.deg; i++) {
    fprintf(stderr, "%d", poly_gf2_get_coeff(&parity, i));
  }
  fprintf(stderr, "\n");
#endif

  /* Write output: message bits followed by parity bits */
  size_t total_bits = in_bits + (size_t)r;
  size_t total_bytes = (total_bits + 7) / 8;

  if (total_bytes > (*out_bits) / 8) {
    poly_gf2_free(&g_gf2);
    poly_gf2_free(&m);
    poly_gf2_free(&shifted);
    poly_gf2_free(&quotient);
    poly_gf2_free(&parity);
    poly_gf2m_free(&g);
    gf2m_ctx_free(&ctx);
    return CODECTK_ENOMEM;
  }

  memset(out, 0, total_bytes);

  /* Copy message bits */
  for (size_t i = 0; i < in_bits; i++) {
    int byte_idx = (int)(i / 8);
    int bit_idx = (int)(i % 8);
    int bit = (in[byte_idx] >> bit_idx) & 1;
    if (bit) {
      out[byte_idx] |= (uint8_t)(1 << bit_idx);
    }
  }

  /* Copy parity bits */
  for (int i = 0; i < r; i++) {
    size_t bit_pos = in_bits + (size_t)i;
    int byte_idx = (int)(bit_pos / 8);
    int bit_idx = (int)(bit_pos % 8);

    if (poly_gf2_get_coeff(&parity, i)) {
      out[byte_idx] |= (uint8_t)(1 << bit_idx);
    }
  }

  *out_bits = total_bits;

  /* Cleanup */
  poly_gf2_free(&g_gf2);
  poly_gf2_free(&m);
  poly_gf2_free(&shifted);
  poly_gf2_free(&quotient);
  poly_gf2_free(&parity);
  poly_gf2m_free(&g);
  gf2m_ctx_free(&ctx);

  return CODECTK_OK;
}

/**
 * Berlekamp-Massey algorithm for finding error locator polynomial.
 * Given syndromes S_1, S_3, ..., S_{2t-1}, finds Λ(x) such that:
 * Λ(α^{-i}) = 0 for each error location i.
 *
 * Returns the error locator polynomial in lambda.
 */
static void berlekamp_massey(poly_gf2m_t *lambda, const uint16_t *syndromes,
                              unsigned t, const gf2m_ctx *ctx) {
  /* Initialize Λ(x) = 1 */
  poly_gf2m_zero(lambda);
  poly_gf2m_set_coeff(lambda, 0, 1);

  /* B(x) = 1 (previous Λ) */
  poly_gf2m_t B;
  poly_gf2m_init(&B, ctx, (int)(2 * t));
  poly_gf2m_set_coeff(&B, 0, 1);

  int L = 0;  /* Current error count */
  int m = 1;  /* Shift amount */
  uint16_t b = 1;  /* Previous discrepancy */

  for (unsigned n = 0; n < 2 * t; n++) {
    /* Compute discrepancy d_n = S_n + Σ Λ_i * S_{n-i} */
    uint16_t d = syndromes[n];

    for (int i = 1; i <= L; i++) {
      uint16_t lambda_i = poly_gf2m_get_coeff(lambda, i);
      if (lambda_i != 0 && n >= (unsigned)i) {
        unsigned idx = n - (unsigned)i;
        uint16_t prod = gf2m_mul(ctx, lambda_i, syndromes[idx]);
        d = gf2m_add(d, prod);
      }
    }

    if (d == 0) {
      /* No correction needed */
      m++;
    } else {
      poly_gf2m_t T;
      poly_gf2m_init(&T, ctx, lambda->deg + 1);
      poly_gf2m_copy(&T, lambda);

      /* Λ(x) = Λ(x) - (d/b) * x^m * B(x) */
      uint16_t factor = gf2m_mul(ctx, d, gf2m_inv(ctx, b));

      for (int i = 0; i <= B.deg; i++) {
        uint16_t B_i = poly_gf2m_get_coeff(&B, i);
        if (B_i != 0) {
          uint16_t term = gf2m_mul(ctx, factor, B_i);
          uint16_t current = poly_gf2m_get_coeff(lambda, i + m);
          poly_gf2m_set_coeff(lambda, i + m, gf2m_add(current, term));
        }
      }

      if (2 * L <= (int)n) {
        /* Update B(x) and L */
        L = (int)n + 1 - L;
        poly_gf2m_copy(&B, &T);
        b = d;
        m = 1;
      } else {
        m++;
      }

      poly_gf2m_free(&T);
    }
  }

  poly_gf2m_free(&B);
}

/**
 * Chien search: find roots of error locator polynomial.
 * For binary BCH, we check if Λ(α^{-i}) = 0 for i = 0, 1, ..., n-1.
 * If yes, then position i has an error.
 */
static void chien_search(uint8_t *error_positions, int *num_errors,
                         const poly_gf2m_t *lambda, unsigned n, const gf2m_ctx *ctx) {
  *num_errors = 0;

  /* For each possible position i in [0, n-1] */
  for (unsigned i = 0; i < n; i++) {
    /* Evaluate Λ(α^{-i}) = Λ(α^{n-i}) since α^n = 1 */
    unsigned exponent = (n - i) % n;
    uint16_t alpha_inv = (exponent == 0) ? 1 : ctx->alog[exponent];

    uint16_t eval = poly_gf2m_eval(lambda, alpha_inv);

    if (eval == 0) {
      /* Error at position i */
      if (*num_errors < 256) {
        error_positions[*num_errors] = (uint8_t)i;
        (*num_errors)++;
      }
    }
  }
}

/**
 * BCH Decoder
 *
 * 1. Compute syndromes S_1, S_3, ..., S_{2t-1}
 * 2. Use Berlekamp-Massey to find error locator polynomial Λ(x)
 * 3. Use Chien search to find error positions
 * 4. For binary BCH, all errors have value 1, so just flip the bits
 */
static codectk_err bch_decode(const void *pp, const uint8_t *in, size_t in_bits,
                              uint8_t *out, size_t *out_bits, size_t *corr) {
  const bch_params *P = (const bch_params*)pp;

  if (!P || !in || !out || !out_bits) return CODECTK_EINVAL;
  if (P->m < 2 || P->m > 16 || P->t == 0) return CODECTK_EINVAL;

  /* Initialize field context */
  gf2m_ctx ctx;

  uint16_t prim_polys[] = {
    0, 0, 0x7, 0xB, 0x13, 0x25, 0x43, 0x89,
    0x11D, 0x211, 0x409, 0x805, 0x1053, 0x201B, 0x4443, 0x8003, 0x100B
  };

  if (P->m >= sizeof(prim_polys) / sizeof(prim_polys[0])) {
    return CODECTK_EINVAL;
  }

  if (gf2m_ctx_init(&ctx, P->m, prim_polys[P->m]) != 0) {
    return CODECTK_ENOMEM;
  }

  unsigned n = (1U << P->m) - 1;

  if (in_bits < n) {
    gf2m_ctx_free(&ctx);
    return CODECTK_EINVAL;
  }

  /* Build received polynomial r(x) */
  poly_gf2m_t r;
  poly_gf2m_init(&r, &ctx, (int)n);

  for (size_t i = 0; i < n; i++) {
    int byte_idx = (int)(i / 8);
    int bit_idx = (int)(i % 8);
    int bit = (in[byte_idx] >> bit_idx) & 1;

    if (bit) {
      poly_gf2m_set_coeff(&r, (int)i, 1);
    }
  }

  /* Compute syndromes S_1, S_3, ..., S_{2t-1} */
  uint16_t *syndromes = (uint16_t*)calloc(2 * P->t, sizeof(uint16_t));
  if (!syndromes) {
    poly_gf2m_free(&r);
    gf2m_ctx_free(&ctx);
    return CODECTK_ENOMEM;
  }

  for (unsigned i = 0; i < 2 * P->t; i++) {
    /* S_{2i+1} = r(α^{2i+1}) */
    unsigned exp = 2 * i + 1;
    uint16_t alpha_power = ctx.alog[exp % n];
    syndromes[i] = poly_gf2m_eval(&r, alpha_power);
  }

  /* Check if all syndromes are zero (no errors) */
  int has_errors = 0;
  for (unsigned i = 0; i < 2 * P->t; i++) {
    if (syndromes[i] != 0) {
      has_errors = 1;
      break;
    }
  }

  if (!has_errors) {
    /* No errors, copy input to output */
    size_t out_bytes = (in_bits + 7) / 8;
    if (out_bytes > (*out_bits) / 8) {
      free(syndromes);
      poly_gf2m_free(&r);
      gf2m_ctx_free(&ctx);
      return CODECTK_ENOMEM;
    }

    memcpy(out, in, out_bytes);
    *out_bits = in_bits;
    if (corr) *corr = 0;

    free(syndromes);
    poly_gf2m_free(&r);
    gf2m_ctx_free(&ctx);
    return CODECTK_OK;
  }

  /* Use Berlekamp-Massey to find error locator polynomial */
  poly_gf2m_t lambda;
  poly_gf2m_init(&lambda, &ctx, (int)(2 * P->t));
  berlekamp_massey(&lambda, syndromes, P->t, &ctx);

  /* Use Chien search to find error positions */
  uint8_t error_positions[256];
  int num_errors = 0;
  chien_search(error_positions, &num_errors, &lambda, n, &ctx);

  /* Check if we found too many errors */
  if (num_errors > (int)P->t) {
    free(syndromes);
    poly_gf2m_free(&lambda);
    poly_gf2m_free(&r);
    gf2m_ctx_free(&ctx);
    return CODECTK_EDECODE;
  }

  /* Copy input to output and flip error bits */
  size_t out_bytes = (in_bits + 7) / 8;
  if (out_bytes > (*out_bits) / 8) {
    free(syndromes);
    poly_gf2m_free(&lambda);
    poly_gf2m_free(&r);
    gf2m_ctx_free(&ctx);
    return CODECTK_ENOMEM;
  }

  memcpy(out, in, out_bytes);

  /* Flip error bits (binary BCH: all errors have value 1) */
  for (int i = 0; i < num_errors; i++) {
    unsigned pos = error_positions[i];
    int byte_idx = (int)(pos / 8);
    int bit_idx = (int)(pos % 8);
    out[byte_idx] ^= (uint8_t)(1 << bit_idx);
  }

  *out_bits = in_bits;
  if (corr) *corr = (size_t)num_errors;

  /* Cleanup */
  free(syndromes);
  poly_gf2m_free(&lambda);
  poly_gf2m_free(&r);
  gf2m_ctx_free(&ctx);

  return CODECTK_OK;
}

static const codectk_codec BCH = {
  .name = "bch",
  .encode = bch_encode,
  .decode = bch_decode
};

const codectk_codec* bch_codec(void) {
  return &BCH;
}
