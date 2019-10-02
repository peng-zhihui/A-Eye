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
#include <iostream>
#include <xtl/xspan.hpp>

namespace nncase
{
namespace runtime
{
    class binary_writer
    {
    public:
        binary_writer(std::ostream &stream)
            : stream_(stream)
        {
        }

        template <class T>
        void write(T &&value)
        {
            stream_.write(reinterpret_cast<const char *>(&value), sizeof(value));
        }

        template <class T>
        void write_array(xtl::span<const T> value)
        {
            stream_.write(reinterpret_cast<const char *>(value.data()), value.size_bytes());
        }

        std::streampos position() const
        {
            return stream_.tellp();
        }

        void position(std::streampos pos)
        {
            stream_.seekp(pos);
        }

        std::streamoff align_position(size_t alignment)
        {
            auto pos = position();
            auto rem = pos % alignment;
            if (rem != 0)
            {
                auto off = std::streamoff(alignment - rem);
                position(pos + off);
                return off;
            }

            return 0;
        }

    private:
        std::ostream &stream_;
    };
}
}
