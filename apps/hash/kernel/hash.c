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
    // Constants for FNV-1a hash algorithm
    const uint32_t FNV_offset_basis = 2166136261U;
    const uint32_t FNV_prime = 16777619U;

    // Get the maximum vector length for 32-bit integers
    size_t vlmax = vsetvlmax_e32m1();

    // Temporary arrays to hold data
    uint32_t tempX[vlmax], tempY[vlmax], tempZ[vlmax];

    // Loop over the data in chunks of maximum vector length
    for (int idx = 0; idx < N; idx += vlmax) {

        // Set the vector length for this iteration
        size_t vl = vsetvl_e32m1(N - idx);

        // Initialize the hash value
        vuint32m1_t vhash = vmv_v_x_u32m1(FNV_offset_basis, vl);

        // Load data into temporary arrays
        for (int i = 0; i < vl; i++) {
            tempX[i] = data[4*(idx+i)];
            tempY[i] = data[4*(idx+i) + 1];
            tempZ[i] = data[4*(idx+i) + 2];
        }

        // Perform the hash operation for each data element
        for (int j = 0; j < 3; j++) {
            uint32_t *temp = (j == 0) ? tempX : (j == 1) ? tempY : tempZ;
            vuint32m1_t vdata = vle32_v_u32m1(temp, vl); 

            // XOR the hash with the data
            vhash = vxor_vv_u32m1(vhash, vdata, vl);
            
            // Multiply the hash by the FNV prime
            vhash = vmul_vx_u32m1(vhash, FNV_prime, vl);
        }

        // Perform final operations on the hash
        vuint32m1_t vhash_shr = vsrl_vx_u32m1(vhash, 28, vl);
        vuint32m1_t vhash_masked = vand_vx_u32m1(vhash, 0x0FFFFFFF, vl);
        vhash = vxor_vv_u32m1(vhash_shr, vhash_masked, vl);

        // Store the hash in the output array
        vse32_v_u32m1(&out[idx], vhash, vl);
    }
}

void hash_rvv(const int *idx, uint32_t *out, const int N) {
    rvv_hash_wrapper(N, idx, out);
}


/*=========================================================================================*/

void cpu_kernel_hash_wrapper(const int N, const int K, const int *data,
                                   const int *kernel_offset, uint32_t *out) {
    for (int k = 0; k < K; k++) {
        for (int i = 0; i < N; i++) {
            int cur_coord[3];
            for (int j = 0; j < 3; j++) {
                cur_coord[j] = data[i * 4 + j] + kernel_offset[k * 3 + j];
            }
            uint32_t hash = 2166136261U; // 32-bit FNV offset basis
            for (int j = 0; j < 3; j++) {
                hash ^= (unsigned int)cur_coord[j];
                hash *= 16777619U; // 32-bit FNV prime
            }
            hash = (hash >> 28) ^ (hash & 0xFFFFFFF);
            out[k * N + i] = hash;
        }
    }
}

void kernel_hash_cpu(const int *idx, const int *kernel_offset,
                uint32_t *out, const int N, const int K) {
    cpu_kernel_hash_wrapper(N, K, idx, kernel_offset, out);
}


