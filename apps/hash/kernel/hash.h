#ifndef HASH_H
#define HASH_H

#include <riscv_vector.h>


void cpu_hash_wrapper(const int N, const int *data, uint32_t *out);

void hash_cpu(const int *idx, uint32_t *out, const int N);


void rvv_hash_wrapper(const int N, const int *data, uint32_t *out);

void hash_rvv(const int *idx, uint32_t *out, const int N);


void cpu_kernel_hash_wrapper(const int N, const int K, const int *data,
                             const int *kernel_offset, uint32_t *out);

void kernel_hash_cpu(const int *idx, const int *kernel_offset,
                           uint32_t *out, const int N, const int K);

void rvv_kernel_hash_wrapper(const int N, const int K, const int *data,
                             const int *kernel_offset, uint32_t *out);

void kernel_hash_rvv(const int *idx, const int *kernel_offset,
                            uint32_t *out, const int N, const int K);

// void hash_query_cpu(const uint32_t* hash_query, const uint32_t* hash_target,
//                     const uint32_t* idx_target, uint32_t* out, const int n, const int n1);

void hash_query_rvv(const uint32_t* hash_query, const uint32_t* hash_target,
                    const uint32_t* idx_target, uint32_t* out, const int n, const int n1);

#endif // HASH_H