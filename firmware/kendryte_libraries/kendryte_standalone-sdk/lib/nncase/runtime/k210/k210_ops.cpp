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
#include <kernels/k210/k210_kernels.h>
#include <runtime/k210/k210_ops_body.h>
#include <runtime/kernel_registry.h>
#if !NNCASE_TARGET_K210_SIMULATOR
#include <dmac.h>
#include <sysctl.h>
#endif

using namespace nncase;
using namespace nncase::runtime;
using namespace nncase::runtime::k210;

namespace
{
#if !NNCASE_TARGET_K210_SIMULATOR
void kpu_send_layer(const kpu_layer_argument_t &layer)
{
    kpu->layer_argument_fifo = layer.interrupt_enabe.reg;
    kpu->layer_argument_fifo = layer.image_addr.reg;
    kpu->layer_argument_fifo = layer.image_channel_num.reg;
    kpu->layer_argument_fifo = layer.image_size.reg;
    kpu->layer_argument_fifo = layer.kernel_pool_type_cfg.reg;
    kpu->layer_argument_fifo = layer.kernel_load_cfg.reg;
    kpu->layer_argument_fifo = layer.kernel_offset.reg;
    kpu->layer_argument_fifo = layer.kernel_calc_type_cfg.reg;
    kpu->layer_argument_fifo = layer.write_back_cfg.reg;
    kpu->layer_argument_fifo = layer.conv_value.reg;
    kpu->layer_argument_fifo = layer.conv_value2.reg;
    kpu->layer_argument_fifo = layer.dma_parameter.reg;
}

void kpu_conv2d_normal(kpu_layer_argument_t &layer, plic_irq_callback_t callback, void *userdata)
{
    kpu->interrupt_clear.reg = 0b111;
    kpu->interrupt_mask.reg = 0b110;
    layer.interrupt_enabe.data.int_en = 1;
    plic_irq_register(IRQN_AI_INTERRUPT, callback, userdata);
    plic_irq_enable(IRQN_AI_INTERRUPT);
    kpu_send_layer(layer);
    usleep(1);
}

void kpu_conv2d_output(kpu_layer_argument_t &layer, dmac_channel_number_t dma_ch, uint8_t *dest, plic_irq_callback_t callback, void *userdata)
{
    kpu->interrupt_clear.reg = 0b111;
    kpu->interrupt_mask.reg = 0b111;
    layer.dma_parameter.data.send_data_out = 1;
    sysctl_dma_select((sysctl_dma_channel_t)dma_ch, SYSCTL_DMA_SELECT_AI_RX_REQ);
    dmac_set_irq(dma_ch, callback, userdata, 1);
    dmac_set_single_mode(dma_ch, (void *)(&kpu->fifo_data_out), dest, DMAC_ADDR_NOCHANGE, DMAC_ADDR_INCREMENT,
        DMAC_MSIZE_8, DMAC_TRANS_WIDTH_64, (layer.dma_parameter.data.dma_total_byte + 8) / 8);
    kpu_send_layer(layer);
}

int kpu_plic_thunk(void *userdata)
{
    kpu->interrupt_clear.reg = 0b111;
    kpu->interrupt_mask.reg = 0b111;

    auto &ctx = *reinterpret_cast<k210_interpreter_context *>(userdata);
    (ctx.interpreter->*ctx.step)();
    return 0;
}

void kpu_upload_dma(dmac_channel_number_t dma_ch, const uint8_t *src, uint8_t *dest, size_t input_size, plic_irq_callback_t callback, void *userdata)
{
    dmac_set_irq(dma_ch, callback, userdata, 1);
    dmac_set_single_mode(dma_ch, (void *)src, (void *)dest, DMAC_ADDR_INCREMENT, DMAC_ADDR_INCREMENT,
        DMAC_MSIZE_16, DMAC_TRANS_WIDTH_64, input_size / 8);
    usleep(1);
}

int kpu_dma_plic_thunk(void *userdata)
{
    auto &ctx = *reinterpret_cast<k210_interpreter_context *>(userdata);
    (ctx.interpreter->*ctx.step)();
    return 0;
}
#endif
}

namespace nncase
{
namespace runtime
{
    namespace k210
    {
        kernel_call_result kpu_upload(kpu_upload_options &options, interpreter_t &interpreter, interpreter_step_t step)
        {
            auto input = interpreter.memory_at<uint8_t>(options.input);
            auto output = interpreter.memory_at<uint8_t>(options.output);
#if !NNCASE_TARGET_K210_SIMULATOR
            if (options.in_shape[3] % 64 == 0)
            {
                auto &ctx = interpreter.context();
                ctx.interpreter = &interpreter;
                ctx.step = step;
                kpu_upload_dma(interpreter.dma_ch(), input.data(), output.data(), input.size(), kpu_dma_plic_thunk, &ctx);
                return kcr_async;
            }
#endif
            kernels::k210::kpu_upload(input.data(), output.data(), options.in_shape);
            return kcr_done;
        }

