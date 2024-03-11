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

int main() {
    printf("\n==========\n=  HASH  =\n==========\n");
    printf("------------------------------------------------------------\n------------------------------------------------------------\n");

    int data[DATA_SIZE];
    uint32_t result_cpu[NHASH];
    uint32_t result_ara[NHASH];

    seed_simple(1234);
    generate_data(data, DATA_SIZE);

    printf("Hashing %d points...\n", NHASH);

    printf("------------------------------------------------------------\nRunning CPU Hash\n------------------------------------------------------------\n");

    int64_t start_cpu = get_cycle_count();
    hash_cpu(data, result_cpu, NHASH);
    int64_t end_cpu = get_cycle_count();

    printf("CPU cycles: %ld\n\n", end_cpu - start_cpu);

    printf("------------------------------------------------------------\nRunning Ara Hash\n------------------------------------------------------------\n");
    
    int64_t start_ara = get_cycle_count();
    hash_rvv(data, result_ara, NHASH);
    int64_t end_ara =  get_cycle_count();

    printf("RVV cycles: %ld\n\n", end_ara - start_ara);

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

    return error;
}
