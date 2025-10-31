/**
 * test_huffman.c - Tests for Huffman coding
 */

#include "../include/huffman.h"
#include "../include/codectk.h"
#include <stdio.h>
#include <string.h>

static int test_count = 0;
static int pass_count = 0;

#define TEST(name) do { test_count++; printf("  [%d] %s: ", test_count, name); } while (0)
#define PASS() do { pass_count++; printf("PASS\n"); } while (0)
#define FAIL(msg) do { printf("FAIL - %s\n", msg); } while (0)

static void test_huffman_encode_decode(void) {
  TEST("Huffman encode/decode round-trip");

  const codectk_codec *codec = huffman_codec();

  const char *text = "hello world";
  size_t text_len = strlen(text);

  uint8_t encoded[2000];  /* Need room for 1032-byte header + data */
  uint8_t decoded[2000];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  /* Encode */
  codectk_err err = codec->encode(NULL, (const uint8_t*)text, text_len * 8,
                                    encoded, &encoded_bits);
  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  /* Decode */
  size_t num_corrected = 0;
  err = codec->decode(NULL, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
  if (err != CODECTK_OK) {
    FAIL("decode failed");
    return;
  }

  /* Verify */
  size_t decoded_bytes = decoded_bits / 8;
  if (decoded_bytes != text_len) {
    FAIL("decoded length mismatch");
    return;
  }

  if (memcmp(text, decoded, text_len) != 0) {
    FAIL("decoded data mismatch");
    return;
  }

  PASS();
}

static void test_huffman_single_symbol(void) {
  TEST("Huffman with single repeated symbol");

  const codectk_codec *codec = huffman_codec();

  uint8_t input[10];
  memset(input, 'A', sizeof(input));

  uint8_t encoded[2000];  /* Need room for header */
  uint8_t decoded[2000];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  codectk_err err = codec->encode(NULL, input, sizeof(input) * 8, encoded, &encoded_bits);
  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  size_t num_corrected = 0;
  err = codec->decode(NULL, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
  if (err != CODECTK_OK) {
    FAIL("decode failed");
    return;
  }

  if (decoded_bits / 8 != sizeof(input)) {
    FAIL("decoded length mismatch");
    return;
  }

  if (memcmp(input, decoded, sizeof(input)) != 0) {
    FAIL("decoded data mismatch");
    return;
  }

  PASS();
}

static void test_huffman_varied_frequencies(void) {
  TEST("Huffman with varied symbol frequencies");

  const codectk_codec *codec = huffman_codec();

  /* Create input with varied frequencies: lots of 'a', some 'b', few 'c' */
  char input[100];
  int pos = 0;
  for (int i = 0; i < 70; i++) input[pos++] = 'a';
  for (int i = 0; i < 20; i++) input[pos++] = 'b';
  for (int i = 0; i < 10; i++) input[pos++] = 'c';

  uint8_t encoded[3000];  /* Large buffer for header + data */
  uint8_t decoded[3000];
  size_t encoded_bits = sizeof(encoded) * 8;
  size_t decoded_bits = sizeof(decoded) * 8;

  codectk_err err = codec->encode(NULL, (const uint8_t*)input, sizeof(input) * 8,
                                    encoded, &encoded_bits);
  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  size_t num_corrected = 0;
  err = codec->decode(NULL, encoded, encoded_bits, decoded, &decoded_bits, &num_corrected);
  if (err != CODECTK_OK) {
    FAIL("decode failed");
    return;
  }

  if (decoded_bits / 8 != sizeof(input)) {
    FAIL("decoded length mismatch");
    return;
  }

  if (memcmp(input, decoded, sizeof(input)) != 0) {
    FAIL("decoded data mismatch");
    return;
  }

  PASS();
}

static void test_huffman_compression_ratio(void) {
  TEST("Huffman compression ratio check");

  const codectk_codec *codec = huffman_codec();

  /* Highly compressible input - must be large enough to overcome 1032-byte header */
  char input[5000];
  memset(input, 'x', sizeof(input));

  uint8_t encoded[20000];  /* Large buffer for header + data */
  size_t encoded_bits = sizeof(encoded) * 8;

  codectk_err err = codec->encode(NULL, (const uint8_t*)input, sizeof(input) * 8,
                                    encoded, &encoded_bits);
  if (err != CODECTK_OK) {
    FAIL("encode failed");
    return;
  }

  /* With 5000 bytes of single symbol, encoding should be much smaller
   * Header is 1032 bytes, but data compression should make total < input */
  size_t encoded_bytes = (encoded_bits + 7) / 8;
  if (encoded_bytes >= sizeof(input)) {
    FAIL("no compression achieved");
    return;
  }

  PASS();
}

int test_huffman_suite(void) {
  test_count = 0;
  pass_count = 0;

  test_huffman_encode_decode();
  test_huffman_single_symbol();
  test_huffman_varied_frequencies();
  test_huffman_compression_ratio();

  printf("  huffman: %d/%d tests passed\n", pass_count, test_count);
  return test_count - pass_count;
}
