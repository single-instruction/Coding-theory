/**
 * test_gf2.c - Tests for GF(2) operations
 */

#include "../include/gf2.h"
#include <stdio.h>
#include <assert.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) \
  do { \
    test_count++; \
    printf("  [%d] %s: ", test_count, name); \
  } while (0)

#define PASS() \
  do { \
    pass_count++; \
    printf("PASS\n"); \
  } while (0)

#define FAIL(msg) \
  do { \
    printf("FAIL - %s\n", msg); \
  } while (0)

static void test_vec_init_free(void) {
  TEST("Vector init and free");

  gf2_vec v;
  if (gf2_vec_init(&v, 100) != 0) {
    FAIL("init failed");
    return;
  }

  if (v.n_bits != 100 || v.bits == NULL) {
    gf2_vec_free(&v);
    FAIL("init incorrect");
    return;
  }

  gf2_vec_free(&v);
  PASS();
}

static void test_vec_set_get(void) {
  TEST("Vector set and get");

  gf2_vec v;
  gf2_vec_init(&v, 64);

  gf2_vec_set(&v, 0, 1);
  gf2_vec_set(&v, 7, 1);
  gf2_vec_set(&v, 15, 1);
  gf2_vec_set(&v, 63, 1);

  if (gf2_vec_get(&v, 0) != 1 || gf2_vec_get(&v, 7) != 1 ||
      gf2_vec_get(&v, 15) != 1 || gf2_vec_get(&v, 63) != 1) {
    gf2_vec_free(&v);
    FAIL("get mismatch");
    return;
  }

  if (gf2_vec_get(&v, 1) != 0 || gf2_vec_get(&v, 5) != 0) {
    gf2_vec_free(&v);
    FAIL("zero bit not zero");
    return;
  }

  gf2_vec_free(&v);
  PASS();
}

static void test_vec_xor(void) {
  TEST("Vector XOR");

  gf2_vec a, b, result;
  gf2_vec_init(&a, 32);
  gf2_vec_init(&b, 32);
  gf2_vec_init(&result, 32);

  gf2_vec_set(&a, 0, 1);
  gf2_vec_set(&a, 10, 1);
  gf2_vec_set(&b, 0, 1);
  gf2_vec_set(&b, 20, 1);

  gf2_vec_copy(&result, &a);
  gf2_vec_xor(&result, &b);

  /* result[0] = 1 XOR 1 = 0 */
  /* result[10] = 1 XOR 0 = 1 */
  /* result[20] = 0 XOR 1 = 1 */
  if (gf2_vec_get(&result, 0) != 0 || gf2_vec_get(&result, 10) != 1 ||
      gf2_vec_get(&result, 20) != 1) {
    gf2_vec_free(&a);
    gf2_vec_free(&b);
    gf2_vec_free(&result);
    FAIL("XOR result incorrect");
    return;
  }

  gf2_vec_free(&a);
  gf2_vec_free(&b);
  gf2_vec_free(&result);
  PASS();
}

static void test_vec_dot(void) {
  TEST("Vector dot product");

  gf2_vec a, b;
  gf2_vec_init(&a, 32);
  gf2_vec_init(&b, 32);

  gf2_vec_set(&a, 0, 1);
  gf2_vec_set(&a, 5, 1);
  gf2_vec_set(&a, 10, 1);
  gf2_vec_set(&b, 0, 1);
  gf2_vec_set(&b, 10, 1);

  /* dot = (1*1) + (0*0) + (1*1) = 1 + 0 + 1 = 0 (mod 2) */
  int dot = gf2_vec_dot(&a, &b);
  if (dot != 0) {
    gf2_vec_free(&a);
    gf2_vec_free(&b);
    FAIL("dot product incorrect");
    return;
  }

  gf2_vec_free(&a);
  gf2_vec_free(&b);
  PASS();
}

static void test_vec_weight(void) {
  TEST("Vector Hamming weight");

  gf2_vec v;
  gf2_vec_init(&v, 100);

  gf2_vec_set(&v, 0, 1);
  gf2_vec_set(&v, 10, 1);
  gf2_vec_set(&v, 20, 1);
  gf2_vec_set(&v, 99, 1);

  if (gf2_vec_weight(&v) != 4) {
    gf2_vec_free(&v);
    FAIL("weight incorrect");
    return;
  }

  gf2_vec_free(&v);
  PASS();
}

static void test_mat_init_free(void) {
  TEST("Matrix init and free");

  gf2_mat m;
  if (gf2_mat_init(&m, 10, 20) != 0) {
    FAIL("init failed");
    return;
  }

  if (m.n_rows != 10 || m.n_cols != 20) {
    gf2_mat_free(&m);
    FAIL("dimensions wrong");
    return;
  }

  gf2_mat_free(&m);
  PASS();
}

static void test_mat_row_reduce(void) {
  TEST("Matrix row reduction");

  gf2_mat m;
  gf2_mat_init(&m, 3, 3);

  /* Create identity matrix */
  gf2_mat_set(&m, 0, 0, 1);
  gf2_mat_set(&m, 1, 1, 1);
  gf2_mat_set(&m, 2, 2, 1);

  size_t rank = gf2_mat_row_reduce(&m);

  if (rank != 3) {
    gf2_mat_free(&m);
    FAIL("rank incorrect");
    return;
  }

  gf2_mat_free(&m);
  PASS();
}

int test_gf2_suite(void) {
  test_count = 0;
  pass_count = 0;

  test_vec_init_free();
  test_vec_set_get();
  test_vec_xor();
  test_vec_dot();
  test_vec_weight();
  test_mat_init_free();
  test_mat_row_reduce();

  printf("  gf2: %d/%d tests passed\n", pass_count, test_count);
  return test_count - pass_count;
}
