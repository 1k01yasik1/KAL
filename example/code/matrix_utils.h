#pragma once
#include <iostream>
#include <vector>

typedef std::vector<std::vector<int>> Matrix;

Matrix make_matrix(int n, int m);

Matrix random_matrix(int n, int m);

Matrix input_matrix(int n, int m);

void print_matrix(const Matrix &mat);
