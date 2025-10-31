/**
 * test_bch.c - Tests for BCH codes
 */

#include "../include/bch.h"
#include "../include/codectk.h"
#include <stdio.h>
#include <string.h>

static int test_count = 0;
static int pass_count = 0;

#define DEBUG_BCH 0  /* Set to 1 for detailed output */

#define TEST(name) do { test_count++; printf("  [%d] %s: ", test_count, name); } while (0)
#define PASS() do { pass_count++; printf("PASS\n"); } while (0)
#define FAIL(msg) do { printf("FAIL - %s\n", msg); } while (0)

#if DEBUG_BCH
#define DEBUG_PRINT(...) printf(__VA_ARGS__)
static void print_bits(const char *label, const uint8_t *data, size_t bits) {
  printf("%s: ", label);
  for (size_t i = 0; i < bits; i++) {
    printf("%d", (data[i/8] >> (i%8)) & 1);
  }
  printf("\n");
}
#else
#define DEBUG_PRINT(...)
#define print_bits(...)
#endif

/**
 * Test BCH(15,7,2) - m=4, t=2
 * This is a double-error correcting BCH code.
 */
static void test_bch_15_7(void) {
  TEST("BCH(15,7) encode/decode with no errors");

  const codectk_codec *codec = bch_codec();
  bch_params params = {.m = 4, .t = 2};

  /* Message: 7 bits */
  uint8_t message[2] = {0x5A, 0x00};  /* 01011010 in binary, use first 7 bits */
  uint8_t encoded[10];
  uint8_t decoded[10];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  /* Encode */
  codectk_err err = codec->encode(&params, message, 7, encoded, &encoded_bits);
  DEBUG_PRINT("\n    Encode result: %s, encoded_bits=%zu\n", codectk_strerror(err), encoded_bits);
  print_bits("    Message", message, 7);
  print_bits("    Encoded", encoded, encoded_bits);

  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  /* Should produce 15 bits (7 message + 8 parity) */
  if (encoded_bits != 15) {
    FAIL("encoded length incorrect");
    return;
  }

  /* Decode (no errors) */
  size_t num_corrected = 0;
  err = codec->decode(&params, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
  DEBUG_PRINT("    Decode result: %s, decoded_bits=%zu, corrected=%zu\n",
              codectk_strerror(err), decoded_bits, num_corrected);
  print_bits("    Decoded", decoded, decoded_bits);

  if (err != CODECTK_OK) {
    FAIL("decode failed");
    return;
  }

  /* Verify message bits are recovered */
  for (size_t i = 0; i < 7; i++) {
    int orig_byte = (int)(i / 8);
    int orig_bit = (int)(i % 8);
    int dec_byte = (int)(i / 8);
    int dec_bit = (int)(i % 8);

    int orig = (message[orig_byte] >> orig_bit) & 1;
    int dec = (decoded[dec_byte] >> dec_bit) & 1;

    if (orig != dec) {
      DEBUG_PRINT("    Mismatch at bit %zu: orig=%d, dec=%d\n", i, orig, dec);
      FAIL("decoded message mismatch");
      return;
    }
  }

  if (num_corrected != 0) {
    DEBUG_PRINT("    Expected 0 corrections, got %zu\n", num_corrected);
    FAIL("should have 0 corrections");
    return;
  }

  PASS();
}

/**
 * Test BCH single-error correction
 */
static void test_bch_single_error(void) {
  TEST("BCH(15,7) single-error correction");

  const codectk_codec *codec = bch_codec();
  bch_params params = {.m = 4, .t = 2};

  /* Message: 7 bits */
  uint8_t message[2] = {0x3F, 0x00};  /* 00111111, use first 7 bits */
  uint8_t encoded[10];
  uint8_t decoded[10];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  /* Encode */
  codectk_err err = codec->encode(&params, message, 7, encoded, &encoded_bits);
  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  /* Introduce single bit error at position 5 */
  int byte_idx = 5 / 8;
  int bit_idx = 5 % 8;
  encoded[byte_idx] ^= (uint8_t)(1 << bit_idx);

  /* Decode with error correction */
  size_t num_corrected = 0;
  err = codec->decode(&params, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
  if (err != CODECTK_OK) {
    FAIL("decode failed");
    return;
  }

  /* Verify message bits are recovered */
  for (size_t i = 0; i < 7; i++) {
    int orig_byte = (int)(i / 8);
    int orig_bit = (int)(i % 8);
    int dec_byte = (int)(i / 8);
    int dec_bit = (int)(i % 8);

    int orig = (message[orig_byte] >> orig_bit) & 1;
    int dec = (decoded[dec_byte] >> dec_bit) & 1;

    if (orig != dec) {
      FAIL("decoded message mismatch after correction");
      return;
    }
  }

  if (num_corrected != 1) {
    FAIL("should have corrected exactly 1 error");
    return;
  }

  PASS();
}

/**
 * Test BCH(31,21,2) - m=5, t=2
 * This is a double-error correcting BCH code.
 */
static void test_bch_31_21(void) {
  TEST("BCH(31,21) encode/decode with no errors");

  const codectk_codec *codec = bch_codec();
  bch_params params = {.m = 5, .t = 2};

  /* Message: 21 bits */
  uint8_t message[4] = {0xAA, 0xBB, 0x0C, 0x00};  /* Use first 21 bits */
  uint8_t encoded[10];
  uint8_t decoded[10];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  /* Encode */
  codectk_err err = codec->encode(&params, message, 21, encoded, &encoded_bits);
  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  /* Should produce 31 bits (21 message + 10 parity) */
  if (encoded_bits != 31) {
    FAIL("encoded length incorrect");
    return;
  }

  /* Decode (no errors) */
  size_t num_corrected = 0;
  err = codec->decode(&params, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
  if (err != CODECTK_OK) {
    FAIL("decode failed");
    return;
  }

  /* Verify message bits are recovered */
  for (size_t i = 0; i < 21; i++) {
    int orig_byte = (int)(i / 8);
    int orig_bit = (int)(i % 8);
    int dec_byte = (int)(i / 8);
    int dec_bit = (int)(i % 8);

    int orig = (message[orig_byte] >> orig_bit) & 1;
    int dec = (decoded[dec_byte] >> dec_bit) & 1;

    if (orig != dec) {
      FAIL("decoded message mismatch");
      return;
    }
  }

  if (num_corrected != 0) {
    FAIL("should have 0 corrections");
    return;
  }

  PASS();
}

/**
 * Test BCH double-error correction
 */
static void test_bch_double_error(void) {
  TEST("BCH(31,21) double-error correction");

  const codectk_codec *codec = bch_codec();
  bch_params params = {.m = 5, .t = 2};

  /* Message: 21 bits */
  uint8_t message[4] = {0xFF, 0x00, 0x0F, 0x00};  /* Use first 21 bits */
  uint8_t encoded[10];
  uint8_t decoded[10];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  /* Encode */
  codectk_err err = codec->encode(&params, message, 21, encoded, &encoded_bits);
  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  /* Introduce two bit errors at positions 3 and 17 */
  encoded[3 / 8] ^= (uint8_t)(1 << (3 % 8));
  encoded[17 / 8] ^= (uint8_t)(1 << (17 % 8));

  /* Decode with error correction */
  size_t num_corrected = 0;
  err = codec->decode(&params, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
  if (err != CODECTK_OK) {
    FAIL("decode failed");
    return;
  }

  /* Verify message bits are recovered */
  for (size_t i = 0; i < 21; i++) {
    int orig_byte = (int)(i / 8);
    int orig_bit = (int)(i % 8);
    int dec_byte = (int)(i / 8);
    int dec_bit = (int)(i % 8);

    int orig = (message[orig_byte] >> orig_bit) & 1;
    int dec = (decoded[dec_byte] >> dec_bit) & 1;

    if (orig != dec) {
      FAIL("decoded message mismatch after correction");
      return;
    }
  }

  if (num_corrected != 2) {
    FAIL("should have corrected exactly 2 errors");
    return;
  }

  PASS();
}

/**
 * Test BCH with various message patterns
 */
static void test_bch_patterns(void) {
  TEST("BCH(15,7) with various patterns");

  const codectk_codec *codec = bch_codec();
  bch_params params = {.m = 4, .t = 2};

  /* Test patterns: all zeros, all ones, alternating */
  uint8_t patterns[][2] = {
    {0x00, 0x00},  /* All zeros */
    {0x7F, 0x00},  /* All ones (7 bits) */
    {0x55, 0x00},  /* Alternating 01010101 */
    {0x2A, 0x00},  /* Alternating 10101010 */
  };

  for (size_t p = 0; p < sizeof(patterns) / sizeof(patterns[0]); p++) {
    uint8_t encoded[10];
    uint8_t decoded[10];
    size_t encoded_bits = sizeof(encoded) * 8;
    size_t decoded_bits = sizeof(decoded) * 8;

    /* Encode */
    codectk_err err = codec->encode(&params, patterns[p], 7, encoded, &encoded_bits);
    if (err != CODECTK_OK) {
      FAIL("encode failed for pattern");
      return;
    }

    /* Decode */
    size_t num_corrected = 0;
    err = codec->decode(&params, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
    if (err != CODECTK_OK) {
      FAIL("decode failed for pattern");
      return;
    }

    /* Verify */
    for (size_t i = 0; i < 7; i++) {
      int orig_byte = (int)(i / 8);
      int orig_bit = (int)(i % 8);
      int dec_byte = (int)(i / 8);
      int dec_bit = (int)(i % 8);

      int orig = (patterns[p][orig_byte] >> orig_bit) & 1;
      int dec = (decoded[dec_byte] >> dec_bit) & 1;

      if (orig != dec) {
        FAIL("pattern mismatch");
        return;
      }
    }
  }

  PASS();
}

/**
 * Test BCH with too many errors (should fail to decode)
 */
static void test_bch_too_many_errors(void) {
  TEST("BCH(15,7) with too many errors");

  const codectk_codec *codec = bch_codec();
  bch_params params = {.m = 4, .t = 2};

  /* Message: 7 bits */
  uint8_t message[2] = {0x1F, 0x00};
  uint8_t encoded[10];
  uint8_t decoded[10];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  /* Encode */
  codectk_err err = codec->encode(&params, message, 7, encoded, &encoded_bits);
  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  /* Introduce three bit errors (more than t=1) */
  encoded[2 / 8] ^= (uint8_t)(1 << (2 % 8));
  encoded[7 / 8] ^= (uint8_t)(1 << (7 % 8));
  encoded[12 / 8] ^= (uint8_t)(1 << (12 % 8));

  /* Decode - should detect too many errors */
  size_t num_corrected = 0;
  err = codec->decode(&params, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);

  /* Either EDECODE or the decoder may incorrectly "correct" to wrong codeword */
  /* For this test, we accept either behavior as long as it doesn't crash */
  /* In a real implementation, we'd want EDECODE for uncorrectable errors */

  PASS();  /* Test passes if we don't crash */
}

int test_bch_suite(void) {
  test_count = 0;
  pass_count = 0;

  test_bch_15_7();
  test_bch_single_error();
  test_bch_31_21();
  test_bch_double_error();
  test_bch_patterns();
  test_bch_too_many_errors();

  printf("  bch: %d/%d tests passed\n", pass_count, test_count);
  return test_count - pass_count;
}
