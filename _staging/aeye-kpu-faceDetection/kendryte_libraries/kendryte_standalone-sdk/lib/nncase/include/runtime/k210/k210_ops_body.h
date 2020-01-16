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
#include "k210_runtime_op_utility.h"
#include "k210_sim_types.h"

namespace nncase
{
namespace runtime
{
    namespace k210
    {
        struct kpu_upload_options : simple_node_body<kpu_upload_options>
        {
            memory_range input;
            memory_range output;
            runtime_shape_t in_shape;
        };

        struct kpu_conv2d_options
        {
            memory_range main_mem_output;
            int32_t batches;
            int32_t reserved0;
            kpu_layer_argument_t layer;
            xtl::span<const kpu_batchnorm_argument_t> batch_norm;
            const kpu_activate_table_t *activation;
            xtl::span<const uint8_t> weights;

            void deserialize(span_reader &reader)
            {
                reader.read(main_mem_output);
                reader.read(batches);
                reader.read(reserved0);
                reader.read(layer);

                auto ic = layer.image_channel_num.data.i_ch_num + 1;
                auto oc = layer.image_channel_num.data.o_ch_num + 1;
                auto filter = get_kpu_filter_size((kpu_filter_type_t)layer.kernel_pool_type_cfg.data.kernel_type);
                auto weights_size = layer.interrupt_enabe.data.depth_wise_layer
                    ? oc * filter * filter
                    : ic * oc * filter * filter;

                reader.skip(layer.kernel_pool_type_cfg.data.bwsx_base_addr);
                reader.read_span(batch_norm, oc);
                reader.skip(layer.kernel_calc_type_cfg.data.active_addr);
                reader.get_ref(activation);
                reader.skip(layer.kernel_load_cfg.data.para_start_addr);
                reader.read_span(weights, weights_size);
#if !NNCASE_TARGET_K210_SIMULATOR
                layer.kernel_pool_type_cfg.data.bwsx_base_addr = (uintptr_t)batch_norm.data();
                layer.kernel_calc_type_cfg.data.active_addr = (uintptr_t)activation;
                layer.kernel_load_cfg.data.para_start_addr = (uintptr_t)weights.data();
#endif
            }

            void serialize(binary_writer &writer)
            {
                writer.write(main_mem_output);
                writer.write(batches);
                writer.write(reserved0);

                auto layer_pos = writer.position();
                writer.position(layer_pos + std::streamoff(sizeof(layer)));
                layer.kernel_pool_type_cfg.data.bwsx_base_addr = (uint32_t)writer.align_position(8);
                writer.write_array(batch_norm);
                layer.kernel_calc_type_cfg.data.active_addr = (uint32_t)writer.align_position(256);
                writer.write(*activation);
                layer.kernel_load_cfg.data.para_start_addr = (uint32_t)writer.align_position(128);
                writer.write_array(weights);

                auto end_pos = writer.position();
                writer.position(layer_pos);
                writer.write(layer);
                writer.position(end_pos);
            }
        };
    }
}
}
