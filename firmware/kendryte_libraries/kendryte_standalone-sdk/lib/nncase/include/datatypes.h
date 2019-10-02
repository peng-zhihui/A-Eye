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
#include "target_config.h"
#include <array>
#include <cmath>
#include <limits>
#include <optional>
#include <stdint.h>

namespace nncase
{
typedef enum _datatype
{
    dt_float32,
    dt_uint8
} datatype_t;

struct padding
{
    int32_t before;
    int32_t after;

    int32_t sum() const noexcept { return before + after; }

    static padding zero() noexcept { return {}; }
};

template <class T>
struct value_range
{
    T min;
    T max;

    static constexpr value_range<T> full() noexcept
    {
        return { std::numeric_limits<T>::lowest(), std::numeric_limits<T>::max() };
    }
};

typedef enum _reduce_op
{
    reduce_mean,
    reduce_min,
    reduce_max,
    reduce_sum
} reduce_op_t;

typedef enum _binary_op
{
    binary_add,
    binary_sub,
    binary_mul,
    binary_div,
    binary_min,
    binary_max
} binary_op_t;

typedef enum _unary_op
{
    unary_abs,
    unary_ceil,
    unary_cos,
    unary_exp,
    unary_floor,
    unary_log,
    unary_neg,
    unary_rsqrt,
    unary_sin,
    unary_square
} unary_op_t;

typedef enum _image_resize_mode
{
    image_resize_bilinear,
    image_resize_nearest_neighbor
} image_resize_mode_t;

typedef struct _quant_param
{
    int32_t zero_point;
    float scale;
} quant_param_t;

inline bool operator==(const quant_param_t &lhs, const quant_param_t &rhs) noexcept
{
    return lhs.zero_point == rhs.zero_point && lhs.scale == rhs.scale;
}

inline bool almost_equal(const quant_param_t &lhs, const quant_param_t &rhs) noexcept
{
    return lhs.zero_point == rhs.zero_point && std::abs(lhs.scale - rhs.scale) <= std::numeric_limits<float>::epsilon();
}

struct fixed_mul
{
    float mul;
    int8_t shift;

    int32_t rounded_mul() const noexcept { return (int32_t)roundf(mul); }
};

typedef enum _memory_type
{
    mem_const,
    mem_main,
    mem_k210_kpu
} memory_type_t;

using runtime_shape_t = std::array<int, 4>;
using runtime_paddings_t = std::array<padding, 4>;

struct scalar
{
    datatype_t type;
    std::array<uint8_t, 4> storage;

    scalar() = default;

    template <class T>
    scalar(T &&value) { as<T>() = value; }

    template <class T>
    T &as() noexcept { return *reinterpret_cast<T *>(storage.data()); }

    template <class T>
    const T &as() const noexcept { return *reinterpret_cast<const T *>(storage.data()); }
};

struct memory_range
{
    memory_type_t memory_type;
    datatype_t datatype;
    uint32_t start;
    uint32_t size;
};

inline bool operator==(const padding &lhs, const padding &rhs) noexcept
{
    return lhs.before == rhs.before && lhs.after == rhs.after;
}

inline bool operator!=(const padding &lhs, const padding &rhs) noexcept
{
    return lhs.before != rhs.before || lhs.after != rhs.after;
}

template<class T>
bool operator==(const value_range<T> &lhs, const value_range<T> &rhs) noexcept
{
    return lhs.min == rhs.min && lhs.max == rhs.max;
}

template <class T>
bool operator!=(const value_range<T> &lhs, const value_range<T> &rhs) noexcept
{
    return lhs.min != rhs.min || lhs.max != rhs.max;
}
}
