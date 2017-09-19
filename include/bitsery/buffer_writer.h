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



#ifndef BITSERY_BUFFER_WRITER_H
#define BITSERY_BUFFER_WRITER_H

#include "common.h"
#include <cassert>
#include <utility>

namespace bitsery {

    struct MeasureSize {

        template<size_t SIZE, typename T>
        void writeBytes(const T &) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            _bitsCount += details::BITS_SIZE<T>::value;
        }

        template<typename T>
        void writeBits(const T &, size_t bitsCount) {
            static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
            assert(bitsCount <= details::BITS_SIZE<T>::value);
            _bitsCount += bitsCount;
        }

        template<size_t SIZE, typename T>
        void writeBuffer(const T *, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            _bitsCount += details::BITS_SIZE<T>::value * count;
        }

        void align() {
            auto _scratch = (_bitsCount % 8);
            _bitsCount += (8 - _scratch) % 8;
        }

        void flush() {
            align();
            //flush sessions count
            if (_sessionsBytesCount > 0) {
                auto sessionsDataSizeBytesCount = (_sessionsBytesCount < 0x8000u ? 2 : 4);
                _bitsCount += (_sessionsBytesCount + sessionsDataSizeBytesCount) * 8;
                _sessionsBytesCount = 0;
            }
        }

        void beginSession() {

        }

        void endSession() {
            auto endPos = getWrittenBytesCount();
            details::writeSize(*this, endPos);
            auto sessionEndBytesCount = getWrittenBytesCount() - endPos;
            //remove written bytes, because we'll write them at the end
            _bitsCount -= sessionEndBytesCount * 8;
            _sessionsBytesCount += sessionEndBytesCount;
        }

        //get size in bytes
        size_t getWrittenBytesCount() const {
            return _bitsCount / 8;
        }

    private:
        size_t _bitsCount{};
        size_t _sessionsBytesCount{};
    };

    template<typename Config>
    struct BasicBufferWriter {
        using BufferType = typename Config::BufferType;
        using ValueType = typename details::BufferContainerTraits<BufferType>::TValue;
        using ScratchType = typename details::SCRATCH_TYPE<ValueType>::type;
        using BufferContext = details::WriteBufferContext<BufferType, details::BufferContainerTraits<BufferType>::isResizable>;

        explicit BasicBufferWriter(BufferType &buffer)
                : _bufferContext{buffer}
        {
            static_assert(std::is_unsigned<ValueType>(), "Config::BufferType value type must be unsigned");
            static_assert(sizeof(ValueType) * 2 == sizeof(ScratchType),
                          "ScratchType must be 2x bigger than value type");
            static_assert(sizeof(ValueType) == 1, "currently only supported BufferValueType is 1 byte");
        }

        BasicBufferWriter(const BasicBufferWriter &) = delete;

        BasicBufferWriter &operator=(const BasicBufferWriter &) = delete;

        BasicBufferWriter(BasicBufferWriter &&) noexcept = default;

        BasicBufferWriter &operator=(BasicBufferWriter &&) noexcept = default;

        ~BasicBufferWriter() noexcept = default;

        template<size_t SIZE, typename T>
        void writeBytes(const T &v) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");

            if (!_scratchBits) {
                directWrite(&v, 1);
            } else {
                using UT = typename std::make_unsigned<T>::type;
                writeBits(reinterpret_cast<const UT &>(v), details::BITS_SIZE<T>::value);
            }
        }

        template<size_t SIZE, typename T>
        void writeBuffer(const T *buf, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            if (!_scratchBits) {
                directWrite(buf, count);
            } else {
                using UT = typename std::make_unsigned<T>::type;
                //todo improve implementation
                const auto end = buf + count;
                for (auto it = buf; it != end; ++it)
                    writeBits(reinterpret_cast<const UT &>(*it), details::BITS_SIZE<T>::value);
            }
        }

        template<typename T>
        void writeBits(const T &v, size_t bitsCount) {
            static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
            assert(0 < bitsCount && bitsCount <= details::BITS_SIZE<T>::value);
            assert(v <= (bitsCount < 64
                         ? (1ULL << bitsCount) - 1
                         : (1ULL << (bitsCount-1)) + ((1ULL << (bitsCount-1)) -1)));
            writeBitsInternal(v, bitsCount);
        }

        void align() {
            writeBitsInternal(ValueType{}, (details::BITS_SIZE<ValueType>::value - _scratchBits) % 8);
        }

        void flush() {
            align();
            _session.flushSessions(*this);
        }

        BufferRange<typename details::BufferContainerTraits<BufferType>::TIterator> getWrittenRange() const {
            return _bufferContext.getWrittenRange();
        }

        void beginSession() {
            align();
            _session.begin();
        }

        void endSession() {
            align();
            auto range = _bufferContext.getWrittenRange();
            _session.end(static_cast<size_t>(std::distance(range.begin(), range.end())));
        }

    private:

        template<typename T>
        void directWrite(T &&v, size_t count) {
            _directWriteSwapTag(std::forward<T>(v), count, std::integral_constant<bool,
                    Config::NetworkEndianness != details::getSystemEndianness()>{});
        }

        template<typename T>
        void _directWriteSwapTag(const T *v, size_t count, std::true_type) {
            std::for_each(v, std::next(v, count), [this](const T &v) {
                const auto res = details::swap(v);
                _bufferContext.write(reinterpret_cast<const ValueType *>(&res), sizeof(T));
            });
        }

        template<typename T>
        void _directWriteSwapTag(const T *v, size_t count, std::false_type) {
            _bufferContext.write(reinterpret_cast<const ValueType *>(v), count * sizeof(T));
        }

        template<typename T>
        void writeBitsInternal(const T &v, size_t size) {
            constexpr size_t valueSize = details::BITS_SIZE<ValueType>::value;
            auto value = v;
            auto bitsLeft = size;
            while (bitsLeft > 0) {
                auto bits = std::min(bitsLeft, valueSize);
                _scratch |= static_cast<ScratchType>( value ) << _scratchBits;
                _scratchBits += bits;
                if (_scratchBits >= valueSize) {
                    auto tmp = static_cast<ValueType>(_scratch & _MASK);
                    directWrite(&tmp, 1);
                    _scratch >>= valueSize;
                    _scratchBits -= valueSize;

                    value >>= valueSize;
                }
                bitsLeft -= bits;
            }
        }

        //overload for ValueType, for better performance
        void writeBitsInternal(const ValueType &v, size_t size) {
            if (size > 0) {
                _scratch |= static_cast<ScratchType>( v ) << _scratchBits;
                _scratchBits += size;
                if (_scratchBits >= details::BITS_SIZE<ValueType>::value) {
                    auto tmp = static_cast<ValueType>(_scratch & _MASK);
                    directWrite(&tmp, 1);
                    _scratch >>= details::BITS_SIZE<ValueType>::value;
                    _scratchBits -= details::BITS_SIZE<ValueType>::value;
                }
            }
        }


        const ValueType _MASK = std::numeric_limits<ValueType>::max();
        BufferContext _bufferContext;
        ScratchType _scratch{};
        size_t _scratchBits{};
        details::BufferSessionsWriter _session{};
    };

    //helper type
    using BufferWriter = BasicBufferWriter<DefaultConfig>;
}

#endif //BITSERY_BUFFER_WRITER_H
