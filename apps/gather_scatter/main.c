#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "kernel/gather_scatter.h"
#include "runtime.h"
#include "util.h"

#ifndef SPIKE
#include "printf.h"
#endif

#define NGATHER 30
#define DATA_SIZE 128
#define CHANNELS 128


int main() {

    // printf("\n===================\n= Vectorized Fill =\n===================\n");

    // int arr[DATA_SIZE];
    // size_t n = DATA_SIZE;
    // int value = 0;

    // printf("------------------------------------------------------------\nRunning RVV Vectorized Fill\n------------------------------------------------------------\n");
    
    // printf("Filling %d points...\n", n);

    // int64_t start_fill = get_cycle_count();
    // vectorized_fill(arr, n, value);
    // int64_t end_fill = get_cycle_count();

    // printf("CPU cycles: %ld\n\n", end_fill - start_fill);


    printf("\n==================\n= Gather/Scatter =\n==================\n");
    printf("------------------------------------------------------------\n------------------------------------------------------------\n");

    int n_k = NGATHER, n_in = DATA_SIZE, c = CHANNELS;
    
    float in_feat[DATA_SIZE * CHANNELS];
    float out_feat_cpu[NGATHER * CHANNELS];
    float out_feat_ara[NGATHER * CHANNELS];
    int kmap[NGATHER * 2]; 

    for (int i = 0; i < DATA_SIZE * CHANNELS; i++) {
        in_feat[i] = (float)i / 10.0f; 
    }
    for (int i = 0; i < NGATHER; i++) {
        kmap[2 * i] = i % DATA_SIZE; 
        kmap[2 * i + 1] = (i * 2) % DATA_SIZE;
    }

    memset(out_feat_cpu, 0, sizeof(out_feat_cpu));
    memset(out_feat_ara, 0, sizeof(out_feat_ara));

    printf("Gather/Scatter %d points from %d points with %d channels\n", n_k, n_in, c);

    printf("------------------------------------------------------------\nRunning CPU Gather/Scatter\n------------------------------------------------------------\n");

    int64_t start_cpu = get_cycle_count();
    gather_cpu(n_k, n_in, c, in_feat, out_feat_cpu, kmap, false);
    // scatter_cpu(n_k, n_in, c, in_feat, out_feat_cpu, kmap, false);
    int64_t end_cpu = get_cycle_count();
    printf("CPU cycles: %ld\n\n", end_cpu - start_cpu);

    printf("------------------------------------------------------------\nRunning Ara Gather/Scatter\n------------------------------------------------------------\n");
    int64_t start_ara = get_cycle_count();
    gather_rvv(n_k, n_in, c, in_feat, out_feat_ara, kmap, false); 
    // scatter_rvv(n_k, n_in, c, in_feat, out_feat_ara, kmap, false);
    int64_t end_ara = get_cycle_count();
    printf("RVV cycles: %ld\n\n", end_ara - start_ara);

    int error = 0;
    for (int i = 0; i < n_k * c; i++) {
        if (out_feat_cpu[i] != out_feat_ara[i]) {
            error = 1;
            printf("ERROR: gather/scatter mismatch starts at index %d: %f != %f\n", i, out_feat_cpu[i], out_feat_ara[i]);
            break;
        }
    }

    if (error) {
        printf("FAIL: gather/scatter mismatch\n");
    } else {
        printf("SUCCESS: gather/scatter match\n");
    }

    return 0;
}
