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
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <datatypes.h>

namespace nncase
{
namespace kernels
{
    inline size_t offset(const runtime_shape_t &shape, const runtime_shape_t &index)
    {
        return (((size_t)index[0] * shape[1] + index[1]) * shape[2] + index[2]) * shape[3] + index[3];
    }

    namespace details
    {
        inline int32_t get_windowed_output_size(int32_t size, int32_t filter, int32_t stride, int32_t dilation, const padding &padding)
        {
            auto effective_filter_size = (filter - 1) * dilation + 1;
            return (size + padding.before + padding.after - effective_filter_size + stride) / stride;
        }

        inline size_t compute_size(const runtime_shape_t &shape)
        {
            return size_t(shape[0]) * shape[1] * shape[2] * shape[3];
        }

        template <class T>
        inline T apply_activation(T value, value_range<T> activation)
        {
            return std::clamp(value, activation.min, activation.max);
        }

        inline runtime_shape_t get_reduced_offset(const runtime_shape_t &in_offset, const runtime_shape_t &reduced_shape)
        {
            runtime_shape_t off;
            for (size_t i = 0; i < in_offset.size(); i++)
            {
                if (in_offset[i] >= reduced_shape[i])
                    off[i] = 0;
                else
                    off[i] = in_offset[i];
            }

            return off;
        }

        template <class T, class TRange>
        struct default_ptr_getter
        {
            T *operator()(const TRange &range) const noexcept { return range; }
        };

        template <int32_t Bits>
        int32_t to_signed(uint32_t value)
        {
            auto mask = uint32_t(1) << (Bits - 1);
            if (Bits != 32 && (value & mask) != 0)
            {
                auto sign = 0xFFFFFFFF << Bits;
                return (int)(value | sign);
            }

            return (int32_t)value;
        }

        template <int32_t Bits>
        int64_t to_signed(uint64_t value)
        {
            auto mask = uint64_t(1) << (Bits - 1);
            if ((value & mask) != 0)
            {
                auto sign = 0xFFFFFFFFFFFFFFFF << Bits;
                return (int64_t)(value | sign);
            }

            return (int64_t)value;
        }
    }
}
}
