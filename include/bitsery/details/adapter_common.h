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

#ifndef BITSERY_DETAILS_ADAPTER_COMMON_H
#define BITSERY_DETAILS_ADAPTER_COMMON_H

#include <algorithm>
#include <utility>
#include <cassert>
#include <vector>
#include <stack>
#include <cstring>
#include "adapter_utils.h"
#include "not_defined_type.h"

#include "../common.h"

namespace bitsery {

    namespace details {

        template<typename T>
        struct BitsSize:public std::integral_constant<size_t, sizeof(T) * 8> {

        };

        //add swap functions to class, to avoid compilation warning about unused functions
        struct SwapImpl {
            static uint64_t exec(uint64_t value) {
#ifdef __GNUC__
                return __builtin_bswap64(value);
#else
                value = ( value & 0x00000000FFFFFFFF ) << 32 | ( value & 0xFFFFFFFF00000000 ) >> 32;
            value = ( value & 0x0000FFFF0000FFFF ) << 16 | ( value & 0xFFFF0000FFFF0000 ) >> 16;
            value = ( value & 0x00FF00FF00FF00FF ) << 8  | ( value & 0xFF00FF00FF00FF00 ) >> 8;
            return value;
#endif
            }

            static uint32_t exec(uint32_t value) {
#ifdef __GNUC__
                return __builtin_bswap32(value);
#else
                return ( value & 0x000000ff ) << 24 | ( value & 0x0000ff00 ) << 8 | ( value & 0x00ff0000 ) >> 8 | ( value & 0xff000000 ) >> 24;
#endif
            }

            static uint16_t exec(uint16_t value) {
                return (value & 0x00ff) << 8 | (value & 0xff00) >> 8;
            }

            static uint8_t exec(uint8_t value) {
                return value;
            }
        };

        template<typename TValue>
        TValue swap(TValue value) {
            constexpr size_t TSize = sizeof(TValue);
            using UT = typename std::conditional<TSize == 1, uint8_t,
                    typename std::conditional<TSize == 2, uint16_t,
                            typename std::conditional<TSize == 4, uint32_t, uint64_t>::type>::type>::type;
            return SwapImpl::exec(static_cast<UT>(value));
        }

        //add test data in separate struct, because some compilers only support constexpr functions with return-only body
        struct EndiannessTestData {
            static constexpr uint32_t _sample4Bytes = 0x01020304;
            static constexpr uint8_t _sample1stByte = (const uint8_t &) _sample4Bytes;
        };

        constexpr EndiannessType getSystemEndianness() {
            static_assert(EndiannessTestData::_sample1stByte == 0x04 || EndiannessTestData::_sample1stByte == 0x01,
                          "system must be either little or big endian");
            return EndiannessTestData::_sample1stByte == 0x04 ? EndiannessType::LittleEndian
                                                              : EndiannessType::BigEndian;
        }


        template<typename T>
        struct ScratchType {
            using type = NotDefinedType;
        };

        template<>
        struct ScratchType<uint8_t> {
            using type = uint16_t;
        };

        /*
         * class used by session reader, to access underlying iterators of buffer
         */
        struct SessionAccess {
            template <typename TReader, typename Iterator>
            static Iterator& posIteratorRef(TReader& r) {
                return r.posIt;
            }
            template <typename TReader, typename Iterator>
            static Iterator& endIteratorRef(TReader& r) {
                return r.endIt;
            }
        };

    }
}


#endif //BITSERY_DETAILS_ADAPTER_COMMON_H
