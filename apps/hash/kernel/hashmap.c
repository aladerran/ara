#include "hashmap.h"
#include <stdio.h>

void HashInit(HashMap *array) {
    array->length = 0;
    for (int i = 0; i < MAX_SIZE; i++) {
        array->data[i].is_used = false;
    }
}

void HashAdd(HashMap *array, uint32_t key, uint32_t value) {
    int index = key % MAX_SIZE;
    int start_index = index;
    while (array->data[index].is_used && array->data[index].key != key) {
        index = (index + 1) % MAX_SIZE;  // Linear probing for collision resolution
        if (index == start_index) return; // Stop if we've checked the whole table
    }
    array->data[index].key = key;
    array->data[index].value = value;
    array->data[index].is_used = true;
    if (index == array->length) {
        array->length++;
    }
}

uint32_t HashGetValue(HashMap *array, uint32_t key) {
    int index = key % MAX_SIZE;
    int start_index = index;
    while (array->data[index].is_used) {
        if (array->data[index].key == key) {
            return array->data[index].value;
        }
        index = (index + 1) % MAX_SIZE;
        if (index == start_index) break; // Stop if we've checked the whole table
    }
    return 0;  // Return 0 if not found
}


/*=========================================================================================*/


void HashInit_rvv(HashMap_rvv* map, size_t length) {

    map->length = length;

}

// Add a key-value pair to the hashmap_rvv, assuming no-collision when adding
void HashAdd_rvv(HashMap_rvv* map, uint32_t* keys, uint32_t* values, size_t vl) {

    size_t vl_actual = vsetvl_e32m1(vl);

    vuint32m1_t vkeys = vle32_v_u32m1(keys, vl_actual);
    // Use the keys % MAX_CAPACITY as the index to store the values in the hashmap
    vuint32m1_t vindices = vremu_vx_u32m1(vkeys, MAX_CAPACITY, vl_actual);
    uint32_t indices[MAX_CAPACITY];
    vse32_v_u32m1(indices, vindices, vl_actual);

    for (size_t i = 0; i < vl; i++) {
        map->keys[indices[i]] = keys[i];
        map->values[indices[i]] = values[i];
    }

}

// Get the values of the keys in the hashmap_rvv
void HashGet_rvv(HashMap_rvv* map, const uint32_t* keys, size_t vl, uint32_t* out) {

    size_t vl_actual = vsetvl_e32m1(vl);

    vuint32m1_t vkeys = vle32_v_u32m1(keys, vl_actual);
    vuint32m1_t vindices = vremu_vx_u32m1(vkeys, MAX_CAPACITY, vl_actual);

    for (size_t i = 0; i < map->length; i+=vl_actual) {
        // Load the values from the hashmap_rvv
        vuint32m1_t vvalues = vle32_v_u32m1(map->values, vl_actual);

        // Gather the values from the hashmap_rvv based on vindices
        vuint32m1_t vvalues_gathered = vrgather_vv_u32m1(vvalues, vindices, vl_actual);

        // Store the gathered values back to the output array
        vse32_v_u32m1(out, vvalues, vl_actual);
    }
}
