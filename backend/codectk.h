#pragma once
#include <stddef.h>
#include <stdint.h>

typedef enum {
  CODECTK_OK = 0,
  CODECTK_EINVAL,
  CODECTK_ENOMEM,
  CODECTK_EDECODE,
  CODECTK_ENOTSUP
} codectk_err;

typedef struct {
  const char *name;
  // encode: in bits -> out bits (stream-safe)
  codectk_err (*encode)(const void *params,
                        const uint8_t *in, size_t in_bits,
                        uint8_t *out, size_t *out_bits);
  // decode: corrects errors if possible,
  //  returns EDECODE on failure
  codectk_err (*decode)(const void *params,
                        const uint8_t *in, size_t in_bits,
                        uint8_t *out, size_t *out_bits,
                        size_t *num_corrected);
} codectk_codec;

// simple registry/factory
const codectk_codec* codectk_get(const char *name);
