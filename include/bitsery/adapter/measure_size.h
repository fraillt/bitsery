//MIT License
//
//Copyright (c) 2019 Mindaugas Vinkelis
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

#ifndef BITSERY_ADAPTER_MEASURE_SIZE_H
#define BITSERY_ADAPTER_MEASURE_SIZE_H

#include <cstddef>
#include <type_traits>
#include "../details/adapter_common.h"

namespace bitsery {


    template<typename Config>
    class BasicMeasureSize {
    public:

        static constexpr bool BitPackingEnabled = true;
        using TConfig = Config;
        using TValue = void;

        template<size_t SIZE, typename T>
        void writeBytes(const T&) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            _currPosBits += details::BitsSize<T>::value;
        }

        template<size_t SIZE, typename T>
        void writeBuffer(const T*, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            _currPosBits += details::BitsSize<T>::value * count;
        }

        template<typename T>
        void writeBits(const T&, size_t bitsCount) {
            static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
            assert(bitsCount <= details::BitsSize<T>::value);
            _currPosBits += bitsCount;
        }

        void currentWritePos(size_t pos) {
            align();
            const auto newPos = pos * 8;
            if (_currPosBits > newPos)
                _prevLargestPos = _currPosBits;
            _currPosBits = newPos;
        }

        size_t currentWritePos() const {
            return _currPosBits / 8;
        }

        void align() {
            auto _scratch = (_currPosBits % 8);
            _currPosBits += (8 - _scratch) % 8;
        }

        void flush() {
            align();
        }

        //get size in bytes
        size_t writtenBytesCount() const {
            const auto max = _currPosBits > _prevLargestPos ? _currPosBits : _prevLargestPos;
            return max / 8;
        }

    private:
        size_t _prevLargestPos{};
        size_t _currPosBits{};
    };

    //helper type for default config
    using MeasureSize = BasicMeasureSize<DefaultConfig>;

}

#endif //BITSERY_ADAPTER_MEASURE_SIZE_H
