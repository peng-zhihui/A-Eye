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
#include <runtime/interpreter.h>

namespace nncase
{
namespace runtime
{
    namespace k210
    {
        struct k210_interpreter_context
        {
            interpreter_base *interpreter;
            interpreter_step_t step;
        };

        class interpreter : public interpreter_base
        {
        public:
            using interpreter_base::memory_at;

            interpreter();

#if !NNCASE_TARGET_K210_SIMULATOR

            dmac_channel_number_t dma_ch() const noexcept { return dma_ch_; }
            void dma_ch(dmac_channel_number_t dma_ch) noexcept { dma_ch_ = dma_ch; }
            k210_interpreter_context &context() noexcept { return context_; }
#endif

        protected:
            xtl::span<uint8_t> memory_at(const memory_range &range) const noexcept override;

        private:
#if NNCASE_TARGET_K210_SIMULATOR
            std::unique_ptr<uint8_t[]> kpu_mem_;
#else
            dmac_channel_number_t dma_ch_;
            k210_interpreter_context context_;
#endif
        };
    }
}
}
