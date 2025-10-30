#include "codectk.h"
#include "hamming.h"
#include "bch.h"
#include "goppa.h"
#include "huffman.h"
#include <string.h>

const codectk_codec* codectk_get(const char *name){
  if(!strcmp(name,"hamming")) return hamming_codec();
  if(!strcmp(name,"bch"))     return bch_codec();
  if(!strcmp(name,"goppa"))   return goppa_codec();
  if(!strcmp(name,"huffman")) return huffman_codec();
  return NULL;
}