        kernel_call_result kpu_conv2d(kpu_conv2d_options &options, interpreter_t &interpreter, interpreter_step_t step)
        {
#if NNCASE_TARGET_K210_SIMULATOR
            auto input = interpreter.memory_at<uint8_t>({ mem_k210_kpu, dt_uint8, (uint32_t)options.layer.image_addr.data.image_src_addr * 64, 1 });
            auto kpu_out = interpreter.memory_at<uint8_t>({ mem_k210_kpu, dt_uint8, (uint32_t)options.layer.image_addr.data.image_dst_addr * 64, 1 });

            auto in_h = static_cast<int32_t>(options.layer.image_size.data.i_col_high + 1);
            auto in_w = static_cast<int32_t>(options.layer.image_size.data.i_row_wid + 1);
            auto in_ch = static_cast<int32_t>(options.layer.image_channel_num.data.i_ch_num + 1);
            runtime_shape_t in_shape { options.batches, in_ch, in_h, in_w };
            auto in_fmap_size = kernels::details::compute_size(in_shape);

            auto out_h = static_cast<int32_t>(options.layer.image_size.data.o_col_high + 1);
            auto out_w = static_cast<int32_t>(options.layer.image_size.data.o_row_wid + 1);
            auto out_ch = static_cast<int32_t>(options.layer.image_channel_num.data.o_ch_num + 1);
            runtime_shape_t conv_out_shape { options.batches, out_ch, in_h, in_w };
            auto conv_out_fmap_size = kernels::details::compute_size(conv_out_shape);
            runtime_shape_t out_shape { options.batches, out_ch, out_h, out_w };
            auto out_fmap_size = kernels::details::compute_size(out_shape);

            auto input_tmp = std::make_unique<uint8_t[]>(in_fmap_size);
            auto workspace = std::make_unique<int64_t[]>(conv_out_fmap_size);
            auto conv_output_tmp = std::make_unique<uint8_t[]>(conv_out_fmap_size);
            auto output_tmp = std::make_unique<uint8_t[]>(out_fmap_size);

            kernels::k210::kpu_download(input.data(), input_tmp.get(), in_shape);
            auto is_depthwise = options.layer.interrupt_enabe.data.depth_wise_layer != 0;
            auto filter_size = get_kpu_filter_size((kpu_filter_type_t)options.layer.kernel_pool_type_cfg.data.kernel_type);
            auto pad_value = (uint8_t)options.layer.kernel_pool_type_cfg.data.pad_value;
            auto arg_x = (int32_t)kernels::details::to_signed<24>(options.layer.conv_value.data.arg_x);
            auto shift_x = (int32_t)options.layer.conv_value.data.shr_x;
            auto arg_w = (int32_t)kernels::details::to_signed<24>(options.layer.conv_value.data.arg_w);
            auto shift_w = (int32_t)options.layer.conv_value.data.shr_w;
            auto arg_add = kernels::details::to_signed<40>(options.layer.conv_value2.data.arg_add);

            auto batchnorm = std::make_unique<kpu_batchnorm_segment[]>(out_ch);
            for (size_t i = 0; i < out_ch; i++)
            {
                auto &src = options.batch_norm[i].batchnorm.data;
                auto &dest = batchnorm[i];
                dest.mul = (int32_t)kernels::details::to_signed<24>(src.norm_mul);
                dest.shift = (int32_t)src.norm_shift;
                dest.add = (int32_t)kernels::details::to_signed<32>(src.norm_add);
            }

            kpu_activation_table_t activation;
            for (size_t i = 0; i < 16; i++)
            {
                auto &src = options.activation->activate_para[i].data;
                auto &dest = activation[i];
                dest.start_x = kernels::details::to_signed<36>(src.x_start);
                dest.mul = (int32_t)kernels::details::to_signed<16>(src.y_mul);
                dest.shift = (int32_t)src.shift_number;

                if (i < 16)
                    dest.add = options.activation->activate_para_bias0.data.result_bias[i];
                else
                    dest.add = options.activation->activate_para_bias1.data.result_bias[i - 16];
            }

#define KPU_CONV2D_IMPL(is_depthwise_val, filter_size_val)                                                                                        \
    if (is_depthwise == is_depthwise_val && filter_size == filter_size_val)                                                                       \
    kernels::k210::kpu_conv2d<is_depthwise_val, filter_size_val>(input_tmp.get(), workspace.get(), conv_output_tmp.get(), options.weights.data(), \
        in_h, in_w, in_ch, out_ch, pad_value, arg_x, shift_x, arg_w, shift_w, arg_add, batchnorm.get(), activation)

            KPU_CONV2D_IMPL(true, 1);
            else KPU_CONV2D_IMPL(true, 3);
            else KPU_CONV2D_IMPL(false, 1);
            else KPU_CONV2D_IMPL(false, 3);

            kernels::k210::kpu_pool2d(conv_output_tmp.get(), output_tmp.get(), in_h, in_w, out_ch, (kpu_pool_type_t)options.layer.kernel_pool_type_cfg.data.pool_type);
            kernels::k210::kpu_upload(output_tmp.get(), kpu_out.data(), out_shape);
            if (options.main_mem_output.size)
            {
                auto main_output = interpreter.memory_at<uint8_t>(options.main_mem_output);
                std::copy(output_tmp.get(), output_tmp.get() + out_fmap_size, main_output.data());
            }

            return kcr_done;
#else
            auto &ctx = interpreter.context();
            ctx.interpreter = &interpreter;
            ctx.step = step;

            if (options.main_mem_output.size)
            {
                auto main_output = interpreter.memory_at<uint8_t>(options.main_mem_output);
                kpu_conv2d_output(options.layer, interpreter.dma_ch(), main_output.data(), kpu_plic_thunk, &ctx);
            }
            else
            {
                kpu_conv2d_normal(options.layer, kpu_plic_thunk, &ctx);
            }

            return kcr_async;
#endif
        }
    }
}
}
