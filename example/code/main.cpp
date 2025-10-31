#include "matrix_utils.h"
#include "mult_algos.h"
#include <chrono>
#include <fstream>
#include <iostream>

using namespace std;
using namespace std::chrono;

void benchmark(int n1, int m1, int m2, ofstream &fout, int repeats = 100) {
  Matrix A = random_matrix(n1, m1);
  Matrix B = random_matrix(m1, m2);
  auto measure = [&](Matrix (*func)(const Matrix &, const Matrix &),
                     const string &name) {
    double total_time = 0;
    for (int i = 0; i < repeats; ++i) {
      auto start = high_resolution_clock::now();
      Matrix C = func(A, B);
      auto end = high_resolution_clock::now();
      total_time += duration<double, milli>(end - start).count();
    }
    double avg_time = total_time / repeats;
    cout << "Размер " << n1 << "x" << m2 << ", " << name << ": " << avg_time
         << " ms\n";
    fout << n1 << "," << name << "," << avg_time << "\n";
  };
  measure(mult_standard, "Standard");
  measure(mult_vinograd, "Vinograd");
  measure(mult_vinograd_opt, "Vinograd_Opt");
}

int main() {
  int choice;
  cout
      << "Выберите режим:\n1. Ручной ввод\n2. Автоматический замер времени\n> ";
  cin >> choice;
  if (choice == 1) {
    int n1, m1, m2;
    cout
        << "\nВведите размеры матриц: n1 m1 m2 (для умножения n1×m1 * m1×m2): ";
    cin >> n1 >> m1 >> m2;
    Matrix A = input_matrix(n1, m1);
    Matrix B = input_matrix(m1, m2);
    Matrix C = mult_standard(A, B);
    cout << "\nРезультат (Standard):\n";
    print_matrix(C);
    C = mult_vinograd(A, B);
    cout << "\nРезультат (Vinograd):\n";
    print_matrix(C);
    C = mult_vinograd_opt(A, B);
    cout << "\nРезультат (Vinograd_Opt):\n";
    print_matrix(C);
  } else if (choice == 2) {
    int even_sizes[] = {100, 200, 300, 400, 500};
    int odd_sizes[] = {101, 201, 301, 401, 501};
    ofstream fout_best("results_best.csv");
    fout_best << "n,name,time\n";
    cout << "\n===Лучший случай для алгоритма Винограда (чётные размеры)===\n";
    for (int sz : even_sizes) {
      cout << "\n";
      benchmark(sz, sz, sz, fout_best);
    }
    fout_best.close();
    ofstream fout_worst("results_worst.csv");
    fout_worst << "n,name,time\n";
    cout
        << "\n===Худший случай для алгоритма Винограда (нечетные размеры)===\n";
    for (int sz : odd_sizes) {
      cout << "\n";
      benchmark(sz, sz, sz, fout_worst);
    }
    fout_worst.close();
  }
  return 0;
}
