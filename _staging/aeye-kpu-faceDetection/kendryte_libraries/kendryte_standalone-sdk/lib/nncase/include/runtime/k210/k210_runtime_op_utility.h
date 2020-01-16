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
#include "k210_sim_types.h"

namespace nncase
{
namespace runtime
{
    namespace k210
    {
        struct kpu_layout
        {
            int32_t groups;
            int32_t row_len;
            int32_t row_pitch;
        };

        inline kpu_layout get_kpu_row_layout(int32_t width)
        {
            kpu_layout layout;

            if (width <= 16)
            {
                layout.groups = 4;
                layout.row_len = 1;
                layout.row_pitch = 16;
            }
            else if (width <= 32)
            {
                layout.groups = 2;
                layout.row_len = 1;
                layout.row_pitch = 32;
            }
            else
            {
                layout.groups = 1;
                layout.row_len = (width + 63) / 64;
                layout.row_pitch = 64;
            }

            return layout;
        }

        inline int32_t get_kpu_filter_size(kpu_filter_type_t filter)
        {
            switch (filter)
            {
            case kpu_filter_1x1:
                return 1;
            case kpu_filter_3x3:
                return 3;
            default:
                NNCASE_THROW(std::runtime_error, "Invalid kpu filter");
            }
        }

        inline int32_t get_kpu_padding(kpu_filter_type_t filter)
        {
            switch (filter)
            {
            case kpu_filter_1x1:
                return 0;
            case kpu_filter_3x3:
                return 1;
            default:
                NNCASE_THROW(std::runtime_error, "Invalid kpu filter");
            }
        }

        inline int32_t get_kpu_rows(int32_t width, int32_t height, int32_t channels)
        {
            auto layout = get_kpu_row_layout(width);
            auto one_line_channels = std::min(channels, layout.groups);
            auto blocks = (channels + one_line_channels - 1) / one_line_channels;
            auto size = layout.row_len * height * blocks;
            return size;
        }

        inline int32_t get_kpu_bytes(int32_t width, int32_t height, int32_t channels)
        {
            return get_kpu_rows(width, height, channels) * 64;
        }

        inline int32_t get_kpu_bytes(const runtime_shape_t &shape)
        {
            return get_kpu_bytes(shape[3], shape[2], shape[1]) * shape[0];
        }

        inline int32_t get_kpu_filter_size(kpu_pool_type_t filter)
        {
            switch (filter)
            {
            case kpu_pool_bypass:
                return 1;
            case kpu_pool_max_2_s2:
            case kpu_pool_mean_2_s2:
            case kpu_pool_left_top_2_s2:
            case kpu_pool_right_top_2_s2:
            case kpu_pool_max_2_s1:
            case kpu_pool_mean_2_s1:
                return 2;
            case kpu_pool_max_4_s4:
            case kpu_pool_mean_4_s4:
            case kpu_pool_left_top_4_s4:
                return 4;
            default:
                NNCASE_THROW(std::runtime_error, "Invalid kpu filter");
            }
        }

        inline int32_t get_kpu_filter_stride(kpu_pool_type_t filter)
        {
            switch (filter)
            {
            case kpu_pool_bypass:
                return 1;
            case kpu_pool_max_2_s2:
            case kpu_pool_mean_2_s2:
            case kpu_pool_left_top_2_s2:
            case kpu_pool_right_top_2_s2:
                return 2;
            case kpu_pool_max_2_s1:
            case kpu_pool_mean_2_s1:
                return 1;
            case kpu_pool_max_4_s4:
            case kpu_pool_mean_4_s4:
            case kpu_pool_left_top_4_s4:
                return 4;
            default:
                NNCASE_THROW(std::runtime_error, "Invalid kpu pool type");
            }
        }

        inline int32_t get_kpu_pool_output_size(int32_t input, kpu_pool_type_t pool_type)
        {
            return input / get_kpu_filter_stride(pool_type);
        }

        inline std::array<int32_t, 2> get_kpu_select_pool_offset(kpu_pool_type_t pool_type)
        {
            switch (pool_type)
            {
            case kpu_pool_left_top_2_s2:
                return { 0, 0 };
            case kpu_pool_right_top_2_s2:
                return { 0, 1 };
            case kpu_pool_left_top_4_s4:
                return { 0, 0 };
            default:
                NNCASE_THROW(std::runtime_error, "Invalid kpu pool type");
            }
        }
    }
}
}
