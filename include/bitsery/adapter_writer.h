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

#ifndef BITSERY_ADAPTER_WRITER_H
#define BITSERY_ADAPTER_WRITER_H

#include "details/adapter_common.h"

#include <cassert>
#include <utility>

namespace bitsery {


    template <typename Config, typename Context=void>
    class BasicMeasureSize {
        struct NoExternalContext{};
    public:

        static constexpr bool BitPackingEnabled = true;
        using TConfig = Config;
        using InternalContext = typename Config::InternalContext;
        using ExternalContext = typename std::conditional<std::is_void<Context>::value, NoExternalContext, Context>::type;
        using TValue = void;

        static_assert(details::IsSpecializationOf<InternalContext, std::tuple>::value,
                      "Config::InternalContext must be std::tuple");

        // take ownership of adapter
        template <typename T=Context, typename std::enable_if<std::is_void<T>::value>::type* = nullptr>
        explicit BasicMeasureSize()
        :_internalContext{},
        _externalContext{}
        {
        }

        // get context by reference, do not take ownership of it
        template <typename T=Context, typename std::enable_if<!std::is_void<T>::value>::type* = nullptr>
        explicit BasicMeasureSize(ExternalContext& ctx)
        :_internalContext{},
        _externalContext{ctx}
        {
        }


        template<size_t SIZE, typename T>
        void writeBytes(const T &) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            _currPosBits += details::BitsSize<T>::value;
        }

        template<size_t SIZE, typename T>
        void writeBuffer(const T *, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            _currPosBits += details::BitsSize<T>::value * count;
        }

        template<typename T>
        void writeBits(const T &, size_t bitsCount) {
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

        ExternalContext& externalContext() {
            return _externalContext;
        }

        InternalContext& internalContext() {
            return _internalContext;
        }

    private:
        InternalContext _internalContext;
        typename std::conditional<std::is_void<Context>::value,
            ExternalContext,ExternalContext&>::type _externalContext;

    private:
        size_t _prevLargestPos{};
        size_t _currPosBits{};
    };

    //helper type for default config
    using MeasureSize = BasicMeasureSize<DefaultConfig>;

    template <typename TWriter>
    class AdapterWriterBitPackingWrapper;

    template<typename OutputAdapter, typename Config, typename Context=void>
    struct AdapterWriter: public details::AdapterAndContext<OutputAdapter, Config, Context> {

        using details::AdapterAndContext<OutputAdapter, Config, Context>::AdapterAndContext;

        static constexpr bool BitPackingEnabled = false;
        using typename details::AdapterAndContext<OutputAdapter, Config, Context>::TValue;

        ~AdapterWriter() {
            flush();
        }

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

        void currentWritePos(size_t pos) {
            this->_adapter.currentWritePos(pos);
        }

        size_t currentWritePos() const {
            return this->_adapter.currentWritePos();
        }

        void flush() {
            this->_adapter.flush();
        }

