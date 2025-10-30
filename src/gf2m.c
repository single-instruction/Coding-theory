#include "gf2m.h"
gf2m_vtbl gf2m_backend;

uint16_t gf2m_mul_c(const gf2m_ctx *ctx, uint16_t a, uint16_t b){
  if(!a || !b) return 0;
  unsigned la = ctx->log[a], lb = ctx->log[b];
  return ctx->alog[(la + lb) % ((1u<<ctx->m)-1u)];
}
uint16_t gf2m_inv_c(const gf2m_ctx *ctx, uint16_t a){
  if(!a) return 0; // caller guards
  unsigned la = ctx->log[a];
  return ctx->alog[((1u<<ctx->m)-1u - la)];
}
uint16_t gf2m_sqr_c(const gf2m_ctx *ctx, uint16_t a){
  // multiply by itself; can be optimized via Frobenius map tables
  return gf2m_mul_c(ctx,a,a);
}

// default backend = C
__attribute__((constructor))
static void init_backend(void){
  gf2m_backend.mul = gf2m_mul_c;
  gf2m_backend.inv = gf2m_inv_c;
  gf2m_backend.sqr = gf2m_sqr_c;
}
