#include "hamming.h"
#include "bitio.h"
#include <string.h>

static inline int is_pow2(unsigned x){ return x && !(x&(x-1)); }
static inline void params(unsigned m, unsigned *n, unsigned *k){ *n=(1u<<m)-1u; *k=*n - m; }

static uint64_t place_data(uint64_t data, unsigned m){
  unsigned n,k; params(m,&n,&k);
  uint64_t cw=0; unsigned di=0;
  for(unsigned pos=1; pos<=n; ++pos){
    if(is_pow2(pos)) continue;
    if((data>>di)&1u) cw |= (1ull<<(pos-1));
    if(++di==k) break;
  }
  return cw;
}
static uint64_t set_parity(uint64_t cw, unsigned m){
  unsigned n,k; params(m,&n,&k);
  for(unsigned p=1; p<=n; p<<=1){
    unsigned par=0;
    for(unsigned pos=1; pos<=n; ++pos)
      if((pos & p) && ((cw>>(pos-1))&1u)) par^=1;
    if(par) cw ^= (1ull<<(p-1));
  }
  return cw;
}
static unsigned syndrome(uint64_t cw, unsigned m){
  unsigned n,k; params(m,&n,&k);
  unsigned s=0, bit=1;
  for(unsigned p=1; p<=n; p<<=1, bit<<=1){
    unsigned par=0;
    for(unsigned pos=1; pos<=n; ++pos)
      if((pos & p) && ((cw>>(pos-1))&1u)) par^=1;
    if(par) s |= bit;
  }
  return s; // 0 = ok, otherwise 1..n
}
static uint64_t extract_data(uint64_t cw, unsigned m){
  unsigned n,k; params(m,&n,&k);
  uint64_t d=0; unsigned di=0;
  for(unsigned pos=1; pos<=n; ++pos){
    if(is_pow2(pos)) continue;
    if((cw>>(pos-1))&1u) d |= (1ull<<di);
    if(++di==k) break;
  }
  return d;
}

static codectk_err h_encode(const void *pp, const uint8_t *in, size_t in_bits, uint8_t *out, size_t *out_bits){
  const hamming_params *p = (const hamming_params*)pp;
  unsigned n,k; params(p->m,&n,&k);
  bitr_t R; bitw_t W; bitr_init(&R,in,(in_bits+7)/8); bitw_init(&W,out, (*out_bits)/8);
  for(size_t done=0; done+ k <= in_bits; done += k){
    uint64_t block=0; for(unsigned i=0;i<k;i++){ int b=bitr_get(&R); if(b<0) return CODECTK_EINVAL; block |= (uint64_t)b<<i; }
    uint64_t cw = set_parity(place_data(block,p->m), p->m);
    for(unsigned i=0;i<n;i++) if(bitw_put(&W, (cw>>i)&1u)) return CODECTK_ENOMEM;
  }
  if(bitw_flush(&W)) return CODECTK_ENOMEM;
  *out_bits = ((size_t)(W.p - out))*8;
  return CODECTK_OK;
}
static codectk_err h_decode(const void *pp, const uint8_t *in, size_t in_bits, uint8_t *out, size_t *out_bits, size_t *corr){
  const hamming_params *p = (const hamming_params*)pp;
  unsigned n,k; params(p->m,&n,&k);
  size_t corrected=0;
  bitr_t R; bitw_t W; bitr_init(&R,in,(in_bits+7)/8); bitw_init(&W,out, (*out_bits)/8);
  for(size_t done=0; done + n <= in_bits; done += n){
    uint64_t cw=0; for(unsigned i=0;i<n;i++){ int b=bitr_get(&R); if(b<0) return CODECTK_EINVAL; if(b) cw|=1ull<<i; }
    unsigned s = syndrome(cw,p->m);
    if(s) { if(s<=n) { cw ^= (1ull<<(s-1)); corrected++; } }
    uint64_t d = extract_data(cw,p->m);
    for(unsigned i=0;i<k;i++) if(bitw_put(&W, (d>>i)&1u)) return CODECTK_ENOMEM;
  }
  if(bitw_flush(&W)) return CODECTK_ENOMEM;
  *out_bits = ((size_t)(W.p - out))*8;
  if(corr) *corr = corrected;
  return CODECTK_OK;
}
static const codectk_codec HAM = {
  .name="hamming", .encode=h_encode, .decode=h_decode
};
const codectk_codec* hamming_codec(void){ return &HAM; }
