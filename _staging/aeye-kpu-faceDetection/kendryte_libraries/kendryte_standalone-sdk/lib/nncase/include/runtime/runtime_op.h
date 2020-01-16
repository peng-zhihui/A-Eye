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
#include <string_view>

namespace nncase
{
namespace runtime
{
#define BEGINE_DEFINE_TARGET(...)
#define DEFINE_NEUTRAL_RUNTIME_OP(id, name, value) rop_##id = value,
#define DEFINE_RUNTIME_OP(target, id, name, value) rop_##target##_##id = value,
#define END_DEFINE_TARGET()

    enum runtime_opcode : uint32_t
    {
#include "runtime_op.def"
    };

#undef DEFINE_NEUTRAL_RUNTIME_OP
#undef DEFINE_RUNTIME_OP
#define DEFINE_NEUTRAL_RUNTIME_OP(id, name, value) \
    case rop_##id:                                 \
        return #name;
#define DEFINE_RUNTIME_OP(target, id, name, value) \
    case rop_##target##_##id:                      \
        return #name;

    constexpr std::string_view node_opcode_names(runtime_opcode opcode)
    {
        switch (opcode)
        {
#include "runtime_op.def"
        default:
            return {};
        }
    }

#undef BEGINE_DEFINE_TARGET
#undef DEFINE_NEUTRAL_RUNTIME_OP
#undef DEFINE_RUNTIME_OP
#undef END_DEFINE_TARGET
}
}
