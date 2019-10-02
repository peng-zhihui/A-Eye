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
    class binary_reader
    {
    public:
        binary_reader(std::istream &stream)
            : stream_(stream)
        {
        }

        template <class T>
        T read()
        {
            T value;
            read(value);
            return value;
        }

        template <class T>
        void read(T &value)
        {
            stream_.read(reinterpret_cast<char *>(&value), sizeof(value));
        }

        template <class T>
        void read_array(xtl::span<T> value)
        {
            stream_.read(reinterpret_cast<char *>(value.data()), value.size_bytes());
        }

        std::streampos position() const
        {
            return stream_.tellg();
        }

        void position(std::streampos pos)
        {
            stream_.seekg(pos);
        }

        void skip(std::streamoff off)
        {
            stream_.seekg(off, std::ios::cur);
        }

        size_t avail()
        {
            auto pos = stream_.tellg();
            stream_.seekg(0, std::ios::end);
            auto end = stream_.tellg();
            stream_.seekg(pos);
            return size_t(end - pos);
        }

    private:
        std::istream &stream_;
    };
}
}
