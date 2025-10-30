// src/bch.c  (high-level structure; fill TODOs)
#include "bch.h"
#include "gf2m.h"
#include "poly.h"

static codectk_err bch_encode(const void *pp, const uint8_t *in, size_t in_bits,
                              uint8_t *out, size_t *out_bits){
  const bch_params *P=(const bch_params*)pp;
  // TODO:
  // 1) compute generator g(x) from minimal polynomials of α, α^3, ..., α^{2t-1}
  // 2) divide message polynomial x^(deg g)*M(x) by g(x), append remainder
  // 3) stream via bitio
  return CODECTK_ENOTSUP; // lift when implemented
}

static codectk_err bch_decode(const void *pp, const uint8_t *in, size_t in_bits,
                              uint8_t *out, size_t *out_bits, size_t *corr){
  const bch_params *P=(const bch_params*)pp;
  // TODO:
  // 1) syndromes S1..S_{2t}
  // 2) Berlekamp–Massey to get Λ(x) and Ω(x)
  // 3) Chien search to find error locations (positions where Λ(α^{-i})=0)
  // 4) Forney to compute magnitudes (binary BCH → all 1), flip bits
  return CODECTK_ENOTSUP;
}

static const codectk_codec BCH = {
  .name="bch",
  .encode=bch_encode, .decode=bch_decode
};
const codectk_codec* bch_codec(void){ return &BCH; }
