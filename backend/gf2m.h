#pragma once
#include <stdint.h>
typedef struct {
  unsigned m;
  const uint16_t *alog; // size (1<<m), alog[log[x]] = x
  const uint16_t *log;  // log[0] undefined; guard with 0 checks
  uint16_t prim;        // primitive element α (generator)
  uint16_t mod_poly;    // for polynomial basis (optional)
} gf2m_ctx;

static inline uint16_t gf2m_add(uint16_t a, uint16_t b){ return a ^ b; } // XOR

// C fallback (log/alog); can be swapped by ASM
uint16_t gf2m_mul(const gf2m_ctx *ctx, uint16_t a, uint16_t b);
uint16_t gf2m_inv(const gf2m_ctx *ctx, uint16_t a);
uint16_t gf2m_sqr(const gf2m_ctx *ctx, uint16_t a);

// batch/ASM hooks
typedef struct {
  uint16_t (*mul)(const gf2m_ctx*, uint16_t, uint16_t);
  uint16_t (*inv)(const gf2m_ctx*, uint16_t);
  uint16_t (*sqr)(const gf2m_ctx*, uint16_t);
} gf2m_vtbl;
extern gf2m_vtbl gf2m_backend; // set at init to C or ASM
