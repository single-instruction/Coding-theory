/**
 * test_bitio.c - Tests for bit-level I/O operations
 */

#include "../include/bitio.h"
#include <stdio.h>
#include <string.h>
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

static void test_write_read_bits(void) {
  TEST("Write and read individual bits");

  uint8_t buf[10];
  memset(buf, 0, sizeof(buf));

  bitw_t w;
  bitw_init(&w, buf, sizeof(buf));

  /* Write pattern: 10110011 */
  bitw_put(&w, 1);
  bitw_put(&w, 0);
  bitw_put(&w, 1);
  bitw_put(&w, 1);
  bitw_put(&w, 0);
  bitw_put(&w, 0);
  bitw_put(&w, 1);
  bitw_put(&w, 1);
  bitw_flush(&w);

  /* Verify: 10110011 = 0xCD in little-endian bit order */
  if (buf[0] != 0xCD) {
    FAIL("bit pattern mismatch");
    return;
  }

  /* Read back */
  bitr_t r;
  bitr_init(&r, buf, 1);

  int bits[8];
  for (int i = 0; i < 8; i++) {
    bits[i] = bitr_get(&r);
  }

  if (bits[0] != 1 || bits[1] != 0 || bits[2] != 1 || bits[3] != 1 ||
      bits[4] != 0 || bits[5] != 0 || bits[6] != 1 || bits[7] != 1) {
    FAIL("read back mismatch");
    return;
  }

  PASS();
}

static void test_partial_byte(void) {
  TEST("Partial byte handling");

  uint8_t buf[10];
  memset(buf, 0, sizeof(buf));

  bitw_t w;
  bitw_init(&w, buf, sizeof(buf));

  /* Write 5 bits: 10101 */
  bitw_put(&w, 1);
  bitw_put(&w, 0);
  bitw_put(&w, 1);
  bitw_put(&w, 0);
  bitw_put(&w, 1);
  bitw_flush(&w);

  /* Should be 10101xxx = 0x15 (lower 5 bits) */
  if ((buf[0] & 0x1F) != 0x15) {
    FAIL("partial byte mismatch");
    return;
  }

  PASS();
}

static void test_multi_byte(void) {
  TEST("Multi-byte write and read");

  uint8_t buf[10];
  memset(buf, 0, sizeof(buf));

  bitw_t w;
  bitw_init(&w, buf, sizeof(buf));

  /* Write 20 bits across 3 bytes */
  for (int i = 0; i < 20; i++) {
    bitw_put(&w, i % 2); /* Alternating 0,1,0,1... */
  }
  bitw_flush(&w);

  bitr_t r;
  bitr_init(&r, buf, 3);

  for (int i = 0; i < 20; i++) {
    int b = bitr_get(&r);
    if (b != i % 2) {
      FAIL("multi-byte mismatch");
      return;
    }
  }

  PASS();
}

static void test_eof_handling(void) {
  TEST("EOF handling");

  uint8_t buf[1] = {0xAA};
  bitr_t r;
  bitr_init(&r, buf, 1);

  /* Read 8 bits */
  for (int i = 0; i < 8; i++) {
    if (bitr_get(&r) < 0) {
      FAIL("premature EOF");
      return;
    }
  }

  /* Next read should return EOF */
  if (bitr_get(&r) != -1) {
    FAIL("EOF not detected");
    return;
  }

  PASS();
}

int test_bitio_suite(void) {
  test_count = 0;
  pass_count = 0;

  test_write_read_bits();
  test_partial_byte();
  test_multi_byte();
  test_eof_handling();

  printf("  bitio: %d/%d tests passed\n", pass_count, test_count);
  return test_count - pass_count;
}
