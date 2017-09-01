//MIT License
//
//Copyright (c) 2017 Mindaugas Vinkelis
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.


#ifndef BITSERY_COMMON_H
#define BITSERY_COMMON_H

#include "details/buffer_common.h"
#include <vector>

#include <list>

namespace bitsery {

    struct DefaultConfig {
        static constexpr EndiannessType NetworkEndianness = EndiannessType::LittleEndian;
        static constexpr bool FixedBufferSize = false;//false means that buffer is resizable and will be used back_insert_iterator for insertion, for reading has no effect.
        using BufferType = std::vector<uint8_t>;//buffer value type must be unsigned, currently only uint8_t supported
    };

/*
 * serializer macro, serialize function specialization that accepts T& and const T&
 */

#define SERIALIZE(ObjectType) \
template <typename S, typename T, typename std::enable_if<std::is_same<T, ObjectType>::value || std::is_same<T, const ObjectType>::value>::type* = nullptr> \
void serialize(S& s, T& o)

#define SERIALIZE_FRIEND(ObjectType) \
template <typename S, typename T, typename std::enable_if<std::is_same<T, ObjectType>::value || std::is_same<T, const ObjectType>::value>::type* = nullptr> \
friend void serialize(S& s, T& o)

}


#endif //BITSERY_COMMON_H