void rvv_kernel_hash_wrapper(const int N, const int K, const int *data,
                             const int *kernel_offset, uint32_t *out) {
    const uint32_t FNV_offset_basis = 2166136261U;
    const uint32_t FNV_prime = 16777619U;

    // Set vector length to the maximum vector length dynamically
    size_t vlmax = vsetvlmax_e32m1();

    uint32_t tempX[vlmax], tempY[vlmax], tempZ[vlmax]; // Temporary arrays for x, y, z values

    for (int i = 0; i < N; i += vlmax) {
        size_t vl = vsetvl_e32m1(N - i); // Adjust vl for the last batch

        // Load data into tempX, tempY, tempZ
        for (size_t j = 0; j < vl; ++j) {
            tempX[j] = data[(i + j) * 4];
            tempY[j] = data[(i + j) * 4 + 1];
            tempZ[j] = data[(i + j) * 4 + 2];
        }

        for (int k = 0; k < K; ++k) {

            // Broadcast current kernel_offset values
            vuint32m1_t vkx = vmv_v_x_u32m1(kernel_offset[k * 3], vl);
            vuint32m1_t vky = vmv_v_x_u32m1(kernel_offset[k * 3 + 1], vl);
            vuint32m1_t vkz = vmv_v_x_u32m1(kernel_offset[k * 3 + 2], vl);

            // Add the kernel_offset values to the data values
            vuint32m1_t vx = vadd_vx_u32m1(vle32_v_u32m1(tempX, vl), kernel_offset[k * 3], vl);
            vuint32m1_t vy = vadd_vx_u32m1(vle32_v_u32m1(tempY, vl), kernel_offset[k * 3 + 1], vl);
            vuint32m1_t vz = vadd_vx_u32m1(vle32_v_u32m1(tempZ, vl), kernel_offset[k * 3 + 2], vl);

            // Initialize the vectorized hash value
            vuint32m1_t vhash = vmv_v_x_u32m1(FNV_offset_basis, vl);

            // Perform the hash-related computation
            vhash = vxor_vv_u32m1(vhash, vx, vl);
            vhash = vmul_vx_u32m1(vhash, FNV_prime, vl);
            vhash = vxor_vv_u32m1(vhash, vy, vl);
            vhash = vmul_vx_u32m1(vhash, FNV_prime, vl);
            vhash = vxor_vv_u32m1(vhash, vz, vl);
            vhash = vmul_vx_u32m1(vhash, FNV_prime, vl);

            // Final hash adjustment
            vhash = vxor_vv_u32m1(vsrl_vx_u32m1(vhash, 28, vl), vand_vx_u32m1(vhash, 0x0FFFFFFF, vl), vl);

            // Store the result in the output array
            vse32_v_u32m1(out + k * N + i, vhash, vl);
        }
    }
}


void kernel_hash_rvv(const int *idx, const int *kernel_offset,
                uint32_t *out, const int N, const int K) {
    rvv_kernel_hash_wrapper(N, K, idx, kernel_offset, out);
}


/*=========================================================================================*/

// void hash_query_cpu(const uint32_t* hash_query, const uint32_t* hash_target,
//                             const uint32_t* idx_target, uint32_t* out, const int n, const int n1) {
//     google::dense_hash_map<uint32_t, uint32_t> hashmap;
//     hashmap.set_empty_key(0);
    
//     for (int idx = 0; idx < n; idx++) {
//         uint32_t key = hash_target[idx];
//         uint32_t val = idx_target[idx] + 1;
//         hashmap.insert(std::make_pair(key, val));
//     }
//     for (int idx = 0; idx < n1; idx++) {
//         uint32_t key = hash_query[idx];
//         google::dense_hash_map<uint32_t, uint32_t>::iterator iter = hashmap.find(key);
//         if (iter != hashmap.end()) {
//             out[idx] = iter->second;
//         }
//     }
// }

#include "hashmap.h"

void hash_query_cpu(const uint32_t* hash_query, const uint32_t* hash_target,
                    const uint32_t* idx_target, uint32_t* out, const int n, const int n1) {
    HashMap hashmap;
    HashInit(&hashmap); 

    for (int i = 0; i < n; i++) {
        HashAdd(&hashmap, hash_target[i], idx_target[i] + 1);
    }

    for (int i = 0; i < n1; i++) {
        out[i] = HashGetValue(&hashmap, hash_query[i]);
    }

}

void hash_query_rvv(const uint32_t* hash_query, const uint32_t* hash_target,
                    const uint32_t* idx_target, uint32_t* out, const int n, const int n1) {
    HashMap HashMap_rvv;
    HashInit_rvv(&HashMap_rvv, n);
    size_t vlmax = vsetvlmax_e32m1();

    for (size_t i = 0; i < n; i+=vlmax) {
        size_t vl = vsetvl_e32m1(n - i);
        HashAdd_rvv(&HashMap_rvv, hash_target + i, idx_target + i, vl);
    }

    for (size_t i = 0; i < n1; i+=vlmax) {
        size_t vl = vsetvl_e32m1(n1 - i);
        HashGet_rvv(&HashMap_rvv, hash_query + i, vl, out + i);
    }
}

