#include "matrix_utils.h"
#include <random>

Matrix make_matrix(int n, int m) { return Matrix(n, std::vector<int>(m, 0)); }

Matrix random_matrix(int n, int m) {
  Matrix mat = make_matrix(n, m);
  std::random_device rd;
  std::mt19937 gen(rd());
  std::uniform_int_distribution<> dis(0, 9);
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < m; ++j)
      mat[i][j] = dis(gen);
  return mat;
}

Matrix input_matrix(int n, int m) {
  Matrix mat = make_matrix(n, m);
  std::cout << "\nВведите матрицу " << n << "x" << m << " построчно:\n";
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < m; ++j)
      std::cin >> mat[i][j];
  return mat;
}

void print_matrix(const Matrix &mat) {
  for (const auto &row : mat) {
    for (int val : row)
      std::cout << val << " ";
    std::cout << "\n";
  }
}
