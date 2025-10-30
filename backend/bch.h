#pragma once
#include "codectk.h"
#include <stddef.h>
typedef struct {
  unsigned m;     // 2<=m<=12 (start with small m)
  unsigned t;     // capability
  // precomputed tables (optional):
  uint16_t *alog, *log;
} bch_params;

const codectk_codec* bch_codec(void);
