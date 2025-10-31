#pragma once
#include "matrix_utils.h"

Matrix mult_standard(const Matrix &A, const Matrix &B);

Matrix mult_vinograd(const Matrix &A, const Matrix &B);

Matrix mult_vinograd_opt(const Matrix &A, const Matrix &B);
