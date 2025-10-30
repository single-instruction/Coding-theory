// src/goppa.c  (structure; finish algebra in poly/gf2m)
#include "goppa.h"
#include "gf2m.h"
#include "poly.h"
#include "bitio.h"

static codectk_err goppa_encode(const void *pp, const uint8_t *in, size_t in_bits,
                                uint8_t *out, size_t *out_bits){
  const goppa_params *P=(const goppa_params*)pp;
  // Systematic encoding via parity-check H built from (L,g)
  // H_{j,i} = L_i^{j} / g(L_i) for j=0..t-1 (over GF(2^m)) → binary expansion rows
  // TODO: build H once, then compute parity bits = H * message mod 2
  return CODECTK_ENOTSUP;
}

static codectk_err goppa_decode(const void *pp, const uint8_t *in, size_t in_bits,
                                uint8_t *out, size_t *out_bits, size_t *corr){
  const goppa_params *P=(const goppa_params*)pp;
  // Patterson:
  // 1) compute S(x) = Σ r_i/(x - L_i) mod g(x)  where r is received vector
  // 2) T(x) = inv(S) mod g
  // 3) find a(x), b(x) with a^2 + x b^2 ≡ T (quadratic congruence mod g)
  // 4) recover error locator; evaluate at L to locate errors; flip bits
  return CODECTK_ENOTSUP;
}

static const codectk_codec GOPPA = {
  .name="goppa",
  .encode=goppa_encode, .decode=goppa_decode
};
const codectk_codec* goppa_codec(void){ return &GOPPA; }
