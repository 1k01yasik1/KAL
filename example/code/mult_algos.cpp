#include "mult_algos.h"
#include <vector>

Matrix mult_standard(const Matrix &A, const Matrix &B) {
  int n = A.size(), m = B[0].size(), k = B.size();
  Matrix C(n, std::vector<int>(m, 0));
  for (int i = 0; i < n; ++i)
    for (int j = 0; j < m; ++j)
      for (int t = 0; t < k; ++t)
        C[i][j] += A[i][t] * B[t][j];
  return C;
}

Matrix mult_vinograd(const Matrix &A, const Matrix &B) {
  int n = A.size(), m = B[0].size(), k = B.size();
  Matrix C(n, std::vector<int>(m, 0));
  std::vector<int> row_factor(n, 0), col_factor(m, 0);
  for (int i = 0; i < n; ++i)
    for (int t = 0; t < k / 2; ++t)
      row_factor[i] += A[i][2 * t] * A[i][2 * t + 1];
  for (int j = 0; j < m; ++j)
    for (int t = 0; t < k / 2; ++t)
      col_factor[j] += B[2 * t][j] * B[2 * t + 1][j];
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      C[i][j] = -row_factor[i] - col_factor[j];
      for (int t = 0; t < k / 2; ++t)
        C[i][j] +=
            (A[i][2 * t] + B[2 * t + 1][j]) * (A[i][2 * t + 1] + B[2 * t][j]);
    }
  }
  if (k % 2 == 1) {
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < m; ++j)
        C[i][j] += A[i][k - 1] * B[k - 1][j];
  }
  return C;
}

Matrix mult_vinograd_opt(const Matrix &A, const Matrix &B) {
  int n = A.size(), m = B[0].size(), k = B.size();
  Matrix C(n, std::vector<int>(m, 0));
  std::vector<int> row_factor(n, 0), col_factor(m, 0);
  int k2 = k / 2;
  for (int i = 0; i < n; ++i) {
    int sum = 0;
    for (int t = 0; t < k2; ++t)
      sum += A[i][2 * t] * A[i][2 * t + 1];
    row_factor[i] = sum;
  }
  for (int j = 0; j < m; ++j) {
    int sum = 0;
    for (int t = 0; t < k2; ++t)
      sum += B[2 * t][j] * B[2 * t + 1][j];
    col_factor[j] = sum;
  }
  for (int i = 0; i < n; ++i) {
    for (int j = 0; j < m; ++j) {
      int temp = -row_factor[i] - col_factor[j];
      for (int t = 0; t < k2; ++t)
        temp +=
            (A[i][2 * t] + B[2 * t + 1][j]) * (A[i][2 * t + 1] + B[2 * t][j]);
      C[i][j] = temp;
    }
  }
  if (k % 2 == 1) {
    int last = k - 1;
    for (int i = 0; i < n; ++i)
      for (int j = 0; j < m; ++j)
        C[i][j] += A[i][last] * B[last][j];
  }
  return C;
}
