#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdlib.h> 
#include <stdint.h> 
#include <stdbool.h> 

#include <riscv_vector.h>

#define MAX_SIZE 1024
#define MAX_CAPACITY 1024

typedef struct {
    uint32_t key;
    uint32_t value;
    bool is_used;
} PairValue; // a PaireValue takes up 12 bytes

typedef struct {
    PairValue data[MAX_SIZE];
    int length;
} HashMap; // a HashMap takes up MAX_SIZE * 12 bytes

void HashInit(HashMap *array);
void HashAdd(HashMap *array, uint32_t key, uint32_t value);
uint32_t HashGetValue(HashMap *array, uint32_t key);

/*=========================================================================================*/

typedef struct {
    uint32_t keys[MAX_CAPACITY];
    uint32_t values[MAX_CAPACITY];
    size_t length;
} HashMap_rvv; // a HashMap_rvv takes up 8 * MAX_CAPACITY+1 bytes

void HashInit_rvv(HashMap_rvv* map, size_t length);
void HashAdd_rvv(HashMap_rvv* map, uint32_t* keys, uint32_t* values, size_t vl);
void HashGet_rvv(HashMap_rvv* map, const uint32_t* keys, size_t vl, uint32_t* out);

#endif // HASH_MAP_H
