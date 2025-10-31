// include/hamming.h

// runtime m → (n,k) = (2^m−1, 2^m−1−m)
// encodes/decodes streams of k-bit blocks into n-bit blocks, corrects 1-bit errors

#pragma once
#include "codectk.h"
typedef struct { unsigned m; } hamming_params;
const codectk_codec* hamming_codec(void);
