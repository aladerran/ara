#ifndef HASH_MAP_H
#define HASH_MAP_H

#include <stdlib.h> 
#include <stdint.h> 
#include <stdbool.h> 

#include <riscv_vector.h>

// #define MAX_SIZE 1024
// #define MAX_CAPACITY 1024

// typedef struct {
//     uint32_t key;
//     uint32_t value;
//     bool is_used;
// } PairValue; // a PaireValue takes up 12 bytes

// typedef struct {
//     PairValue data[MAX_SIZE];
//     int length;
// } HashMap; // a HashMap takes up MAX_SIZE * 12 bytes

// void HashInit(HashMap *array);
// void HashAdd(HashMap *array, uint32_t key, uint32_t value);
// uint32_t HashGetValue(HashMap *array, uint32_t key);

/*=========================================================================================*/

#define HASH_TABLE_SIZE 1024
#define EMPTY_KEY 0xFFFFFFFF // Representation of an empty key slot
#define HASH_FACTOR 0x9e3779b9 // Random prime number
#define W 128 // Vector width

typedef struct {
    uint32_t keys[HASH_TABLE_SIZE];
    uint32_t payloads[HASH_TABLE_SIZE];
} HashTable;

void hash_table_init(HashTable* ht);
void insert_batch(HashTable* ht, const uint32_t* keys, const uint32_t* payloads, size_t num_keys);
// void find_batch(HashTable* ht, const uint32_t* keys, uint32_t* out, size_t num_keys);

#endif // HASH_MAP_H
