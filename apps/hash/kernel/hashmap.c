#include "hashmap.h"
#include <stdio.h>

// void HashInit(HashMap *array) {
//     array->length = 0;
//     for (int i = 0; i < MAX_SIZE; i++) {
//         array->data[i].is_used = false;
//     }
// }

// void HashAdd(HashMap *array, uint32_t key, uint32_t value) {
//     int index = key % MAX_SIZE;
//     int start_index = index;
//     while (array->data[index].is_used && array->data[index].key != key) {
//         index = (index + 1) % MAX_SIZE;  // Linear probing for collision resolution
//         if (index == start_index) return; // Stop if we've checked the whole table
//     }
//     array->data[index].key = key;
//     array->data[index].value = value;
//     array->data[index].is_used = true;
//     if (index == array->length) {
//         array->length++;
//     }
// }

// uint32_t HashGetValue(HashMap *array, uint32_t key) {
//     int index = key % MAX_SIZE;
//     int start_index = index;
//     while (array->data[index].is_used) {
//         if (array->data[index].key == key) {
//             return array->data[index].value;
//         }
//         index = (index + 1) % MAX_SIZE;
//         if (index == start_index) break; // Stop if we've checked the whole table
//     }
//     return 0;  // Return 0 if not found
// }


/*=========================================================================================*/

void hash_table_init(HashTable* ht) {
    for (int i = 0; i < HASH_TABLE_SIZE; i++) {
        ht->keys[i] = EMPTY_KEY;
        ht->payloads[i] = 0;
    }
}


// #define DEBUG
// #define NO_CONFLICT

void insert_batch(HashTable* ht, const uint32_t* keys, const uint32_t* payloads, size_t num_keys) {

    size_t vl = W;

    #ifdef DEBUG
    uint32_t debug[W];
    #endif

    uint32_t unique_ids[W];
    for (size_t lane = 0; lane < W; ++lane) {
        unique_ids[lane] = lane; // Unique identifier for conflict detection
    }

    vuint32m1_t unique_ids_v = vle32_v_u32m1(unique_ids, vl); // Load unique lane identifiers


    size_t i = 0;
    
    vbool32_t mask = vmset_m_b32(vl);

    vuint32m1_t offset = vmv_v_x_u32m1(0, vl);

    // Other variables needed
    vuint32m1_t vempty = vmv_v_x_u32m1(EMPTY_KEY, vl);
    vuint32m1_t maskedoff = vmv_v_x_u32m1(0, vl);
    vuint32m1_t vhtsize = vmv_v_x_u32m1(HASH_TABLE_SIZE, vl);
    vuint32m1_t element_size = vmv_v_x_u32m1(4, vl);


    bool stop = false;

    while (!stop){

        if (i + W >= num_keys) {
            stop = true;
        }

        vl = vsetvl_e32m1(num_keys - i);


        #ifdef DEBUG
        printf("i: %d\n", i);
        #endif

        vuint32m1_t k = vle32_v_u32m1(&keys[i], vl);
        vuint32m1_t v = vle32_v_u32m1(&payloads[i], vl);

        size_t num_true = vcpop_m_b32(mask, vl);
        i += num_true;

        // Mutiplicative hashing & offset
        vuint32m1_t hash = vmul_vx_u32m1(k, HASH_FACTOR, vl);
        vuint32m1_t h = vremu_vx_u32m1(hash, HASH_TABLE_SIZE, vl);

        #ifdef DEBUG
        vse32_v_u32m1(debug, h, vl);
        for (int j = 0; j < W; j++) {
            printf("debug: h[%d] = %d\n", j, debug[j]);
        }
        #endif

        printf("debug:fix overflow\n");

        // Fix overflows for h
        h = vadd_vv_u32m1(h, offset, vl); // Add offset to h
        vbool32_t overflow_mask = vmsgtu_vx_u32m1_b32(h, HASH_TABLE_SIZE, vl);
        h = vsub_vv_u32m1_m(overflow_mask, h, h, vhtsize, vl);

        #ifdef DEBUG
        vse32_v_u32m1(debug, h, vl);
        for (int j = 0; j < W; j++) {
            printf("debug: h[%d] = %d\n", j, debug[j]);
        }
        #endif

        vuint32m1_t vkeys = vle32_v_u32m1(ht->keys, vl);
        vuint32m1_t table_keys = vrgather_vv_u32m1_m(mask, maskedoff, vkeys, h, vl); // not sure what to do w. maskedoff

        printf("debug:update mask\n");

        // update mask = (table_keys == EMPTY_KEY)
        mask = vmseq_vv_u32m1_b32(table_keys, vempty, vl);

        #ifndef NO_CONFLICT
        printf("debug:conflict detection\n");

        #ifdef DEBUG
        printf("Before conflict detection\n");
        for (int j = 0; j < W; j++) {
            printf("ht->keys[%d] = %d\n", j, ht->keys[j]);
        }
        #endif

        // Detect conflicts and resolve them
        // Scatter unique identifiers to the hash table for conflict detection
        vuint32m1_t h_with_offset = vmul_vv_u32m1(h, element_size, vl); // aladerran: Damn... we need to mutiply 4 (bytes) w. the index
        vsuxei32_v_u32m1_m(mask, ht->keys, h_with_offset, unique_ids_v, vl); // Use mask to scatter only where it's safe
        
        #ifdef DEBUG
        printf("In conflict detection\n");
        for (int j = 0; j < W; j++) {
            printf("ht->keys[%d] = %d\n", j, ht->keys[j]);
        }
        #endif

        // Gather keys again to check for conflicts
        vuint32m1_t post_scatter_keys = vrgather_vv_u32m1(vkeys, h_with_offset, vl); 

        // Generate conflict mask where gathered keys do not match the unique identifiers
        vbool32_t conflict_mask = vmseq_vv_u32m1_b32(post_scatter_keys, unique_ids_v, vl); 

        // Invert the conflict mask to find lanes without conflicts. Since rvv intrinsics do not have
        // vmnot_m_b32, we must use vmxor with a mask of all true to invert the bits.
        vbool32_t m_all = vmset_m_b32(vl); // Mask of all true bits
        vbool32_t safe_to_write_mask = vmxor_mm_b32(conflict_mask, m_all, vl);

        printf("debug:scatter keys and payloads\n");

        // Scatter keys and payloads to the hash table where there are no conflicts
        vsuxei32_v_u32m1_m(safe_to_write_mask, ht->keys, h_with_offset, k, vl);
        vsuxei32_v_u32m1_m(safe_to_write_mask, ht->payloads, h_with_offset, v, vl);

        #endif

        #ifdef NO_CONFLICT
        // Scatter keys and payloads to the hash table where there are no conflicts
        vuint32m1_t h_with_offset = vmul_vv_u32m1(h, element_size, vl);
        vsuxei32_v_u32m1_m(mask, ht->keys, h_with_offset, k, vl);
        vsuxei32_v_u32m1_m(mask, ht->payloads, h_with_offset, v, vl);
        #endif

        printf("debug:increment/reset offsets\n");

        // Increment or reset offsets w.r.t. mask
        offset = vadd_vx_u32m1_m(mask, offset, offset, 1, vl);

    }

}

