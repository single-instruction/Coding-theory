/**
 * test_gf2m.c - Tests for GF(2^m) field operations
 */

#include "../include/gf2m.h"
#include <stdio.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { test_count++; printf("  [%d] %s: ", test_count, name); } while (0)
#define PASS() do { pass_count++; printf("PASS\n"); } while (0)
#define FAIL(msg) do { printf("FAIL - %s\n", msg); } while (0)

static void test_ctx_init(void) {
  TEST("Field context initialization");

  gf2m_ctx ctx;
  /* GF(2^4) with primitive polynomial x^4 + x + 1 = 0x13 */
  if (gf2m_ctx_init(&ctx, 4, 0x13) != 0) {
    FAIL("init failed");
    return;
  }

  if (ctx.m != 4 || ctx.alog == NULL || ctx.log == NULL) {
    gf2m_ctx_free(&ctx);
    FAIL("context incorrect");
    return;
  }

  gf2m_ctx_free(&ctx);
  PASS();
}

static void test_field_axioms(void) {
  TEST("Field axioms (associativity, commutativity)");

  gf2m_ctx ctx;
  gf2m_ctx_init(&ctx, 4, 0x13);

  uint16_t a = 3, b = 5, c = 7;

  /* Commutativity: a*b = b*a */
  uint16_t ab = gf2m_mul(&ctx, a, b);
  uint16_t ba = gf2m_mul(&ctx, b, a);
  if (ab != ba) {
    gf2m_ctx_free(&ctx);
    FAIL("multiplication not commutative");
    return;
  }

  /* Associativity: (a*b)*c = a*(b*c) */
  uint16_t abc1 = gf2m_mul(&ctx, ab, c);
  uint16_t bc = gf2m_mul(&ctx, b, c);
  uint16_t abc2 = gf2m_mul(&ctx, a, bc);
  if (abc1 != abc2) {
    gf2m_ctx_free(&ctx);
    FAIL("multiplication not associative");
    return;
  }

  gf2m_ctx_free(&ctx);
  PASS();
}

static void test_inverse(void) {
  TEST("Multiplicative inverse");

  gf2m_ctx ctx;
  gf2m_ctx_init(&ctx, 4, 0x13);

  for (uint16_t a = 1; a < 16; a++) {
    uint16_t inv_a = gf2m_inv(&ctx, a);
    uint16_t product = gf2m_mul(&ctx, a, inv_a);

    if (product != 1) {
      gf2m_ctx_free(&ctx);
      FAIL("inverse incorrect");
      return;
    }
  }

  gf2m_ctx_free(&ctx);
  PASS();
}

static void test_power(void) {
  TEST("Exponentiation");

  gf2m_ctx ctx;
  gf2m_ctx_init(&ctx, 4, 0x13);

  uint16_t a = 3;

  /* a^0 = 1 */
  if (gf2m_pow(&ctx, a, 0) != 1) {
    gf2m_ctx_free(&ctx);
    FAIL("a^0 != 1");
    return;
  }

  /* a^1 = a */
  if (gf2m_pow(&ctx, a, 1) != a) {
    gf2m_ctx_free(&ctx);
    FAIL("a^1 != a");
    return;
  }

  /* a^2 = a*a */
  uint16_t a2 = gf2m_mul(&ctx, a, a);
  if (gf2m_pow(&ctx, a, 2) != a2) {
    gf2m_ctx_free(&ctx);
    FAIL("a^2 incorrect");
    return;
  }

  gf2m_ctx_free(&ctx);
  PASS();
}

int test_gf2m_suite(void) {
  test_count = 0;
  pass_count = 0;

  test_ctx_init();
  test_field_axioms();
  test_inverse();
  test_power();

  printf("  gf2m: %d/%d tests passed\n", pass_count, test_count);
  return test_count - pass_count;
}
