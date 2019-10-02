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
#include "../datatypes.h"
#include "runtime_op.h"

namespace nncase
{
namespace runtime
{
    enum model_target : uint32_t
    {
        MODEL_TARGET_CPU = 0,
        MODEL_TARGET_K210 = 1
    };

    struct model_header
    {
        uint32_t identifier;
        uint32_t version;
        uint32_t flags;
        model_target target;
        uint32_t constants;
        uint32_t main_mem;
        uint32_t nodes;
        uint32_t inputs;
        uint32_t outputs;
        uint32_t reserved0;
    };

    constexpr uint32_t MODEL_IDENTIFIER = 'KMDL';
    constexpr uint32_t MODEL_VERSION = 4;

    struct node_header
    {
        runtime_opcode opcode;
        uint32_t body_size;
    };
}
}
