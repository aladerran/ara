#include <stdlib.h>
#include <stdio.h>

#include "hash.h"


void cpu_hash_wrapper(const int N, const int *data, uint32_t *out) {
    for (int i = 0; i < N; i++) {
        uint32_t hash = 2166136261U; // 32-bit FNV offset basis
        for (int j = 0; j < 3; j++) {
            hash ^= (unsigned int)data[4 * i + j];
            hash *= 16777619U; // 32-bit FNV prime
        }
        hash = (hash >> 28) ^ (hash & 0xFFFFFFF);
        out[i] = hash;
    }
}

void hash_cpu(const int *idx, uint32_t *out, const int N) {
    cpu_hash_wrapper(N, idx, out);
}


void rvv_hash_wrapper(const int N, const int *data, uint32_t *out) {
    const uint32_t FNV_offset_basis = 2166136261U;
    const uint32_t FNV_prime = 16777619U;

    size_t vlmax = vsetvlmax_e32m1();

    uint32_t tempX[vlmax], tempY[vlmax], tempZ[vlmax];

    for (int idx = 0; idx < N; idx += vlmax) {

        size_t vl = vsetvl_e32m1(N - idx);

        vuint32m1_t vhash = vmv_v_x_u32m1(FNV_offset_basis, vl);

        for (int i = 0; i < vl; i++) {
            tempX[i] = data[4*(idx+i)];
            tempY[i] = data[4*(idx+i) + 1];
            tempZ[i] = data[4*(idx+i) + 2];
        }

        for (int j = 0; j < 3; j++) {
            uint32_t *temp = (j == 0) ? tempX : (j == 1) ? tempY : tempZ;
            vuint32m1_t vdata = vle32_v_u32m1(temp, vl); 

            vhash = vxor_vv_u32m1(vhash, vdata, vl);
            
            vhash = vmul_vx_u32m1(vhash, FNV_prime, vl);
        }

        vuint32m1_t vhash_shr = vsrl_vx_u32m1(vhash, 28, vl);
        vuint32m1_t vhash_masked = vand_vx_u32m1(vhash, 0x0FFFFFFF, vl);
        vhash = vxor_vv_u32m1(vhash_shr, vhash_masked, vl);

        vse32_v_u32m1(&out[idx], vhash, vl);
    }
}


void hash_rvv(const int *idx, uint32_t *out, const int N) {
    rvv_hash_wrapper(N, idx, out);
}


/*=========================================================================================*/

void cpu_kernel_hash_wrapper(const int N, const int K, const int *data,
                             const int *kernel_offset, int64_t *out) {
    for (int k = 0; k < K; k++) {
        for (int i = 0; i < N; i++) {
            int cur_coord[4];
            for (int j = 0; j < 3; j++) {
                cur_coord[j] = data[i * 4 + j] + kernel_offset[k * 3 + j];
            }
            cur_coord[3] = data[i * 4 + 3];
            uint64_t hash = 14695981039346656037UL;
            for (int j = 0; j < 4; j++) {
                hash ^= (unsigned int)cur_coord[j];
                hash *= 1099511628211UL;
            }
            hash = (hash >> 60) ^ (hash & 0xFFFFFFFFFFFFFFF);
            out[k * N + i] = hash;
        }
    }
}

void kernel_hash_cpu(const int *idx, const int *kernel_offset,
                           uint32_t *out, const int N, const int K, const int B) {
    for (int b = 0; b < B; b++) {
        cpu_kernel_hash_wrapper(N, K, idx + b * N * 4, kernel_offset, out + b * N * K);
    }
}