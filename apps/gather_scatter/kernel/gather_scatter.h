#ifndef GATHER_SCATTER_H
#define GATHER_SCATTER_H

#include <riscv_vector.h>
#include <stdbool.h>


void gather_cpu(const int n_k, const int n_in, const int c,
                const float *in_feat, float *out_feat, const int *kmap,
                const bool transpose);

void scatter_cpu(const int n_k, const int n_in, const int c,
                 const float *in_feat, float *out_feat, const int *kmap,
                 const bool transpose);

void gather_rvv(const int n_k, const int n_in, const int c,
                const float *in_feat, float *out_feat, const int *kmap,
                const bool transpose);

void scatter_rvv(const int n_k, const int n_in, const int c,
                 const float *in_feat, float *out_feat, const int *kmap,
                 const bool transpose);

void vectorized_fill(int* arr, size_t n, int value);

#endif