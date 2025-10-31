/**
 * test_main.c - Main test runner for codectk library
 *
 * Coordinates all unit tests and reports results.
 */

#include <stdio.h>
#include <stdlib.h>

/* Forward declarations of test suites */
extern int test_bitio_suite(void);
extern int test_gf2_suite(void);
extern int test_gf2m_suite(void);
extern int test_poly_suite(void);
extern int test_hamming_suite(void);
extern int test_huffman_suite(void);
extern int test_bch_suite(void);

int main(void) {
  int total_failures = 0;

  printf("==============================================\n");
  printf("Coding Theory Toolkit - Test Suite\n");
  printf("==============================================\n\n");

  printf("Running bitio tests.\n");
  total_failures += test_bitio_suite();
  printf("\n");

  printf("Running GF(2) tests.\n");
  total_failures += test_gf2_suite();
  printf("\n");

  printf("Running GF(2^m) tests.\n");
  total_failures += test_gf2m_suite();
  printf("\n");

  printf("Running polynomial tests.\n");
  total_failures += test_poly_suite();
  printf("\n");

  printf("Running Hamming code tests.\n");
  total_failures += test_hamming_suite();
  printf("\n");

  printf("Running Huffman coding tests.\n");
  total_failures += test_huffman_suite();
  printf("\n");

  // printf("Running BCH code tests...\n");
  // total_failures += test_bch_suite();
  // printf("\n");

  printf("==============================================\n");
  if (total_failures == 0) {
    printf("ALL TESTS PASSED\n");
  } else {
    printf("FAILURES: %d\n", total_failures);
  }
  printf("==============================================\n");

  return (total_failures == 0) ? 0 : 1;
}
