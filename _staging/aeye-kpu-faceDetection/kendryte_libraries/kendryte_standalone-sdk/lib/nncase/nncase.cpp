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
#include <nncase.h>
#include <kernels/k210/k210_kernels.h>
#include <runtime/target_interpreter.h>
#include <stdio.h>

using namespace nncase;
using namespace nncase::runtime;

#define NNCASE_DEBUG 0

namespace
{
void kpu_upload_dma(dmac_channel_number_t dma_ch, const uint8_t *src, uint8_t *dest, size_t input_size, plic_irq_callback_t callback, void *userdata)
{
    dmac_set_irq(dma_ch, callback, userdata, 1);
    dmac_set_single_mode(dma_ch, (void *)src, (void *)dest, DMAC_ADDR_INCREMENT, DMAC_ADDR_INCREMENT,
        DMAC_MSIZE_16, DMAC_TRANS_WIDTH_64, input_size / 8);
    usleep(1);
}
}

class nncase_context
{
public:
    int load_kmodel(const uint8_t *buffer)
    {
        return interpreter_.try_load_model(buffer) ? 0 : -1;
    }

    int get_output(uint32_t index, uint8_t **data, size_t *size)
    {
        if (index >= interpreter_.outputs_size())
            return -1;

        auto mem = interpreter_.memory_at<uint8_t>(interpreter_.output_at(index));
        *data = mem.data();
        *size = mem.size();
        return 0;
    }

    int run_kmodel(const uint8_t *src, dmac_channel_number_t dma_ch, kpu_done_callback_t done_callback, void *userdata)
    {
        done_callback_ = done_callback;
        userdata_ = userdata;
        interpreter_.dma_ch(dma_ch);

        auto input = interpreter_.input_at(0);
        auto mem = interpreter_.memory_at<uint8_t>(input);
        if (input.memory_type == mem_main)
        {
            std::copy(src, src + mem.size(), mem.begin());
            interpreter_.run(done_thunk, on_error_thunk, node_profile_thunk, this);
            return 0;
        }
        else if (input.memory_type == mem_k210_kpu)
        {
            auto shape = interpreter_.input_shape_at(0);
            if (shape[3] % 64 == 0)
            {
                kpu_upload_dma(dma_ch, src, mem.data(), mem.size(), upload_done_thunk, this);
            }
            else
            {
                kernels::k210::kpu_upload(src, mem.data(), shape);
                on_upload_done();
            }

            return 0;
        }

        return -1;
    }

private:
    void on_done()
    {
#if NNCASE_DEBUG
        printf("Total: %fms\n", interpreter_.total_duration().count() / 1e6);
#endif

        if (done_callback_)
            done_callback_(userdata_);
    }

    void on_upload_done()
    {
        interpreter_.run(done_thunk, on_error_thunk, node_profile_thunk, this);
    }

    static void done_thunk(void *userdata)
    {
        reinterpret_cast<nncase_context *>(userdata)->on_done();
    }

    static void on_error_thunk(const char *err, void *userdata)
    {
#if NNCASE_DEBUG
        printf("Fatal: %s\n", err);
#endif
    }

    static void node_profile_thunk(runtime_opcode op, std::chrono::nanoseconds duration, void *userdata)
    {
#if NNCASE_DEBUG
        printf("%s: %fms\n", node_opcode_names(op).data(), duration.count() / 1e6);
#endif
    }

    static int upload_done_thunk(void *userdata)
    {
        reinterpret_cast<nncase_context *>(userdata)->on_upload_done();
        return 0;
    }

private:
    interpreter_t interpreter_;
    kpu_done_callback_t done_callback_;
    void *userdata_;
};

int nncase_load_kmodel(kpu_model_context_t *ctx, const uint8_t *buffer)
{
    auto nnctx = new (std::nothrow) nncase_context();
    if (ctx)
    {
        ctx->is_nncase = 1;
        ctx->nncase_ctx = nnctx;
        return nnctx->load_kmodel(buffer);
    }
    else
    {
        return -1;
    }
}

int nncase_get_output(kpu_model_context_t *ctx, uint32_t index, uint8_t **data, size_t *size)
{
    auto nnctx = reinterpret_cast<nncase_context *>(ctx->nncase_ctx);
    return nnctx->get_output(index, data, size);
}

void nncase_model_free(kpu_model_context_t *ctx)
{
    auto nnctx = reinterpret_cast<nncase_context *>(ctx->nncase_ctx);
    delete nnctx;
    ctx->nncase_ctx = nullptr;
}

int nncase_run_kmodel(kpu_model_context_t *ctx, const uint8_t *src, dmac_channel_number_t dma_ch, kpu_done_callback_t done_callback, void *userdata)
{
    auto nnctx = reinterpret_cast<nncase_context *>(ctx->nncase_ctx);
    return nnctx->run_kmodel(src, dma_ch, done_callback, userdata);
}
