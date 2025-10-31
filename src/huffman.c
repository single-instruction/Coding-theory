/**
 * huffman.c - Dynamic Huffman coding implementation
 *
 * Implements adaptive Huffman encoding with frequency analysis.
 * Format: HUF1 magic + frequency table + compressed data.
 */

#include "../include/huffman.h"
#include "../include/bitio.h"
#include <stdlib.h>
#include <string.h>

#define HUF_NSYMBOLS 257  /* 256 bytes + EOF marker */
#define HUF_EOF_SYMBOL 256

/**
 * Huffman tree node structure.
 */
typedef struct huf_node {
  unsigned freq;
  int symbol;  /* -1 for internal nodes */
  struct huf_node *left, *right;
} huf_node;

/**
 * Min-heap for building Huffman tree.
 */
typedef struct {
  huf_node **nodes;
  int size;
  int capacity;
} min_heap;

/**
 * Code table entry: bit code and length.
 */
typedef struct {
  uint64_t code;
  int length;
} code_entry;

/* Min-heap operations */

static min_heap* heap_create(int capacity) {
  min_heap *h = (min_heap*)malloc(sizeof(min_heap));
  if (!h) return NULL;

  h->nodes = (huf_node**)malloc((size_t)capacity * sizeof(huf_node*));
  if (!h->nodes) {
    free(h);
    return NULL;
  }

  h->size = 0;
  h->capacity = capacity;
  return h;
}

static void heap_free(min_heap *h) {
  if (h) {
    free(h->nodes);
    free(h);
  }
}

static void heap_swap(huf_node **a, huf_node **b) {
  huf_node *temp = *a;
  *a = *b;
  *b = temp;
}

static void heap_sift_down(min_heap *h, int idx) {
  int smallest = idx;
  int left = 2 * idx + 1;
  int right = 2 * idx + 2;

  if (left < h->size && h->nodes[left]->freq < h->nodes[smallest]->freq) {
    smallest = left;
  }
  if (right < h->size && h->nodes[right]->freq < h->nodes[smallest]->freq) {
    smallest = right;
  }

  if (smallest != idx) {
    heap_swap(&h->nodes[idx], &h->nodes[smallest]);
    heap_sift_down(h, smallest);
  }
}

static void heap_sift_up(min_heap *h, int idx) {
  if (idx == 0) return;

  int parent = (idx - 1) / 2;
  if (h->nodes[idx]->freq < h->nodes[parent]->freq) {
    heap_swap(&h->nodes[idx], &h->nodes[parent]);
    heap_sift_up(h, parent);
  }
}

static void heap_insert(min_heap *h, huf_node *node) {
  if (h->size >= h->capacity) return;

  h->nodes[h->size] = node;
  heap_sift_up(h, h->size);
  h->size++;
}

static huf_node* heap_extract_min(min_heap *h) {
  if (h->size == 0) return NULL;

  huf_node *min_node = h->nodes[0];
  h->nodes[0] = h->nodes[h->size - 1];
  h->size--;
  heap_sift_down(h, 0);

  return min_node;
}

/* Huffman tree construction */

static huf_node* create_node(int symbol, unsigned freq) {
  huf_node *node = (huf_node*)malloc(sizeof(huf_node));
  if (!node) return NULL;

  node->symbol = symbol;
  node->freq = freq;
  node->left = NULL;
  node->right = NULL;

  return node;
}

static void free_tree(huf_node *root) {
  if (!root) return;

  free_tree(root->left);
  free_tree(root->right);
  free(root);
}

/**
 * Build Huffman tree from frequency table using min-heap.
 */
static huf_node* build_tree(const unsigned *freq) {
  min_heap *h = heap_create(HUF_NSYMBOLS * 2);
  if (!h) return NULL;

  /* Create leaf nodes for symbols with nonzero frequency */
  for (int i = 0; i < HUF_NSYMBOLS; i++) {
    if (freq[i] > 0) {
      huf_node *node = create_node(i, freq[i]);
      if (node) heap_insert(h, node);
    }
  }

  /* Handle special case: only one symbol */
  if (h->size == 1) {
    huf_node *single = heap_extract_min(h);
    huf_node *root = create_node(-1, single->freq);
    root->left = single;
    root->right = NULL;
    heap_free(h);
    return root;
  }

  /* Build tree by combining two minimum nodes repeatedly */
  while (h->size > 1) {
    huf_node *left = heap_extract_min(h);
    huf_node *right = heap_extract_min(h);

    huf_node *parent = create_node(-1, left->freq + right->freq);
    if (!parent) {
      /* Cleanup on allocation failure */
      free_tree(left);
      free_tree(right);
      while (h->size > 0) {
        free_tree(heap_extract_min(h));
      }
      heap_free(h);
      return NULL;
    }

    parent->left = left;
    parent->right = right;
    heap_insert(h, parent);
  }

  huf_node *root = heap_extract_min(h);
  heap_free(h);

  return root;
}

/* Code table generation */

static void build_codes_recursive(huf_node *node, code_entry *table,
                                   uint64_t current_code, int depth) {
  if (!node) return;

  if (node->symbol >= 0) {
    /* Leaf node: store code */
    table[node->symbol].code = current_code;
    table[node->symbol].length = depth;
  } else {
    /* Internal node: recurse */
    if (node->left) {
      build_codes_recursive(node->left, table, (current_code << 1) | 0, depth + 1);
    }
    if (node->right) {
      build_codes_recursive(node->right, table, (current_code << 1) | 1, depth + 1);
    }
  }
}