        size_t writtenBytesCount() const {
            return this->_adapter.writtenBytesCount();
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
                this->_adapter.write(reinterpret_cast<const TValue *>(&res), sizeof(T));
            });
        }

        template<typename T>
        void _directWriteSwapTag(const T *v, size_t count, std::false_type) {
            this->_adapter.write(reinterpret_cast<const TValue *>(v), count * sizeof(T));
        }

    };

    template<typename TWriter>
    class AdapterWriterBitPackingWrapper: public details::AdapterAndContextWrapper<TWriter> {
    public:
        using details::AdapterAndContextWrapper<TWriter>::AdapterAndContextWrapper;

        static constexpr bool BitPackingEnabled = true;

        ~AdapterWriterBitPackingWrapper() {
            align();
        }

        template<size_t SIZE, typename T>
        void writeBytes(const T &v) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");

            if (!_scratchBits) {
                this->_wrapped.template writeBytes<SIZE,T>(v);
            } else {
                using UT = typename std::make_unsigned<T>::type;
                writeBitsInternal(reinterpret_cast<const UT &>(v), details::BitsSize<T>::value);
            }
        }

        template<size_t SIZE, typename T>
        void writeBuffer(const T *buf, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            if (!_scratchBits) {
                this->_wrapped.template writeBuffer<SIZE,T>(buf, count);
            } else {
                using UT = typename std::make_unsigned<T>::type;
                //todo improve implementation
                const auto end = buf + count;
                for (auto it = buf; it != end; ++it)
                    writeBitsInternal(reinterpret_cast<const UT &>(*it), details::BitsSize<T>::value);
            }
        }

        template<typename T>
        void writeBits(const T &v, size_t bitsCount) {
            static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
            assert(0 < bitsCount && bitsCount <= details::BitsSize<T>::value);
            assert(v <= (bitsCount < 64
                         ? (1ULL << bitsCount) - 1
                         : (1ULL << (bitsCount-1)) + ((1ULL << (bitsCount-1)) -1)));
            writeBitsInternal(v, bitsCount);
        }

        void align() {
            writeBitsInternal(UnsignedType{}, (details::BitsSize<UnsignedType>::value - _scratchBits) % 8);
        }

        void currentWritePos(size_t pos) {
            align();
            this->_wrapped.currentWritePos(pos);
        }

        size_t currentWritePos() const {
            return this->_wrapped.currentWritePos();
        }

        void flush() {
            align();
            this->_wrapped.flush();
        }

        size_t writtenBytesCount() const {
            return this->_wrapped.writtenBytesCount();
        }

    private:

        using UnsignedType = typename std::make_unsigned<typename TWriter::TValue>::type;
        using ScratchType = typename details::ScratchType<UnsignedType>::type;
        static_assert(details::IsDefined<ScratchType>::value, "Underlying adapter value type is not supported");


        template<typename T>
        void writeBitsInternal(const T &v, size_t size) {
            constexpr size_t valueSize = details::BitsSize<UnsignedType>::value;
            auto value = v;
            auto bitsLeft = size;
            while (bitsLeft > 0) {
                auto bits = (std::min)(bitsLeft, valueSize);
                _scratch |= static_cast<ScratchType>( value ) << _scratchBits;
                _scratchBits += bits;
                if (_scratchBits >= valueSize) {
                    auto tmp = static_cast<UnsignedType>(_scratch & _MASK);
                    this->_wrapped.template writeBytes<sizeof(UnsignedType), UnsignedType >(tmp);
                    _scratch >>= valueSize;
                    _scratchBits -= valueSize;

                    value >>= valueSize;
                }
                bitsLeft -= bits;
            }
        }

        //overload for TValue, for better performance
        void writeBitsInternal(const UnsignedType &v, size_t size) {
            if (size > 0) {
                _scratch |= static_cast<ScratchType>( v ) << _scratchBits;
                _scratchBits += size;
                if (_scratchBits >= details::BitsSize<UnsignedType>::value) {
                    auto tmp = static_cast<UnsignedType>(_scratch & _MASK);
                    this->_wrapped.template writeBytes<sizeof(UnsignedType), UnsignedType>(tmp);
                    _scratch >>= details::BitsSize<UnsignedType>::value;
                    _scratchBits -= details::BitsSize<UnsignedType>::value;
                }
            }
        }

        const UnsignedType _MASK = (std::numeric_limits<UnsignedType>::max)();
        ScratchType _scratch{};
        size_t _scratchBits{};
    };

    namespace details {
        // used in "making friends" with non-wrapped serializer type
        template <typename TWriter>
        struct GetNonWrappedAdapterWriter {
            using Writer = TWriter;
        };

        template <typename TWrapped>
        struct GetNonWrappedAdapterWriter<AdapterWriterBitPackingWrapper<TWrapped>> {
            using Writer = TWrapped;
        };
    }
}

#endif //BITSERY_ADAPTER_WRITER_H
