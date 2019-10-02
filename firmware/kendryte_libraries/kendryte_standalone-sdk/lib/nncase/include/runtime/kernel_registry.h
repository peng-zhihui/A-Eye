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
#include "target_interpreter.h"
#include <datatypes.h>
#include <runtime/runtime_op.h>
#include <xtl/xspan.hpp>

namespace nncase
{
namespace runtime
{
    enum kernel_call_result
    {
        kcr_done,
        kcr_async,
        kcr_error
    };

    kernel_call_result call_kernel(runtime_opcode opcode, xtl::span<const uint8_t> body, interpreter_t &interpreter, interpreter_step_t step);
}
}
