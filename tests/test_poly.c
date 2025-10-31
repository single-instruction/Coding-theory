/**
 * test_poly.c - Tests for polynomial operations
 */

#include "../include/poly.h"
#include <stdio.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { test_count++; printf("  [%d] %s: ", test_count, name); } while (0)
#define PASS() do { pass_count++; printf("PASS\n"); } while (0)
#define FAIL(msg) do { printf("FAIL - %s\n", msg); } while (0)

static void test_gf2_poly_add(void) {
  TEST("GF(2) polynomial addition");

  poly_gf2_t a, b, result;
  poly_gf2_init(&a, 10);
  poly_gf2_init(&b, 10);
  poly_gf2_init(&result, 10);

  /* a = x^2 + 1 */
  poly_gf2_set_coeff(&a, 0, 1);
  poly_gf2_set_coeff(&a, 2, 1);

  /* b = x^2 + x */
  poly_gf2_set_coeff(&b, 1, 1);
  poly_gf2_set_coeff(&b, 2, 1);

  /* result = a + b = x + 1 */
  poly_gf2_add(&result, &a, &b);

  if (poly_gf2_get_coeff(&result, 0) != 1 ||
      poly_gf2_get_coeff(&result, 1) != 1 ||
      poly_gf2_get_coeff(&result, 2) != 0) {
    poly_gf2_free(&a);
    poly_gf2_free(&b);
    poly_gf2_free(&result);
    FAIL("addition incorrect");
    return;
  }

  poly_gf2_free(&a);
  poly_gf2_free(&b);
  poly_gf2_free(&result);
  PASS();
}

static void test_gf2_poly_mul(void) {
  TEST("GF(2) polynomial multiplication");

  poly_gf2_t a, b, result;
  poly_gf2_init(&a, 10);
  poly_gf2_init(&b, 10);
  poly_gf2_init(&result, 20);

  /* a = x + 1 */
  poly_gf2_set_coeff(&a, 0, 1);
  poly_gf2_set_coeff(&a, 1, 1);

  /* b = x + 1 */
  poly_gf2_set_coeff(&b, 0, 1);
  poly_gf2_set_coeff(&b, 1, 1);

  /* result = (x+1)*(x+1) = x^2 + x + x + 1 = x^2 + 1 (mod 2) */
  poly_gf2_mul(&result, &a, &b);

  if (poly_gf2_get_coeff(&result, 0) != 1 ||
      poly_gf2_get_coeff(&result, 1) != 0 ||
      poly_gf2_get_coeff(&result, 2) != 1) {
    poly_gf2_free(&a);
    poly_gf2_free(&b);
    poly_gf2_free(&result);
    FAIL("multiplication incorrect");
    return;
  }

  poly_gf2_free(&a);
  poly_gf2_free(&b);
  poly_gf2_free(&result);
  PASS();
}

static void test_gf2m_poly_eval(void) {
  TEST("GF(2^m) polynomial evaluation");

  gf2m_ctx ctx;
  gf2m_ctx_init(&ctx, 4, 0x13);

  poly_gf2m_t p;
  poly_gf2m_init(&p, &ctx, 5);

  /* p(x) = 3x^2 + 5x + 2 */
  poly_gf2m_set_coeff(&p, 0, 2);
  poly_gf2m_set_coeff(&p, 1, 5);
  poly_gf2m_set_coeff(&p, 2, 3);

  /* Evaluate at x=1: p(1) = 3 + 5 + 2 = 3 XOR 5 XOR 2 */
  uint16_t result = poly_gf2m_eval(&p, 1);
  uint16_t expected = gf2m_add(gf2m_add(3, 5), 2);

  if (result != expected) {
    poly_gf2m_free(&p);
    gf2m_ctx_free(&ctx);
    FAIL("evaluation incorrect");
    return;
  }

  poly_gf2m_free(&p);
  gf2m_ctx_free(&ctx);
  PASS();
}

static void test_gf2m_poly_gcd(void) {
  TEST("GF(2^m) polynomial GCD");

  gf2m_ctx ctx;
  gf2m_ctx_init(&ctx, 4, 0x13);

  poly_gf2m_t a, b, result;
  poly_gf2m_init(&a, &ctx, 10);
  poly_gf2m_init(&b, &ctx, 10);
  poly_gf2m_init(&result, &ctx, 10);

  /* a = x^2 + 1 */
  poly_gf2m_set_coeff(&a, 0, 1);
  poly_gf2m_set_coeff(&a, 2, 1);

  /* b = x^2 + 1 (same polynomial) */
  poly_gf2m_set_coeff(&b, 0, 1);
  poly_gf2m_set_coeff(&b, 2, 1);

  /* GCD should be x^2 + 1 (or normalized version) */
  poly_gf2m_gcd(&result, &a, &b);

  if (result.deg < 0) {
    poly_gf2m_free(&a);
    poly_gf2m_free(&b);
    poly_gf2m_free(&result);
    gf2m_ctx_free(&ctx);
    FAIL("GCD is zero");
    return;
  }

  poly_gf2m_free(&a);
  poly_gf2m_free(&b);
  poly_gf2m_free(&result);
  gf2m_ctx_free(&ctx);
  PASS();
}

int test_poly_suite(void) {
  test_count = 0;
  pass_count = 0;

  test_gf2_poly_add();
  test_gf2_poly_mul();
  test_gf2m_poly_eval();
  test_gf2m_poly_gcd();

  printf("  poly: %d/%d tests passed\n", pass_count, test_count);
  return test_count - pass_count;
}
