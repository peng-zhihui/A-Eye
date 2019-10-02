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
#include <cassert>

// clang-format off
#define NNCASE_STRINGFY(x) #x
#define NNCASE_CONCAT_2(a, b) a/b
#define NNCASE_CONCAT_3(a, b, c) NNCASE_CONCAT_2(NNCASE_CONCAT_2(a, b), c)
// clang-format on

#define NNCASE_TARGET_HEADER_(prefix, target, name) <NNCASE_CONCAT_3(prefix, target, name)>
#define NNCASE_TARGET_HEADER(prefix, name) NNCASE_TARGET_HEADER_(prefix, NNCASE_TARGET, name)

#ifndef NNCASE_NO_EXCEPTIONS
#include <stdexcept>
#define NNCASE_THROW(exception, ...) throw exception(__VA_ARGS__)
#else
#define NNCASE_THROW(exception, ...) assert(0 && #exception)
#endif
