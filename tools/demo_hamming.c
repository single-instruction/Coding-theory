#include "../include/hamming.h"
#include "../include/codectk.h"
#include <stdio.h>
#include <string.h>

int main(void) {
  const codectk_codec *codec = hamming_codec();
  hamming_params params = {.m = 3}; // Hamming(7,4)

  // Original data: 4 bits = 1010 binary
  uint8_t data[1] = {0x0A};
  uint8_t encoded[10];
  uint8_t decoded[10];
  size_t enc_bits = sizeof(encoded) * 8;
  size_t dec_bits = sizeof(decoded) * 8;

  // Encode
  codec->encode(&params, data, 4, encoded, &enc_bits);
  printf("Original: 0x%02X (4 bits: 1010)\n", data[0] & 0x0F);
  printf("Encoded:  0x%02X (7 bits)\n\n", encoded[0] & 0x7F);

  // Introduce single-bit error
  printf("Introducing error: flipping bit 2\n");
  encoded[0] ^= 0x04; // Flip bit 2
  printf("Corrupted: 0x%02X\n\n", encoded[0] & 0x7F);

  // Decode with error correction
  size_t corrected = 0;
  codectk_err err = codec->decode(&params, encoded, enc_bits,
                                   decoded, &dec_bits, &corrected);

  printf("Decode result: %s\n", codectk_strerror(err));
  printf("Errors corrected: %zu\n", corrected);
  printf("Recovered: 0x%02X (matches original: %s)\n",
         decoded[0] & 0x0F,
         ((decoded[0] & 0x0F) == (data[0] & 0x0F)) ? "YES" : "NO");

  return 0;
}