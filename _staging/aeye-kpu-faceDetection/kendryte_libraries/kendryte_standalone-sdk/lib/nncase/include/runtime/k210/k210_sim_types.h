/* Copyright 2019 Canaan Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once
#include <array>
#include <cstdint>

#ifdef __riscv64
#define NNCASE_TARGET_K210_SIMULATOR 0
#include <kpu.h>
#else
#define NNCASE_TARGET_K210_SIMULATOR 1
#endif

namespace nncase
{
namespace runtime
{
    namespace k210
    {
#if NNCASE_TARGET_K210_SIMULATOR
        typedef struct
        {
            union {
                uint64_t reg;
                struct
                {
                    uint64_t int_en : 1;
                    uint64_t ram_flag : 1;
                    uint64_t full_add : 1;
                    uint64_t depth_wise_layer : 1;
                    uint64_t reserved : 60;
                } data;
            } interrupt_enabe;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t image_src_addr : 15;
                    uint64_t reserved0 : 17;
                    uint64_t image_dst_addr : 15;
                    uint64_t reserved1 : 17;
                } data;
            } image_addr;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t i_ch_num : 10;
                    uint64_t reserved0 : 22;
                    uint64_t o_ch_num : 10;
                    uint64_t reserved1 : 6;
                    uint64_t o_ch_num_coef : 10;
                    uint64_t reserved2 : 6;
                } data;
            } image_channel_num;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t i_row_wid : 10;
                    uint64_t i_col_high : 9;
                    uint64_t reserved0 : 13;
                    uint64_t o_row_wid : 10;
                    uint64_t o_col_high : 9;
                    uint64_t reserved1 : 13;
                } data;
            } image_size;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t kernel_type : 3;
                    uint64_t pad_type : 1;
                    uint64_t pool_type : 4;
                    uint64_t first_stride : 1;
                    uint64_t bypass_conv : 1;
                    uint64_t load_para : 1;
                    uint64_t reserved0 : 5;
                    uint64_t dma_burst_size : 8;
                    uint64_t pad_value : 8;
                    uint64_t bwsx_base_addr : 32;
                } data;
            } kernel_pool_type_cfg;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t load_coor : 1;
                    uint64_t load_time : 6;
                    uint64_t reserved0 : 8;
                    uint64_t para_size : 17;
                    uint64_t para_start_addr : 32;
                } data;
            } kernel_load_cfg;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t coef_column_offset : 4;
                    uint64_t coef_row_offset : 12;
                    uint64_t reserved0 : 48;
                } data;
            } kernel_offset;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t channel_switch_addr : 15;
                    uint64_t reserved : 1;
                    uint64_t row_switch_addr : 4;
                    uint64_t coef_size : 8;
                    uint64_t coef_group : 3;
                    uint64_t load_act : 1;
                    uint64_t active_addr : 32;
                } data;
            } kernel_calc_type_cfg;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t wb_channel_switch_addr : 15;
                    uint64_t reserved0 : 1;
                    uint64_t wb_row_switch_addr : 4;
                    uint64_t wb_group : 3;
                    uint64_t reserved1 : 41;
                } data;
            } write_back_cfg;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t shr_w : 4;
                    uint64_t shr_x : 4;
                    uint64_t arg_w : 24;
                    uint64_t arg_x : 24;
                    uint64_t reserved0 : 8;
                } data;
            } conv_value;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t arg_add : 40;
                    uint64_t reserved : 24;
                } data;
            } conv_value2;

            union {
                uint64_t reg;
                struct
                {
                    uint64_t send_data_out : 1;
                    uint64_t reserved : 15;
                    uint64_t channel_byte_num : 16;
                    uint64_t dma_total_byte : 32;
                } data;
            } dma_parameter;
        } kpu_layer_argument_t;

        typedef struct
        {
            union {
                uint64_t reg;
                struct
                {
                    uint64_t shift_number : 8;
                    uint64_t y_mul : 16;
                    uint64_t x_start : 36;
                } data;
            } activate_para[16];

            union {
                uint64_t reg;
                struct
                {
                    uint8_t result_bias[8];
                } data;
            } activate_para_bias0;

            union {
                uint64_t reg;
                struct
                {
                    uint8_t result_bias[8];
                } data;
            } activate_para_bias1;
        } kpu_activate_table_t;
#endif

        typedef struct
        {
            union {
                uint64_t reg;
                struct
                {
                    uint64_t norm_mul : 24;
                    uint64_t norm_add : 32;
                    uint64_t norm_shift : 4;
                } data;
            } batchnorm;
        } kpu_batchnorm_argument_t;

        typedef enum _kpu_filter_type
        {
            kpu_filter_1x1 = 0,
            kpu_filter_3x3 = 1
        } kpu_filter_type_t;

        typedef enum _kpu_pool_type
        {
            kpu_pool_bypass = 0,
            kpu_pool_max_2_s2 = 1,
            kpu_pool_mean_2_s2 = 2,
            kpu_pool_max_4_s4 = 3,
            kpu_pool_mean_4_s4 = 4,
            kpu_pool_left_top_2_s2 = 5,
            kpu_pool_right_top_2_s2 = 6,
            kpu_pool_left_top_4_s4 = 7,
            kpu_pool_mean_2_s1 = 8,
            kpu_pool_max_2_s1 = 9
        } kpu_pool_type_t;

        struct kpu_batchnorm_segment
        {
            int32_t mul;
            int32_t shift;
            int32_t add;
        };

        struct kpu_activation_segment
        {
            int64_t start_x;
            int32_t mul;
            int32_t shift;
            int32_t add;
        };

        using kpu_activation_table_t = std::array<kpu_activation_segment, 16>;

        struct piecewise_linear_segment
        {
            float start;
            float mul;
            float add;
        };
    }
}
}
