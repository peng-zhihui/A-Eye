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
#include "../kernel_utils.h"
#include <runtime/runtime_op_utility.h>

namespace nncase
{
namespace kernels
{
    namespace cpu
    {
        inline void conv2d(const float *input, float *output, const float *weights, const float *bias, const runtime_shape_t &in_shape,
            int32_t out_channels, int32_t filter_h, int32_t filter_w, int32_t stride_h, int32_t stride_w, int32_t dilation_h, int32_t dilation_w,
            const padding &padding_h, const padding &padding_w, const value_range<float> &fused_activation)
        {
            const auto out_h = details::get_windowed_output_size(in_shape[1], filter_h, stride_h, dilation_h, padding_h);
            const auto out_w = details::get_windowed_output_size(in_shape[2], filter_w, stride_w, dilation_w, padding_w);

            for (int batch = 0; batch < in_shape[0]; batch++)
            {
                auto in_batch = input + (size_t)batch * in_shape[1] * in_shape[2] * in_shape[3];

                for (int oy = 0; oy < out_h; oy++)
                {
                    for (int ox = 0; ox < out_w; ox++)
                    {
                        int in_y_origin = (oy * stride_h) - padding_h.before;
                        int in_x_origin = (ox * stride_w) - padding_w.before;
                        int filter_y_start = std::max(0, (-in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_y_end = std::min(filter_h, (in_shape[1] - in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_xSstart = std::max(0, (-in_x_origin + dilation_w - 1) / dilation_w);
                        int filter_x_end = std::min(filter_w, (in_shape[2] - in_x_origin + dilation_w - 1) / dilation_w);

                        for (int oc = 0; oc < out_channels; oc++)
                        {
                            auto w_oc = weights + (size_t)oc * filter_h * filter_w * in_shape[3];
                            float value = bias[oc];

                            for (int ky = filter_y_start; ky < filter_y_end; ky++)
                            {
                                for (int kx = filter_xSstart; kx < filter_x_end; kx++)
                                {
                                    int in_y = in_y_origin + dilation_h * ky;
                                    int in_x = in_x_origin + dilation_w * kx;

                                    auto in_pix = in_batch + ((size_t)in_y * in_shape[2] + in_x) * in_shape[3];
                                    auto w_pix = w_oc + ((size_t)ky * filter_w + kx) * in_shape[3];

                                    for (int ic = 0; ic < in_shape[3]; ic++)
                                        value += in_pix[ic] * w_pix[ic];
                                }
                            }

                            *output++ = details::apply_activation(value, fused_activation);
                        }
                    }
                }
            }
        }

        inline void depthwise_conv2d(const float *input, float *output, const float *weights, const float *bias, const runtime_shape_t &in_shape,
            int32_t filter_h, int32_t filter_w, int32_t stride_h, int32_t stride_w, int32_t dilation_h, int32_t dilation_w,
            const padding &padding_h, const padding &padding_w, const value_range<float> &fused_activation)
        {
            const auto out_h = details::get_windowed_output_size(in_shape[1], filter_h, stride_h, dilation_h, padding_h);
            const auto out_w = details::get_windowed_output_size(in_shape[2], filter_w, stride_w, dilation_w, padding_w);

            for (int batch = 0; batch < in_shape[0]; batch++)
            {
                auto in_batch = input + (size_t)batch * in_shape[1] * in_shape[2] * in_shape[3];

                for (int oy = 0; oy < out_h; oy++)
                {
                    for (int ox = 0; ox < out_w; ox++)
                    {
                        int in_y_origin = (oy * stride_h) - padding_h.before;
                        int in_x_origin = (ox * stride_w) - padding_w.before;
                        int filter_y_start = std::max(0, (-in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_y_end = std::min(filter_h, (in_shape[1] - in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_xSstart = std::max(0, (-in_x_origin + dilation_w - 1) / dilation_w);
                        int filter_x_end = std::min(filter_w, (in_shape[2] - in_x_origin + dilation_w - 1) / dilation_w);

                        for (int oc = 0; oc < in_shape[3]; oc++)
                        {
                            auto w_oc = weights + (size_t)oc * filter_h * filter_w;
                            float value = bias[oc];

                            for (int ky = filter_y_start; ky < filter_y_end; ky++)
                            {
                                for (int kx = filter_xSstart; kx < filter_x_end; kx++)
                                {
                                    int in_y = in_y_origin + dilation_h * ky;
                                    int in_x = in_x_origin + dilation_w * kx;

                                    auto in_pix = in_batch + ((size_t)in_y * in_shape[2] + in_x) * in_shape[3];
                                    auto w_pix = w_oc + ((size_t)ky * filter_w + kx);

                                    value += in_pix[oc] * w_pix[0];
                                }
                            }

                            *output++ = details::apply_activation(value, fused_activation);
                        }
                    }
                }
            }
        }

        template <class TBinaryOp, class TOutputOp>
        void reduce_window2d(const float *input, float *output, float init_value, const runtime_shape_t &in_shape,
            int32_t filter_h, int32_t filter_w, int32_t stride_h, int32_t stride_w, int32_t dilation_h, int32_t dilation_w,
            const padding &padding_h, const padding &padding_w, const value_range<float> &fused_activation, TBinaryOp &&binary_op, TOutputOp &&window_op)
        {
            const auto out_h = details::get_windowed_output_size(in_shape[1], filter_h, stride_h, dilation_h, padding_h);
            const auto out_w = details::get_windowed_output_size(in_shape[2], filter_w, stride_w, dilation_w, padding_w);

            for (int batch = 0; batch < in_shape[0]; batch++)
            {
                auto in_batch = input + (size_t)batch * in_shape[1] * in_shape[2] * in_shape[3];

                for (int oy = 0; oy < out_h; oy++)
                {
                    for (int ox = 0; ox < out_w; ox++)
                    {
                        int in_y_origin = (oy * stride_h) - padding_h.before;
                        int in_x_origin = (ox * stride_w) - padding_w.before;
                        int filter_y_start = std::max(0, (-in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_y_end = std::min(filter_h, (in_shape[1] - in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_xSstart = std::max(0, (-in_x_origin + dilation_w - 1) / dilation_w);
                        int filter_x_end = std::min(filter_w, (in_shape[2] - in_x_origin + dilation_w - 1) / dilation_w);

                        for (int oc = 0; oc < in_shape[3]; oc++)
                        {
                            float value = init_value;
                            int32_t kernel_count = 0;

                            for (int ky = filter_y_start; ky < filter_y_end; ky++)
                            {
                                for (int kx = filter_xSstart; kx < filter_x_end; kx++)
                                {
                                    int in_y = in_y_origin + dilation_h * ky;
                                    int in_x = in_x_origin + dilation_w * kx;

                                    auto in_pix = in_batch + ((size_t)in_y * in_shape[2] + in_x) * in_shape[3];

                                    value = binary_op(value, in_pix[oc]);
                                    kernel_count++;
                                }
                            }

                            *output++ = details::apply_activation(window_op(value, kernel_count), fused_activation);
                        }
                    }
                }
            }
        }

        inline void quantized_conv2d(const uint8_t *input, uint8_t *output, const uint8_t *weights, const int32_t *bias, const runtime_shape_t &in_shape,
            int32_t out_channels, int32_t filter_h, int32_t filter_w, int32_t stride_h, int32_t stride_w, int32_t dilation_h, int32_t dilation_w,
            const padding &padding_h, const padding &padding_w, int32_t input_offset, int32_t filter_offset, int32_t output_mul, int32_t output_shift, int32_t output_offset)
        {
            const auto out_h = details::get_windowed_output_size(in_shape[1], filter_h, stride_h, dilation_h, padding_h);
            const auto out_w = details::get_windowed_output_size(in_shape[2], filter_w, stride_w, dilation_w, padding_w);

            for (int batch = 0; batch < in_shape[0]; batch++)
            {
                auto in_batch = input + (size_t)batch * in_shape[1] * in_shape[2] * in_shape[3];

                for (int oy = 0; oy < out_h; oy++)
                {
                    for (int ox = 0; ox < out_w; ox++)
                    {
                        int in_y_origin = (oy * stride_h) - padding_h.before;
                        int in_x_origin = (ox * stride_w) - padding_w.before;
                        int filter_y_start = std::max(0, (-in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_y_end = std::min(filter_h, (in_shape[1] - in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_xSstart = std::max(0, (-in_x_origin + dilation_w - 1) / dilation_w);
                        int filter_x_end = std::min(filter_w, (in_shape[2] - in_x_origin + dilation_w - 1) / dilation_w);

                        for (int oc = 0; oc < out_channels; oc++)
                        {
                            auto w_oc = weights + (size_t)oc * filter_h * filter_w * in_shape[3];
                            int32_t value = bias[oc];

                            for (int ky = filter_y_start; ky < filter_y_end; ky++)
                            {
                                for (int kx = filter_xSstart; kx < filter_x_end; kx++)
                                {
                                    int in_y = in_y_origin + dilation_h * ky;
                                    int in_x = in_x_origin + dilation_w * kx;

                                    auto in_pix = in_batch + ((size_t)in_y * in_shape[2] + in_x) * in_shape[3];
                                    auto w_pix = w_oc + ((size_t)ky * filter_w + kx) * in_shape[3];

                                    for (int ic = 0; ic < in_shape[3]; ic++)
                                        value += (in_pix[ic] - input_offset) * (w_pix[ic] - filter_offset);
                                }
                            }

                            value = runtime::mul_and_carry_shift(value, output_mul, output_shift) + output_offset;
                            *output++ = (uint8_t)std::clamp(value, 0, 255);
                        }
                    }
                }
            }
        }

        inline void quantized_depthwise_conv2d(const uint8_t *input, uint8_t *output, const uint8_t *weights, const int32_t *bias, const runtime_shape_t &in_shape,
            int32_t filter_h, int32_t filter_w, int32_t stride_h, int32_t stride_w, int32_t dilation_h, int32_t dilation_w,
            const padding &padding_h, const padding &padding_w, int32_t input_offset, int32_t filter_offset, int32_t output_mul, int32_t output_shift, int32_t output_offset)
        {
            const auto out_h = details::get_windowed_output_size(in_shape[1], filter_h, stride_h, dilation_h, padding_h);
            const auto out_w = details::get_windowed_output_size(in_shape[2], filter_w, stride_w, dilation_w, padding_w);

            for (int batch = 0; batch < in_shape[0]; batch++)
            {
                auto in_batch = input + (size_t)batch * in_shape[1] * in_shape[2] * in_shape[3];

                for (int oy = 0; oy < out_h; oy++)
                {
                    for (int ox = 0; ox < out_w; ox++)
                    {
                        int in_y_origin = (oy * stride_h) - padding_h.before;
                        int in_x_origin = (ox * stride_w) - padding_w.before;
                        int filter_y_start = std::max(0, (-in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_y_end = std::min(filter_h, (in_shape[1] - in_y_origin + dilation_h - 1) / dilation_h);
                        int filter_xSstart = std::max(0, (-in_x_origin + dilation_w - 1) / dilation_w);
                        int filter_x_end = std::min(filter_w, (in_shape[2] - in_x_origin + dilation_w - 1) / dilation_w);

                        for (int oc = 0; oc < in_shape[3]; oc++)
                        {
                            auto w_oc = weights + (size_t)oc * filter_h * filter_w;
                            int32_t value = bias[oc];

                            for (int ky = filter_y_start; ky < filter_y_end; ky++)
                            {
                                for (int kx = filter_xSstart; kx < filter_x_end; kx++)
                                {
                                    int in_y = in_y_origin + dilation_h * ky;
                                    int in_x = in_x_origin + dilation_w * kx;

                                    auto in_pix = in_batch + ((size_t)in_y * in_shape[2] + in_x) * in_shape[3];
                                    auto w_pix = w_oc + ((size_t)ky * filter_w + kx);

                                    value += (in_pix[oc] - input_offset) * (w_pix[0] - filter_offset);
                                }
                            }

                            value = runtime::mul_and_carry_shift(value, output_mul, output_shift) + output_offset;
                            *output++ = (uint8_t)std::clamp(value, 0, 255);
                        }
                    }
                }
            }
        }
    }
}
}
