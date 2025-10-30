#pragma once
#include <stdio.h>
#include <stdint.h>

typedef struct { uint8_t buf; int bitpos; uint8_t *p, *end; } bitw_t;
typedef struct { int buf, bitpos; const uint8_t *p, *end; } bitr_t;

static inline void bitw_init(bitw_t *w, uint8_t *out, size_t out_bytes)
{ w->buf=0; w->bitpos=0; w->p=out; w->end=out+out_bytes; }

static inline int bitw_put(bitw_t *w, unsigned b){
  if(w->p>=w->end) return -1;
  w->buf |= (b & 1u) << w->bitpos;
  if(++w->bitpos==8){ *w->p++ = w->buf; w->buf=0; w->bitpos=0; }
  return 0;
}
static inline int bitw_flush(bitw_t *w){
  if(w->bitpos){ if(w->p>=w->end) return -1; *w->p++ = w->buf; w->buf=0; w->bitpos=0; }
  return 0;
}

static inline void bitr_init(bitr_t *r, const uint8_t *in, size_t in_bytes)
{ r->buf=-1; r->bitpos=8; r->p=in; r->end=in+in_bytes; }

static inline int bitr_get(bitr_t *r){
  if(r->bitpos==8){ if(r->p>=r->end) return -1; r->buf=*r->p++; r->bitpos=0; }
  int b = (r->buf >> r->bitpos) & 1; r->bitpos++; return b;
}
