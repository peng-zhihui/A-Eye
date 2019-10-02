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
    namespace neutral
    {
        struct binary_options : public simple_node_body<binary_options>
        {
            memory_range input_a;
            memory_range input_b;
            memory_range output;
            binary_op_t binary_op;
            runtime_shape_t in_a_shape;
            runtime_shape_t in_b_shape;
            runtime_shape_t out_shape;
            value_range<float> fused_activation;
        };

        struct concat_options
        {
            memory_range output;
            uint32_t inner_size;
            uint32_t outer_size;
            uint32_t inputs_count;
            xtl::span<const memory_range> inputs;
            xtl::span<const int32_t> dims;

            void deserialize(span_reader &reader)
            {
                reader.read(output);
                reader.read(inner_size);
                reader.read(outer_size);
                reader.read(inputs_count);
                reader.read_span(inputs, inputs_count);
                reader.read_span(dims, inputs_count);
            }

            void serialize(binary_writer &writer) const
            {
                writer.write(output);
                writer.write(inner_size);
                writer.write(outer_size);
                writer.write(inputs_count);
                writer.write_array(inputs);
                writer.write_array(dims);
            }
        };

        struct conv2d_options
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
            int32_t groups;
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
                reader.read(groups);
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
                reader.read_span(weights, (size_t)out_channels * in_shape[1] / groups * filter_h * filter_w);
                reader.read_span(bias, out_channels);
            }

            void serialize(binary_writer &writer) const
            {
                writer.write(input);
                writer.write(output);
                writer.write(in_shape);
                writer.write(groups);
                writer.write(out_channels);
                writer.write(padding_h);
                writer.write(padding_w);
                writer.write(filter_h);
                writer.write(filter_w);
                writer.write(stride_h);
                writer.write(stride_w);
                writer.write(dilation_h);
                writer.write(dilation_w);
                writer.write(fused_activation);
                writer.write_array(weights);
                writer.write_array(bias);
            }
        };

        struct dequantize_options : public simple_node_body<dequantize_options>
        {
            memory_range input;
            memory_range output;
            quant_param_t quant_param;
        };

        struct matmul_options
        {
            memory_range input_a;
            memory_range input_b;
            memory_range output;
            int32_t a_rows;
            int32_t a_cols;
            int32_t b_cols;
            value_range<float> fused_activation;
            xtl::span<const float> bias;

            void deserialize(span_reader &reader)
            {
                reader.read(input_a);
                reader.read(input_b);
                reader.read(output);
                reader.read(a_rows);
                reader.read(a_cols);
                reader.read(b_cols);
                reader.read(fused_activation);
                reader.read_span(bias, b_cols);
            }

            void serialize(binary_writer &writer) const
            {
                writer.write(input_a);
                writer.write(input_b);
                writer.write(output);
                writer.write(a_rows);
                writer.write(a_cols);
                writer.write(b_cols);
                writer.write(fused_activation);
                writer.write_array(bias);
            }
        };

        struct memory_copy_options : public simple_node_body<memory_copy_options>
        {
            memory_range input;
            memory_range output;
        };

        struct pad_options : public simple_node_body<pad_options>
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
            runtime_paddings_t paddings;
            scalar pad_value;
        };

        struct quantize_options : public simple_node_body<quantize_options>
        {
            memory_range input;
            memory_range output;
            quant_param_t quant_param;
        };

        struct reduce_options : public simple_node_body<reduce_options>
        {
            memory_range input;
            memory_range output;
            reduce_op_t reduce_op;
            runtime_shape_t in_shape;
            runtime_shape_t out_shape;
            float init_value;
        };

        struct reduce_window2d_options : simple_node_body<reduce_window2d_options>
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

        struct resize_image_options : public simple_node_body<resize_image_options>
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
            int32_t out_h;
            int32_t out_w;
            image_resize_mode_t mode;
            bool align_corners;
        };

        struct softmax_options : public simple_node_body<softmax_options>
        {
            memory_range input;
            memory_range output;
            int32_t inner_size;
            int32_t outer_size;
            float beta;
        };

        struct transpose_options : public simple_node_body<transpose_options>
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
            runtime_shape_t perm;
        };

        struct strided_slice_options : public simple_node_body<strided_slice_options>
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
            runtime_shape_t begin;
            runtime_shape_t end;
            runtime_shape_t strides;
            int32_t begin_mask;
            int32_t end_mask;
            int32_t ellipsis_mask;
            int32_t new_axis_mask;
            int32_t shrink_axis_mask;
        };

        struct unary_options : public simple_node_body<unary_options>
        {
            memory_range input;
            memory_range output;
            unary_op_t unary_op;
        };
    }
}
}