static void build_code_table(huf_node *root, code_entry *table) {
  memset(table, 0, HUF_NSYMBOLS * sizeof(code_entry));

  if (!root) return;

  /* Handle single-symbol tree */
  if (root->left && !root->right) {
    table[root->left->symbol].code = 0;
    table[root->left->symbol].length = 1;
    return;
  }

  build_codes_recursive(root, table, 0, 0);
}

/* Encoder */

static codectk_err huf_encode(const void *pp, const uint8_t *in, size_t in_bits,
                                uint8_t *out, size_t *out_bits) {
  (void)pp; /* params unused for dynamic Huffman */

  size_t in_bytes = (in_bits + 7) / 8;
  if (in_bytes == 0) return CODECTK_EINVAL;

  /* Build frequency table */
  unsigned freq[HUF_NSYMBOLS] = {0};
  for (size_t i = 0; i < in_bytes; i++) {
    freq[in[i]]++;
  }
  freq[HUF_EOF_SYMBOL] = 1; /* EOF marker */

  /* Build Huffman tree */
  huf_node *root = build_tree(freq);
  if (!root) return CODECTK_ENOMEM;

  /* Generate code table */
  code_entry codes[HUF_NSYMBOLS];
  build_code_table(root, codes);

  /* Write header directly to output buffer: magic "HUF1" + frequency table */
  size_t header_size = 4 + HUF_NSYMBOLS * 4;
  size_t out_bytes = (*out_bits) / 8;

  if (out_bytes < header_size) {
    free_tree(root);
    return CODECTK_ENOMEM;
  }

  uint8_t *p = out;

  /* Write magic */
  const char magic[4] = {'H', 'U', 'F', '1'};
  memcpy(p, magic, 4);
  p += 4;

  /* Write frequency table (4 bytes per symbol, 257 symbols) */
  for (int i = 0; i < HUF_NSYMBOLS; i++) {
    uint32_t f = (uint32_t)freq[i];
    *p++ = (uint8_t)(f & 0xff);
    *p++ = (uint8_t)((f >> 8) & 0xff);
    *p++ = (uint8_t)((f >> 16) & 0xff);
    *p++ = (uint8_t)((f >> 24) & 0xff);
  }

  /* Initialize bitio writer starting after header */
  bitw_t W;
  bitw_init(&W, p, out_bytes - header_size);

  /* Encode data */
  for (size_t i = 0; i < in_bytes; i++) {
    uint8_t symbol = in[i];
    code_entry *ce = &codes[symbol];

    for (int bit = ce->length - 1; bit >= 0; bit--) {
      int b = (ce->code >> bit) & 1;
      if (bitw_put(&W, (unsigned)b)) {
        free_tree(root);
        return CODECTK_ENOMEM;
      }
    }
  }

  /* Write EOF code */
  code_entry *eof_ce = &codes[HUF_EOF_SYMBOL];
  for (int bit = eof_ce->length - 1; bit >= 0; bit--) {
    int b = (eof_ce->code >> bit) & 1;
    if (bitw_put(&W, (unsigned)b)) {
      free_tree(root);
      return CODECTK_ENOMEM;
    }
  }

  if (bitw_flush(&W)) {
    free_tree(root);
    return CODECTK_ENOMEM;
  }

  /* Total size: header + compressed data */
  *out_bits = (header_size + (size_t)(W.p - p)) * 8;

  free_tree(root);
  return CODECTK_OK;
}

/* Decoder */

static codectk_err huf_decode(const void *pp, const uint8_t *in, size_t in_bits,
                                uint8_t *out, size_t *out_bits, size_t *corr) {
  (void)pp;
  (void)corr; /* No error correction in Huffman */

  size_t in_bytes = (in_bits + 7) / 8;
  if (in_bytes < 4 + HUF_NSYMBOLS * 4) return CODECTK_EINVAL;

  /* Verify magic */
  if (in[0] != 'H' || in[1] != 'U' || in[2] != 'F' || in[3] != '1') {
    return CODECTK_EINVAL;
  }

  const uint8_t *p = in + 4;

  /* Read frequency table */
  unsigned freq[HUF_NSYMBOLS];
  for (int i = 0; i < HUF_NSYMBOLS; i++) {
    freq[i] = (unsigned)p[0] | ((unsigned)p[1] << 8) |
              ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
    p += 4;
  }

  /* Rebuild Huffman tree */
  huf_node *root = build_tree(freq);
  if (!root) return CODECTK_ENOMEM;

  /* Decode data */
  bitr_t R;
  bitr_init(&R, p, in_bytes - (4 + HUF_NSYMBOLS * 4));

  bitw_t W;
  bitw_init(&W, out, (*out_bits) / 8);

  huf_node *node = root;
  int done = 0;

  while (!done) {
    int b = bitr_get(&R);
    if (b < 0) break; /* End of input */

    /* Navigate tree */
    if (b == 0) {
      node = node->left;
    } else {
      node = node->right;
    }

    if (!node) {
      /* Malformed stream */
      free_tree(root);
      return CODECTK_EDECODE;
    }

    /* Leaf node? */
    if (node->symbol >= 0) {
      if (node->symbol == HUF_EOF_SYMBOL) {
        done = 1;
      } else {
        /* Write complete byte to output */
        if (W.p >= W.end) {
          free_tree(root);
          return CODECTK_ENOMEM;
        }
        *W.p++ = (uint8_t)node->symbol;
      }

      node = root; /* Reset to root for next symbol */
    }
  }

  /* Output is complete bytes, no need to flush */
  *out_bits = ((size_t)(W.p - out)) * 8;

  free_tree(root);
  return CODECTK_OK;
}

static const codectk_codec HUF = {
  .name = "huffman",
  .encode = huf_encode,
  .decode = huf_decode
};

const codectk_codec* huffman_codec(void) {
  return &HUF;
}
