/**
 * pipe.c - Command-line tool for codec demonstration
 *
 * Usage:
 *   pipe encode <codec> <input> <output>
 *   pipe decode <codec> <input> <output>
 *
 * Example:
 *   echo "Hello" > input.txt
 *   ./pipe encode huffman input.txt encoded.bin
 *   ./pipe decode huffman encoded.bin output.txt
 */

#include "../include/codectk.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE_SIZE (10 * 1024 * 1024) /* 10 MB limit */

static int read_file(const char *path, uint8_t **data, size_t *size) {
  FILE *f = fopen(path, "rb");
  if (!f) {
    fprintf(stderr, "Error: cannot open input file '%s'\n", path);
    return -1;
  }

  fseek(f, 0, SEEK_END);
  long fsize = ftell(f);
  fseek(f, 0, SEEK_SET);

  if (fsize < 0 || fsize > MAX_FILE_SIZE) {
    fprintf(stderr, "Error: file size invalid or too large\n");
    fclose(f);
    return -1;
  }

  *data = (uint8_t*)malloc((size_t)fsize);
  if (!*data) {
    fprintf(stderr, "Error: out of memory\n");
    fclose(f);
    return -1;
  }

  size_t nread = fread(*data, 1, (size_t)fsize, f);
  fclose(f);

  if (nread != (size_t)fsize) {
    fprintf(stderr, "Error: failed to read entire file\n");
    free(*data);
    return -1;
  }

  *size = (size_t)fsize;
  return 0;
}

static int write_file(const char *path, const uint8_t *data, size_t size) {
  FILE *f = fopen(path, "wb");
  if (!f) {
    fprintf(stderr, "Error: cannot open output file '%s'\n", path);
    return -1;
  }

  size_t nwritten = fwrite(data, 1, size, f);
  fclose(f);

  if (nwritten != size) {
    fprintf(stderr, "Error: failed to write entire file\n");
    return -1;
  }

  return 0;
}

static void print_usage(const char *prog) {
  printf("Usage:\n");
  printf("  %s encode <codec> <input> <output>\n", prog);
  printf("  %s decode <codec> <input> <output>\n", prog);
  printf("\n");
  printf("Available codecs:\n");
  printf("  huffman - Huffman source coding\n");
  printf("  hamming - Hamming error-correcting code (requires m parameter)\n");
  printf("\n");
  printf("Examples:\n");
  printf("  %s encode huffman input.txt encoded.bin\n", prog);
  printf("  %s decode huffman encoded.bin output.txt\n", prog);
}

int main(int argc, char **argv) {
  if (argc < 5) {
    print_usage(argv[0]);
    return 1;
  }

  const char *operation = argv[1];
  const char *codec_name = argv[2];
  const char *input_path = argv[3];
  const char *output_path = argv[4];

  /* Get codec */
  const codectk_codec *codec = codectk_get(codec_name);
  if (!codec) {
    fprintf(stderr, "Error: unknown codec '%s'\n", codec_name);
    return 1;
  }

  /* Read input */
  uint8_t *input_data = NULL;
  size_t input_size = 0;
  if (read_file(input_path, &input_data, &input_size) != 0) {
    return 1;
  }

  /* Allocate output buffer (generous size) */
  size_t output_capacity = input_size * 10 + 10000;
  uint8_t *output_data = (uint8_t*)malloc(output_capacity);
  if (!output_data) {
    fprintf(stderr, "Error: out of memory\n");
    free(input_data);
    return 1;
  }

  size_t output_bits = output_capacity * 8;
  codectk_err err;

  if (strcmp(operation, "encode") == 0) {
    printf("Encoding with %s...\n", codec_name);
    err = codec->encode(NULL, input_data, input_size * 8, output_data, &output_bits);

    if (err != CODECTK_OK) {
      fprintf(stderr, "Error: encode failed: %s\n", codectk_strerror(err));
      free(input_data);
      free(output_data);
      return 1;
    }

    size_t output_bytes = (output_bits + 7) / 8;
    printf("Encoded: %zu bytes -> %zu bytes (%.2f%% of original)\n",
           input_size, output_bytes,
           100.0 * output_bytes / input_size);

    if (write_file(output_path, output_data, output_bytes) != 0) {
      free(input_data);
      free(output_data);
      return 1;
    }

  } else if (strcmp(operation, "decode") == 0) {
    printf("Decoding with %s...\n", codec_name);

    size_t num_corrected = 0;
    err = codec->decode(NULL, input_data, input_size * 8, output_data,
                         &output_bits, &num_corrected);

    if (err != CODECTK_OK) {
      fprintf(stderr, "Error: decode failed: %s\n", codectk_strerror(err));
      free(input_data);
      free(output_data);
      return 1;
    }

    size_t output_bytes = (output_bits + 7) / 8;
    printf("Decoded: %zu bytes -> %zu bytes\n", input_size, output_bytes);

    if (num_corrected > 0) {
      printf("Corrected %zu errors\n", num_corrected);
    }

    if (write_file(output_path, output_data, output_bytes) != 0) {
      free(input_data);
      free(output_data);
      return 1;
    }

  } else {
    fprintf(stderr, "Error: unknown operation '%s' (use 'encode' or 'decode')\n",
            operation);
    free(input_data);
    free(output_data);
    return 1;
  }

  free(input_data);
  free(output_data);

  printf("Success!\n");
  return 0;
}
