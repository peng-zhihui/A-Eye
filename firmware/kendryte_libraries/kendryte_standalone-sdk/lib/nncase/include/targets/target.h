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
#include <ir/quantizer.h>
#include <memory>
#include <scheduler/memory_allocator.h>
#include <transforms/transform.h>
#include <unordered_map>
#include <vector>

namespace nncase
{
class target
{
public:
    virtual void fill_allocators(std::unordered_map<memory_type_t, scheduler::memory_allocator *> &allocators, std::vector<std::unique_ptr<scheduler::memory_allocator>> &allocator_holders) = 0;
    virtual void registry_codegen_ops() = 0;
    virtual void registry_evaluator_ops() = 0;
    virtual void add_default_transforms(std::vector<std::unique_ptr<transforms::transform>> &transforms) = 0;
    virtual void add_optimize1_transforms(std::vector<std::unique_ptr<transforms::transform>> &transforms) = 0;
    virtual void add_optimize2_transforms(std::vector<std::unique_ptr<transforms::transform>> &transforms) = 0;
    virtual void add_quantization_checkpoint_transforms(std::vector<std::unique_ptr<transforms::transform>> &transforms) = 0;
    virtual void add_quantization_transforms(ir::quantizer& quantizer, const quant_param_t& input_quant_param, std::vector<std::unique_ptr<transforms::transform>> &transforms) = 0;
};
}
