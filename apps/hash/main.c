#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "kernel/hash.h"
#include "runtime.h"
#include "util.h"

#ifndef SPIKE
#include "printf.h"
#endif

#define NHASH 1000
#define DATA_SIZE (NHASH * 4)

#define NQUERY 500     // Number of queries
#define NDATA 1000      // Number of data elements in the hash map


static uint32_t simple_seed = 0;

void seed_simple(uint32_t seed) {
    simple_seed = seed;
}

uint32_t simple_rand() {
    simple_seed = simple_seed * 1103515245 + 12345;
    return (simple_seed / 65536) % 32768;
}

void generate_data(int *data, int N) {
    for (int i = 0; i < N; i++) {
        data[i] = simple_rand() % 100;
    }
}

void copy_data(int *src, int *dst, int N) {
    for (int i = 0; i < N; i++) {
        dst[i] = src[i];
    }
}

void initialize_data(int *data, int N) {
    for (int i = 0; i < N; i++) {
        data[i] = 0;
    }
}


int main() {

    int data[DATA_SIZE];

    seed_simple(1234);
    generate_data(data, DATA_SIZE);

    printf("\n==========\n=  HASH  =\n==========\n");
    printf("------------------------------------------------------------\n------------------------------------------------------------\n");

    printf("Hashing %d points...\n", NHASH);

    printf("------------------------------------------------------------\nRunning CPU Hash\n------------------------------------------------------------\n");

    uint32_t result_cpu[NHASH];
    uint32_t result_ara[NHASH];
    
    int64_t start_cpu = get_cycle_count();
    hash_cpu(data, result_cpu, NHASH);
    int64_t end_cpu = get_cycle_count();

    printf(" CPU cycles: %ld\n\n", end_cpu - start_cpu);

    printf("------------------------------------------------------------\nRunning Ara Hash\n------------------------------------------------------------\n");
    
    int64_t start_ara = get_cycle_count();
    hash_rvv(data, result_ara, NHASH);
    int64_t end_ara =  get_cycle_count();

    printf(" RVV cycles: %ld\n\n", end_ara - start_ara);

    int error = 0;
    for (int i = 0; i < NHASH; i++) {
        if (result_cpu[i] != result_ara[i]) {
            error = 1;
            printf("ERROR: hash mismatch starts at index %d: %u != %u\n", i, result_cpu[i], result_ara[i]);
            break;
        }
    }

    if (error) {
        printf("FAIL: hash mismatch\n");
    } else {
        printf("SUCCESS: hash match\n");
    }


    printf("\n===================\n=  KERNEL HASH  =\n===================\n");
    printf("------------------------------------------------------------\n------------------------------------------------------------\n");


    int kenel_volume = 3;
    int kernel_offset[kenel_volume];
    uint32_t result_cpu_kernel[NHASH];
    uint32_t result_ara_kernel[NHASH];

    seed_simple(1234);
    generate_data(kernel_offset, kenel_volume);

    int data_rvv[DATA_SIZE];
    int kernel_offset_rvv[kenel_volume];

    printf("------------------------------------------------------------\nRunning CPU Kernel Hash\n------------------------------------------------------------\n");

    int64_t start_cpu_kernel = get_cycle_count();
    kernel_hash_cpu(data, kernel_offset, result_cpu_kernel, NHASH, kenel_volume);
    int64_t end_cpu_kernel = get_cycle_count();

    // store the result of the CPU kernel hash into a place that cannot be modified
    const uint32_t result_cpu_kernel_const[NHASH];
    memcpy((void*)result_cpu_kernel_const, result_cpu_kernel, sizeof(result_cpu_kernel));    

    printf(" CPU cycles: %ld\n\n", end_cpu_kernel - start_cpu_kernel);

    printf("------------------------------------------------------------\nRunning Ara Kernel Hash\n------------------------------------------------------------\n");

    int64_t start_ara_kernel = get_cycle_count();
    kernel_hash_rvv(data, kernel_offset, result_ara_kernel, NHASH, kenel_volume);
    int64_t end_ara_kernel = get_cycle_count();

    printf(" RVV cycles: %ld\n\n", end_ara_kernel - start_ara_kernel);

    int error_kernel = 0;
    for (int i = 0; i < NHASH; i++) {
        if (result_cpu_kernel_const[i] != result_ara_kernel[i]) {
            error_kernel = 1;
            printf("ERROR: hash mismatch starts at index %d: %u != %u\n", i, result_cpu_kernel_const[i], result_ara_kernel[i]);
            break;
        }
    }

    if (error_kernel) {
        printf("FAIL: kernel hash mismatch\n");
    } else {
        printf("SUCCESS: kernel hash match\n");
    }

    printf("\n===================\n=   HASH QUERY   =\n===================\n");
    printf("------------------------------------------------------------\n------------------------------------------------------------\n");

    static uint32_t hash_target[NDATA], idx_target[NDATA], hash_query[NQUERY], out[NQUERY], out_rvv[NQUERY];

    generate_data((int*)hash_target, NDATA);
    generate_data((int*)idx_target, NDATA); 
    generate_data((int*)hash_query, NQUERY);

    printf("Testing hash_query_cpu with %d queries and %d data points\n", NQUERY, NDATA);
    printf("------------------------------------------------------------\nRunning hash_query_cpu\n------------------------------------------------------------\n");

    int64_t start_hash_query = get_cycle_count();
    hash_query_cpu(hash_query, hash_target, idx_target, out, NDATA, NQUERY);
    int64_t end_hash_query = get_cycle_count();

    printf("hash_query_cpu CPU cycles: %ld\n\n", end_hash_query - start_hash_query);

    printf("------------------------------------------------------------\nRunning hash_query_rvv\n------------------------------------------------------------\n");

    int64_t start_hash_query_rvv = get_cycle_count();
    hash_query_rvv(hash_query, hash_target, idx_target, out_rvv, NDATA, NQUERY);
    int64_t end_hash_query_rvv = get_cycle_count();

    printf("hash_query_rvv RVV cycles: %ld\n\n", end_hash_query_rvv - start_hash_query_rvv);

    // Optionally print some of the output for verification
    int error_query = 0;
    printf("Sample Outputs:\n");
    for (int i = 0; i < 100 && i < NQUERY; i++) { 
        printf("Query %u -> Output %u\n", hash_query[i], out[i]);
        printf("Query %u -> Output %u (rvv)\n", hash_query[i], out_rvv[i]);
    }

    if (error_query) {
        printf("FAIL: hash query mismatch\n");
    } else {
        printf("SUCCESS: hash query match\n");
    }

    printf("------------------------------------------------------------\n");

    return 0;
}
