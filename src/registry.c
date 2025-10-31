#include "../include/codectk.h"
#include "../include/hamming.h"
#include "../include/bch.h"
#include "../include/goppa.h"
#include "../include/huffman.h"
#include <string.h>

const codectk_codec* codectk_get(const char *name) {
  if (!strcmp(name, "hamming")) return hamming_codec();
  if (!strcmp(name, "bch"))     return bch_codec();
  if (!strcmp(name, "goppa"))   return goppa_codec();
  if (!strcmp(name, "huffman")) return huffman_codec();
  return NULL;
}

const char* codectk_strerror(codectk_err err) {
  switch (err) {
    case CODECTK_OK:
      return "Success";
    case CODECTK_EINVAL:
      return "Invalid argument or parameters";
    case CODECTK_ENOMEM:
      return "Out of memory or buffer too small";
    case CODECTK_EDECODE:
      return "Decoding failed: too many errors to correct";
    case CODECTK_ENOTSUP:
      return "Operation not supported or not yet implemented";
    default:
      return "Unknown error code";
  }
}
