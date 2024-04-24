#include <stdlib.h>
#include <stdio.h>

#include "gather_scatter.h"


void gather_cpu(const int n_k, const int n_in, const int c,
                const float *in_feat, float *out_feat, const int *kmap,
                const bool transpose) {

    for (int i = 0; i < n_k; i++) {
        int in_pos = kmap[2 * i + transpose];
        if (in_pos < 0) {
            continue;
        }
        for (int j = 0; j < c; j++) {
            out_feat[i * c + j] = in_feat[in_pos * c + j];
        }
    }

}

// Gather using RVV within the inner loop, benefits when channel number is large
void gather_rvv(const int n_k, const int n_in, const int c,
                const float *in_feat, float *out_feat, const int *kmap,
                const bool transpose) {

    const int index_offset = transpose ? 1 : 0;
    size_t vlmax = vsetvlmax_e32m1();

    // size_t inner_stride = vlmax / c;
    
    // for (int i = 0; i < n_k; i += vlmax) {
    //     size_t vl = vsetvl_e32m1(n_k - i);

    //     // Load kmap indices with stride
    //     vint32m1_t vindex = vlse32_v_i32m1(&kmap[2 * i + index_offset], sizeof(int) * 2, vl);
        
    //     // Generate mask for valid indices
    //     vbool32_t mask = vmnot_m_b32(vmslt_vx_i32m1_b32(vindex, 0, vl), vl);

    //     // Calculate the number of valid indices
    //     unsigned long vl_valid = vpopc_m_b32(mask, vl); // Clang seems not to support Vector mask population count functions...

    //     // Cast vl_valid to size_t
    //     vl_valid = (size_t)vl_valid;

    //     // Compress the indices to make the indexed data contiguous
    //     vint32m1_t vindex_compressed = vcompress_vm_i32m1(mask, vindex_compressed, vindex, vl);

    //     // Store the vindex_compressed to a buffer
    //     int index_buffer[vl_valid];
    //     vse32_v_i32m1(index_buffer, vindex_compressed, vl_valid);

    //     for (int k = 0; k < vl_valid; k += inner_stride){
    //         int in_pos = index_buffer[k];
    //         for (int j = 0; j < c; j += vlmax) {
    //             size_t inner_vl = vsetvl_e32m1(c - j);
                
    //             // Prepare vectorized feat buffer
    //             for (int l = 0; l < inner_stride; l++) {
    //                 vfloat32m1_t vfeat = vle32_v_f32m1(&in_feat[in_pos * c + j + l], inner_vl);

    //                 // Store the data from vfeat to out_feat directly
    //                 vse32_v_f32m1(&out_feat[(i + k + l) * c + j], vfeat, inner_vl);
    //             }
    //         }
    //     }
    // }

    for (int i = 0; i < n_k; i++) {
        int in_pos = kmap[2 * i + transpose];
        if (in_pos < 0) {
            continue;
        }

        for (int j = 0; j < c; j += vlmax) {
            size_t inner_vl = vsetvl_e32m1(c - j);

            // Prepare vectorized feat buffer
            vfloat32m1_t vfeat = vle32_v_f32m1(&in_feat[in_pos * c + j], inner_vl);

            // Store the data from vfeat to out_feat directly
            vse32_v_f32m1(&out_feat[i * c + j], vfeat, inner_vl);
        }
    }
}


/*=========================================================================================*/


void scatter_cpu(const int n_k, const int n_in, const int c,
                 const float *in_feat, float *out_feat, const int *kmap,
                 const bool transpose) {

    for (int i = 0; i < n_k; i++) {
        int in_pos = kmap[2 * i + transpose];
        if (in_pos < 0) {
            continue;
        }
        for (int j = 0; j < c; j++) {
            out_feat[in_pos * c + j] += in_feat[i * c + j];
        }
    }

}

// Scatter using RVV has the same structure as gather, only the last step is different
// since we need to accumulate the final values instead of just storing them
void scatter_rvv(const int n_k, const int n_in, const int c,
                 const float *in_feat, float *out_feat, const int *kmap,
                 const bool transpose) {

    const int index_offset = transpose ? 1 : 0;
    size_t vlmax = vsetvlmax_e32m1();

    // size_t inner_stride = vlmax / c;
    
    // for (int i = 0; i < n_k; i += vlmax) {
    //     size_t vl = vsetvl_e32m1(n_k - i);

    //     vint32m1_t vindex = vlse32_v_i32m1(&kmap[2 * i + index_offset], sizeof(int) * 2, vl);
        
    //     vbool32_t mask = vmnot_m_b32(vmslt_vx_i32m1_b32(vindex, 0, vl), vl);

    //     unsigned long vl_valid = vpopc_m_b32(mask, vl);

    //     vl_valid = (size_t)vl_valid;

    //     vint32m1_t vindex_compressed = vcompress_vm_i32m1(mask, vindex_compressed, vindex, vl);

    //     int index_buffer[vl_valid];
    //     vse32_v_i32m1(index_buffer, vindex_compressed, vl_valid);

    //     for (int k = 0; k < vl_valid; k += inner_stride){
    //         int in_pos = index_buffer[k];
    //         for (int j = 0; j < c; j += vlmax) {
    //             size_t inner_vl = vsetvl_e32m1(c - j);
                
    //             for (int l = 0; l < inner_stride; l++) {
    //                 vfloat32m1_t vfeat = vle32_v_f32m1(&in_feat[in_pos * c + j + l], inner_vl);

    //                 // accumulate the final values instead of directly storing them
    //                 vfloat32m1_t vout_feat = vle32_v_f32m1(&out_feat[in_pos * c + j], inner_vl);
    //                 vfloat32m1_t vout_feat_acc = vfadd_vv_f32m1(vout_feat, vfeat, inner_vl);
    //                 vse32_v_f32m1(&out_feat[in_pos * c + j], vout_feat_acc, inner_vl);
    //             }
    //         }
    //     }
    // }
 
    for (int i = 0; i < n_k; i++) {
        int in_pos = kmap[2 * i + transpose];
        if (in_pos < 0) {
            continue;
        }

        for (int j = 0; j < c; j += vlmax) {
            size_t inner_vl = vsetvl_e32m1(c - j);

            // Prepare vectorized feat buffer
            vfloat32m1_t vfeat = vle32_v_f32m1(&in_feat[i * c + j], inner_vl);

            // Load the data from out_feat
            vfloat32m1_t vout_feat = vle32_v_f32m1(&out_feat[in_pos * c + j], inner_vl);

            // Accumulate the data
            vfloat32m1_t vout_feat_acc = vfadd_vv_f32m1(vout_feat, vfeat, inner_vl);

            // Store the data back to out_feat
            vse32_v_f32m1(&out_feat[in_pos * c + j], vout_feat_acc, inner_vl);
        }
    }
}


/*=========================================================================================*/


// vectorized std::fill
void vectorized_fill(int* arr, size_t n, int value) {
    size_t vlmax = vsetvlmax_e32m1();  
    vint32m1_t vvalue = vmv_v_x_i32m1(value, vlmax); 

    size_t i = 0;
    for (; i <= n - vlmax; i += vlmax) {
        vse32_v_i32m1(arr + i, vvalue, vlmax);  
    }

    if (i < n) {
        size_t vl = vsetvl_e32m1(n - i);
        vse32_v_i32m1(arr + i, vvalue, vl);
    }
}
