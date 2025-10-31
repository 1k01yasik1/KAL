#include "../mult_algos.h"
#include <iostream>
#include <vector>

int main() {
  Matrix A = {{1, 2}, {3, 4}};
  Matrix B = {{5, 6}, {7, 8}};
  Matrix expected = {{19, 22}, {43, 50}};

  bool ok1 = (mult_standard(A, B) == expected);
  std::cout << (ok1 ? "TEST 1 PASSED\n" : "TEST 1 FAILED\n");

  bool ok2 = (mult_vinograd(A, B) == expected);
  std::cout << (ok2 ? "TEST 2 PASSED\n" : "TEST 2 FAILED\n");

  bool ok3 = (mult_vinograd_opt(A, B) == expected);
  std::cout << (ok3 ? "TEST 3 PASSED\n" : "TEST 3 FAILED\n");

  return 0;
}
