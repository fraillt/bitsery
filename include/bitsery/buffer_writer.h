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

#include "details/buffer_common.h"
#include "details/sessions.h"

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
                _bitsCount += (_sessionsBytesCount + 4) * 8;
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


    template <typename TWriter>
    class BitPackingWriter;

    template<typename Config, typename OutputAdapter>
    struct BasicWriter {
        using TValue = typename OutputAdapter::TValue;
        using ScratchType = typename details::SCRATCH_TYPE<TValue>::type;
        static_assert(details::IsDefined<TValue>::value, "Please define adapter traits or include from <bitsery/traits/...>");
        static_assert(details::IsDefined<ScratchType>::value, "Underlying adapter value type is not supported");

        explicit BasicWriter(OutputAdapter adapter)
                : _outputAdapter{std::move(adapter)}
        {
        }

        BasicWriter(const BasicWriter &) = delete;

        BasicWriter &operator=(const BasicWriter &) = delete;

        BasicWriter(BasicWriter &&) noexcept = default;

        BasicWriter &operator=(BasicWriter &&) noexcept = default;

        ~BasicWriter() noexcept = default;

        template<size_t SIZE, typename T>
        void writeBytes(const T &v) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            directWrite(&v, 1);

        }

        template<size_t SIZE, typename T>
        void writeBuffer(const T *buf, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            directWrite(buf, count);
        }

        template<typename T>
        void writeBits(const T &, size_t ) {
            static_assert(std::is_void<T>::value,
                          "Bit-packing is not enabled.\nEnable by call to `enableBitPacking`) or create Serializer with bit packing enabled.");
        }

        //to have the same interface as bitpackingwriter
        void align() {

        }

        void flush() {
            _session.flushSessions(*this);
        }

        size_t getWrittenBytesCount() const {
            return _outputAdapter.getWrittenBytesCount();
        }

        void beginSession() {
            _session.begin(*this);
        }

        void endSession() {
            _session.end(*this);
        }

    private:
        friend class BitPackingWriter<BasicWriter<Config, OutputAdapter>>;
        template<typename T>
        void directWrite(T &&v, size_t count) {
            _directWriteSwapTag(std::forward<T>(v), count, std::integral_constant<bool,
                    Config::NetworkEndianness != details::getSystemEndianness()>{});
        }

        template<typename T>
        void _directWriteSwapTag(const T *v, size_t count, std::true_type) {
            std::for_each(v, std::next(v, count), [this](const T &v) {
                const auto res = details::swap(v);
                _outputAdapter.write(reinterpret_cast<const TValue *>(&res), sizeof(T));
            });
        }

        template<typename T>
        void _directWriteSwapTag(const T *v, size_t count, std::false_type) {
            _outputAdapter.write(reinterpret_cast<const TValue *>(v), count * sizeof(T));
        }

        OutputAdapter _outputAdapter;
        typename std::conditional<Config::BufferSessionsEnabled,
                session::SessionsWriter<BasicWriter<Config, OutputAdapter>>,
                session::DisabledSessionsWriter<BasicWriter<Config, OutputAdapter>>>::type
                _session{};
    };

    template<typename TWriter>
    struct BitPackingWriter {
        using TValue = typename TWriter::TValue;
        using ScratchType = typename details::SCRATCH_TYPE<TValue>::type;

        explicit BitPackingWriter(TWriter &writer)
                : _writer{writer}
        {
        }

        BitPackingWriter(const BitPackingWriter&) = delete;
        BitPackingWriter& operator = (const BitPackingWriter&) = delete;

        BitPackingWriter(BitPackingWriter&& ) noexcept = default;
        BitPackingWriter& operator = (BitPackingWriter&& ) noexcept = default;

        ~BitPackingWriter() {
            align();
        }

        template<size_t SIZE, typename T>
        void writeBytes(const T &v) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");

            if (!_scratchBits) {
                _writer.template writeBytes<SIZE,T>(v);
            } else {
                using UT = typename std::make_unsigned<T>::type;
                writeBitsInternal(reinterpret_cast<const UT &>(v), details::BITS_SIZE<T>::value);
            }
        }

        template<size_t SIZE, typename T>
        void writeBuffer(const T *buf, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            if (!_scratchBits) {
                _writer.template writeBuffer<SIZE,T>(buf, count);
            } else {
                using UT = typename std::make_unsigned<T>::type;
                //todo improve implementation
                const auto end = buf + count;
                for (auto it = buf; it != end; ++it)
                    writeBitsInternal(reinterpret_cast<const UT &>(*it), details::BITS_SIZE<T>::value);
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
            writeBitsInternal(TValue{}, (details::BITS_SIZE<TValue>::value - _scratchBits) % 8);
        }

        void flush() {
            align();
            _writer._session.flushSessions(_writer);
        }

        size_t getWrittenBytesCount() const {
            return _writer.getWrittenBytesCount();
        }

        void beginSession() {
            align();
            _writer._session.begin(_writer);
        }

        void endSession() {
            align();
            _writer._session.end(_writer);
        }

    private:

        template<typename T>
        void writeBitsInternal(const T &v, size_t size) {
            constexpr size_t valueSize = details::BITS_SIZE<TValue>::value;
            auto value = v;
            auto bitsLeft = size;
            while (bitsLeft > 0) {
                auto bits = std::min(bitsLeft, valueSize);
                _scratch |= static_cast<ScratchType>( value ) << _scratchBits;
                _scratchBits += bits;
                if (_scratchBits >= valueSize) {
                    auto tmp = static_cast<TValue>(_scratch & _MASK);
                    _writer.template writeBytes<sizeof(TValue), TValue >(tmp);
                    _scratch >>= valueSize;
                    _scratchBits -= valueSize;

                    value >>= valueSize;
                }
                bitsLeft -= bits;
            }
        }

        //overload for TValue, for better performance
        void writeBitsInternal(const TValue &v, size_t size) {
            if (size > 0) {
                _scratch |= static_cast<ScratchType>( v ) << _scratchBits;
                _scratchBits += size;
                if (_scratchBits >= details::BITS_SIZE<TValue>::value) {
                    auto tmp = static_cast<TValue>(_scratch & _MASK);
                    _writer.template writeBytes<sizeof(TValue), TValue>(tmp);
                    _scratch >>= details::BITS_SIZE<TValue>::value;
                    _scratchBits -= details::BITS_SIZE<TValue>::value;
                }
            }
        }

        const TValue _MASK = std::numeric_limits<TValue>::max();
        ScratchType _scratch{};
        size_t _scratchBits{};
        TWriter& _writer;

    };
}

#endif //BITSERY_BUFFER_WRITER_H
