#pragma once
#include "codectk.h"
#include <stddef.h>
typedef struct {
  unsigned m;           // GF(2^m)
  unsigned t;           // deg g
  size_t   n;           // |L|
  const uint16_t *L;    // support set in [0..2^m-1], size n
  // g(x) coefficients in GF(2^m) (t+1 coeffs)
  const uint16_t *g;    // g[0] + g[1] x + ... + g[t] x^t
  // field tables:
  const uint16_t *alog; // size 2^m
  const uint16_t *log;  // size 2^m
} goppa_params;
const codectk_codec* goppa_codec(void);
