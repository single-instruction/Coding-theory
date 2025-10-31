/**
 * test_hamming.c - Tests for Hamming error-correcting codes
 */

#include "../include/hamming.h"
#include "../include/codectk.h"
#include <stdio.h>
#include <string.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { test_count++; printf("  [%d] %s: ", test_count, name); } while (0)
#define PASS() do { pass_count++; printf("PASS\n"); } while (0)
#define FAIL(msg) do { printf("FAIL - %s\n", msg); } while (0)

static void test_hamming_encode_decode(void) {
  TEST("Hamming(7,4) encode/decode");

  const codectk_codec *codec = hamming_codec();
  hamming_params params = {.m = 3}; /* (7,4) code */

  uint8_t input[1] = {0x0A}; /* 4 bits: 1010 */
  uint8_t encoded[10];
  uint8_t decoded[10];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  /* Encode */
  codectk_err err = codec->encode(&params, input, 4, encoded, &encoded_bits);
  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  /* Decode (no errors) */
  size_t num_corrected = 0;
  err = codec->decode(&params, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
  if (err != CODECTK_OK) {
    FAIL("decode failed");
    return;
  }

  /* Verify */
  if ((decoded[0] & 0x0F) != (input[0] & 0x0F)) {
    FAIL("decoded data mismatch");
    return;
  }

  if (num_corrected != 0) {
    FAIL("no errors should be detected");
    return;
  }

  PASS();
}

static void test_hamming_single_error_correction(void) {
  TEST("Hamming single-bit error correction");

  const codectk_codec *codec = hamming_codec();
  hamming_params params = {.m = 3};

  uint8_t input[1] = {0x0F}; /* 4 bits: 1111 */
  uint8_t encoded[10];
  uint8_t decoded[10];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  /* Encode */
  codec->encode(&params, input, 4, encoded, &encoded_bits);

  /* Introduce single-bit error */
  encoded[0] ^= 0x01; /* Flip LSB */

  /* Decode */
  size_t num_corrected = 0;
  codectk_err err = codec->decode(&params, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
  if (err != CODECTK_OK) {
    FAIL("decode failed");
    return;
  }

  /* Verify correction */
  if (num_corrected != 1) {
    FAIL("should have corrected 1 error");
    return;
  }

  if ((decoded[0] & 0x0F) != (input[0] & 0x0F)) {
    FAIL("decoded data mismatch after correction");
    return;
  }

  PASS();
}

static void test_hamming_multiple_codes(void) {
  TEST("Multiple code sizes");

  const codectk_codec *codec = hamming_codec();

  for (unsigned m = 3; m <= 5; m++) {
    hamming_params params = {.m = m};
    unsigned n = (1u << m) - 1;
    unsigned k = n - m;

    uint8_t input[10];
    memset(input, 0xAA, sizeof(input));

    uint8_t encoded[100];
    uint8_t decoded[100];
    size_t encoded_bits = sizeof(encoded) * 8;
    size_t decoded_bits = sizeof(decoded) * 8;

    codectk_err err = codec->encode(&params, input, k, encoded, &encoded_bits);
    if (err != CODECTK_OK) {
      FAIL("encode failed for varying m");
      return;
    }

    size_t num_corrected = 0;
    err = codec->decode(&params, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
    if (err != CODECTK_OK) {
      FAIL("decode failed for varying m");
      return;
    }

    /* Verify first k bits match */
    int match = 1;
    for (unsigned i = 0; i < k; i++) {
      int in_bit = (input[i/8] >> (i%8)) & 1;
      int out_bit = (decoded[i/8] >> (i%8)) & 1;
      if (in_bit != out_bit) {
        match = 0;
        break;
      }
    }

    if (!match) {
      FAIL("data mismatch for varying m");
      return;
    }
  }

  PASS();
}

int test_hamming_suite(void) {
  test_count = 0;
  pass_count = 0;

  test_hamming_encode_decode();
  test_hamming_single_error_correction();
  test_hamming_multiple_codes();

  printf("  hamming: %d/%d tests passed\n", pass_count, test_count);
  return test_count - pass_count;
}
