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



#ifndef BITSERY_BUFFER_READER_H
#define BITSERY_BUFFER_READER_H

#include "common.h"
#include <algorithm>

namespace bitsery {

    template<typename Config>
    struct BasicBufferReader {
        using BufferType = typename Config::BufferType;
        using ValueType = typename BufferType::value_type;
        using ScratchType = typename details::SCRATCH_TYPE<ValueType>::type;

        BasicBufferReader(const ValueType *data, size_t size) : _pos{data}, _end{data + size} {
            static_assert(std::is_unsigned<ValueType>(), "Config::BufferValueType must be unsigned");
            static_assert(std::is_unsigned<ScratchType>(), "Config::BufferScrathType must be unsigned");
            static_assert(sizeof(ValueType) * 2 == sizeof(ScratchType),
                          "ScratchType must be 2x bigger than value type");
            static_assert(sizeof(ValueType) == 1, "currently only supported BufferValueType is 1 byte");
        }

        BasicBufferReader(typename BufferType::iterator begin, typename BufferType::iterator end)
                : BasicBufferReader(std::addressof(*begin), std::distance(begin, end)) {}

        explicit BasicBufferReader(const BufferType &buf) : BasicBufferReader(buf.data(), buf.size()) {
        }

        BasicBufferReader(const BasicBufferReader &) = delete;

        BasicBufferReader &operator=(const BasicBufferReader &) = delete;

        BasicBufferReader(BasicBufferReader &&) noexcept = default;

        BasicBufferReader &operator=(BasicBufferReader &&) noexcept = default;

        ~BasicBufferReader() noexcept = default;


        template<size_t SIZE, typename T>
        bool readBytes(T &v) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            using UT = typename std::make_unsigned<T>::type;
            return !m_scratch
                   ? directRead(&v, 1)
                   : readBits(reinterpret_cast<UT &>(v), details::BITS_SIZE<T>);
        }

        template<size_t SIZE, typename T>
        bool readBuffer(T *buf, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");

            if (!m_scratchBits)
                return directRead(buf, count);

            using UT = typename std::make_unsigned<T>::type;
            //todo improve implementation
            const auto end = buf + count;
            for (auto it = buf; it != end; ++it) {
                if (!readBits(reinterpret_cast<UT &>(*it), details::BITS_SIZE<T>))
                    return false;
            }
            return true;
        }


        template<typename T>
        bool readBits(T &v, size_t bitsCount) {
            static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");

            const auto bytesRequired = bitsCount > m_scratchBits
                                       ? ((bitsCount - 1 - m_scratchBits) >> 3) + 1u
                                       : 0u;
            if (static_cast<size_t>(std::distance(_pos, _end)) < bytesRequired)
                return false;
            readBitsInternal(v, bitsCount);
            return true;
        }

        bool align() {
            if (m_scratchBits) {
                ScratchType tmp{};
                readBitsInternal(tmp, m_scratchBits);
                return tmp == 0;
            }
            return true;
        }

        bool isCompleted() const {
            return _pos == _end;
        }

    private:
        const ValueType *_pos;
        const ValueType *_end;

        template<typename T>
        bool directRead(T *v, size_t count) {
            static_assert(!std::is_const<T>::value, "");
            const auto bytesCount = sizeof(T) * count;
            if (static_cast<size_t>(std::distance(_pos, _end)) < bytesCount)
                return false;
            //read from buffer, to data ptr,
            std::copy_n(_pos, bytesCount, reinterpret_cast<ValueType *>(v));
            std::advance(_pos, bytesCount);
            //swap each byte if nessesarry
            _swapDataBits(v, count, std::integral_constant<bool,
                    Config::NetworkEndianness != details::getSystemEndianness()>{});
            return true;
        }

        template<typename T>
        void _swapDataBits(T *v, size_t count, std::true_type) {
            std::for_each(v, std::next(v, count), [this](T &v) { v = details::swap(v); });
        }

        template<typename T>
        void _swapDataBits(T *v, size_t count, std::false_type) {
            //empty function because no swap is required
        }

        template<typename T>
        void readBitsInternal(T &v, size_t size) {
            auto bitsLeft = size;
            T res{};
            while (bitsLeft > 0) {
                auto bits = std::min(bitsLeft, details::BITS_SIZE<ValueType>);
                if (m_scratchBits < bits) {
                    ValueType tmp;
                    directRead(&tmp, 1);
                    m_scratch |= static_cast<ScratchType>(tmp) << m_scratchBits;
                    m_scratchBits += details::BITS_SIZE<ValueType>;
                }
                auto shiftedRes =
                        static_cast<T>(m_scratch & ((static_cast<ScratchType>(1) << bits) - 1)) << (size - bitsLeft);
                res |= shiftedRes;
                m_scratch >>= bits;
                m_scratchBits -= bits;
                bitsLeft -= bits;
            }
            v = res;
        }

        ScratchType m_scratch{};
        size_t m_scratchBits{};                                    ///< Number of bits currently in the scratch buffer. If the user wants to read more bits than this, we have to go fetch another dword from memory.

    };
    //helper type
    using BufferReader = BasicBufferReader<DefaultConfig>;
}

#endif //BITSERY_BUFFER_READER_H
