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
#include <cassert>
#include <iostream>
#include <runtime/interpreter.h>
#include <runtime/kernel_registry.h>

using namespace nncase;
using namespace nncase::runtime;

bool interpreter_base::try_load_model(const uint8_t *buffer)
{
    auto offset = buffer;
    model_header_ = reinterpret_cast<const model_header *>(buffer);

    // Validate model
    if (model_header_->identifier != MODEL_IDENTIFIER || model_header_->version != MODEL_VERSION || (model_header_->target != MODEL_TARGET_CPU && model_header_->target != MODEL_TARGET_K210))
        return false;

    // Allocate buffers
    main_mem_.reset(new (std::nothrow) uint8_t[model_header_->main_mem]);
    if (!main_mem_)
        return false;

    offset += sizeof(model_header);
    inputs_ = { reinterpret_cast<const memory_range *>(offset), inputs_size() };
    offset += sizeof(memory_range) * inputs_size();
    input_shapes_ = { reinterpret_cast<const runtime_shape_t *>(offset), inputs_size() };
    offset += sizeof(runtime_shape_t) * inputs_size();
    outputs_ = { reinterpret_cast<const memory_range *>(offset), outputs_size() };
    offset += sizeof(memory_range) * outputs_size();
    constants_ = { offset, model_header_->constants };
    offset += constants_.size();
    node_headers_ = { reinterpret_cast<const node_header *>(offset), nodes_size() };
    offset += sizeof(node_header) * nodes_size();
    node_body_start_ = offset;

    return initialize();
}

bool interpreter_base::initialize()
{
    return true;
}

void interpreter_base::run(run_callback_t callback, error_callback_t on_error, node_profile_callback_t node_profile, void *userdata)
{
    run_callback_ = callback;
    on_error_ = on_error;
    node_profile_ = node_profile;
    userdata_ = userdata;
    cnt_node_ = 0;
    cnt_node_body_ = node_body_start_;
    total_duration_ = {};
    last_time_.reset();
    step();
}

void interpreter_base::step()
{
    auto result = kcr_done;

    while (result == kcr_done)
    {
        if (!last_time_)
        {
            last_time_ = clock_t::now();
        }
        else
        {
            auto now = clock_t::now();
            auto duration = now - *last_time_;
            total_duration_ += duration;
            last_time_ = now;

            if (node_profile_)
                node_profile_(last_op_, duration, userdata_);
        }

        if (cnt_node_ == nodes_size())
        {
            run_callback_(userdata_);
            break;
        }
        else
        {
            auto node_id = cnt_node_++;
            auto header = node_headers_[node_id];
            xtl::span<const uint8_t> body(cnt_node_body_, header.body_size);
            cnt_node_body_ += header.body_size;
            last_op_ = header.opcode;

            result = call_kernel(header.opcode, body, static_cast<interpreter_t &>(*this), &interpreter_base::step);

            if (result == kcr_error)
            {
                if (on_error_)
                {
                    char buffer[256];
                    auto name = node_opcode_names(header.opcode);
                    if (!name.empty())
                        std::sprintf(buffer, "error occurs in running kernel: %s", name.data());
                    else
                        std::sprintf(buffer, "Unknown opcode: (%d)", header.opcode);
                    on_error_(buffer, userdata_);
                }

                break;
            }
        }
    }
}

xtl::span<uint8_t> interpreter_base::memory_at(const memory_range &range) const noexcept
{
    uintptr_t base;

    switch (range.memory_type)
    {
    case mem_const:
        base = (uintptr_t)constants_.data();
        break;
    case mem_main:
        base = (uintptr_t)main_mem_.get();
        break;
    default:
        base = 0;
        assert(!"Invalid memory type");
        break;
    }

    return { reinterpret_cast<uint8_t *>(base + range.start), range.size };
}
