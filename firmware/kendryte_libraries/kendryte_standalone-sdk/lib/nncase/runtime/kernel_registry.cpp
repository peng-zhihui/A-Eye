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
#include <runtime/cpu/cpu_ops_body.h>
#include <runtime/k210/k210_ops_body.h>
#include <runtime/kernel_registry.h>
#include <runtime/neutral/neutral_ops_body.h>
#include <runtime/span_reader.h>

using namespace nncase;
using namespace nncase::runtime;

namespace nncase
{
namespace runtime
{
#define BEGINE_DEFINE_TARGET(target) \
    namespace target                 \
    {

#define DEFINE_NEUTRAL_RUNTIME_OP(id, name, value) \
    kernel_call_result id(id##_options &, interpreter_t &, interpreter_step_t);
#define DEFINE_RUNTIME_OP(target, id, name, value) \
    kernel_call_result id(id##_options &, interpreter_t &, interpreter_step_t);

#define END_DEFINE_TARGET() }

#include <runtime/runtime_op.def>

#undef BEGINE_DEFINE_TARGET
#undef DEFINE_NEUTRAL_RUNTIME_OP
#undef DEFINE_RUNTIME_OP
#undef END_DEFINE_TARGET
}
}

kernel_call_result runtime::call_kernel(runtime_opcode opcode, xtl::span<const uint8_t> body, interpreter_t &interpreter, interpreter_step_t step)
{
    span_reader reader(body);

    switch (opcode)
    {
#define BEGINE_DEFINE_TARGET(...)
#define DEFINE_NEUTRAL_RUNTIME_OP(id, name, value)                       \
    case rop_##id:                                                       \
    {                                                                    \
        nncase::runtime::neutral::id##_options options;                  \
        options.deserialize(reader);                                     \
        return nncase::runtime::neutral::id(options, interpreter, step); \
    }
#define DEFINE_RUNTIME_OP(target, id, name, value)                      \
    case rop_##target##_##id:                                           \
    {                                                                   \
        nncase::runtime::target::id##_options options;                  \
        options.deserialize(reader);                                    \
        return nncase::runtime::target::id(options, interpreter, step); \
    }
#define END_DEFINE_TARGET()

#include <runtime/runtime_op.def>

#undef BEGINE_DEFINE_TARGET
#undef DEFINE_NEUTRAL_RUNTIME_OP
#undef DEFINE_RUNTIME_OP
#undef END_DEFINE_TARGET
    default:
        return kcr_error;
    }
}
