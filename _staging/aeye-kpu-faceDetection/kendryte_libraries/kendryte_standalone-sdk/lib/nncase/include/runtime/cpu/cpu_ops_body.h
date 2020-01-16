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
#include "../node_body.h"

namespace nncase
{
namespace runtime
{
    namespace cpu
    {
        struct cpu_conv2d_options
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
            int32_t out_channels;
            padding padding_h;
            padding padding_w;
            int32_t filter_h;
            int32_t filter_w;
            int32_t stride_h;
            int32_t stride_w;
            int32_t dilation_h;
            int32_t dilation_w;
            value_range<float> fused_activation;
            xtl::span<const float> weights;
            xtl::span<const float> bias;

            void deserialize(span_reader &reader)
            {
                reader.read(input);
                reader.read(output);
                reader.read(in_shape);
                reader.read(out_channels);
                reader.read(padding_h);
                reader.read(padding_w);
                reader.read(filter_h);
                reader.read(filter_w);
                reader.read(stride_h);
                reader.read(stride_w);
                reader.read(dilation_h);
                reader.read(dilation_w);
                reader.read(fused_activation);
                reader.read_span(weights, (size_t)out_channels * in_shape[3] * filter_h * filter_w);
                reader.read_span(bias, out_channels);
            }
        };

        struct cpu_depthwise_conv2d_options
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
            padding padding_h;
            padding padding_w;
            int32_t filter_h;
            int32_t filter_w;
            int32_t stride_h;
            int32_t stride_w;
            int32_t dilation_h;
            int32_t dilation_w;
            value_range<float> fused_activation;
            xtl::span<const float> weights;
            xtl::span<const float> bias;

            void deserialize(span_reader &reader)
            {
                reader.read(input);
                reader.read(output);
                reader.read(in_shape);
                reader.read(padding_h);
                reader.read(padding_w);
                reader.read(filter_h);
                reader.read(filter_w);
                reader.read(stride_h);
                reader.read(stride_w);
                reader.read(dilation_h);
                reader.read(dilation_w);
                reader.read(fused_activation);
                reader.read_span(weights, (size_t)in_shape[3] * filter_h * filter_w);
                reader.read_span(bias, in_shape[3]);
            }
        };

        struct cpu_reduce_window2d_options : simple_node_body<cpu_reduce_window2d_options>
        {
            memory_range input;
            memory_range output;
            reduce_op_t reduce_op;
            runtime_shape_t in_shape;
            padding padding_h;
            padding padding_w;
            int32_t filter_h;
            int32_t filter_w;
            int32_t stride_h;
            int32_t stride_w;
            int32_t dilation_h;
            int32_t dilation_w;
            float init_value;
            value_range<float> fused_activation;
        };

        struct cpu_quantized_conv2d_options
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
            int32_t out_channels;
            padding padding_h;
            padding padding_w;
            int32_t filter_h;
            int32_t filter_w;
            int32_t stride_h;
            int32_t stride_w;
            int32_t dilation_h;
            int32_t dilation_w;
            int32_t input_offset;
            int32_t filter_offset;
            int32_t output_mul;
            int32_t output_shift;
            int32_t output_offset;
            xtl::span<const uint8_t> weights;
            xtl::span<const int32_t> bias;

            void deserialize(span_reader &reader)
            {
                reader.read(input);
                reader.read(output);
                reader.read(in_shape);
                reader.read(out_channels);
                reader.read(padding_h);
                reader.read(padding_w);
                reader.read(filter_h);
                reader.read(filter_w);
                reader.read(stride_h);
                reader.read(stride_w);
                reader.read(dilation_h);
                reader.read(dilation_w);
                reader.read(input_offset);
                reader.read(filter_offset);
                reader.read(output_mul);
                reader.read(output_shift);
                reader.read(output_offset);
                reader.read_span(weights, (size_t)out_channels * in_shape[3] * filter_h * filter_w);
                reader.read_span(bias, out_channels);
            }
        };

        struct cpu_quantized_depthwise_conv2d_options
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
            padding padding_h;
            padding padding_w;
            int32_t filter_h;
            int32_t filter_w;
            int32_t stride_h;
            int32_t stride_w;
            int32_t dilation_h;
            int32_t dilation_w;
            int32_t input_offset;
            int32_t filter_offset;
            int32_t output_mul;
            int32_t output_shift;
            int32_t output_offset;
            xtl::span<const uint8_t> weights;
            xtl::span<const int32_t> bias;

            void deserialize(span_reader &reader)
            {
                reader.read(input);
                reader.read(output);
                reader.read(in_shape);
                reader.read(padding_h);
                reader.read(padding_w);
                reader.read(filter_h);
                reader.read(filter_w);
                reader.read(stride_h);
                reader.read(stride_w);
                reader.read(dilation_h);
                reader.read(dilation_w);
                reader.read(input_offset);
                reader.read(filter_offset);
                reader.read(output_mul);
                reader.read(output_shift);
                reader.read(output_offset);
                reader.read_span(weights, (size_t)in_shape[3] * filter_h * filter_w);
                reader.read_span(bias, in_shape[3]);
            }
        };
    }
}
}
