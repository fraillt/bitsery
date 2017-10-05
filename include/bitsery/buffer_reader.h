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

#include "details/sessions.h"
#include <algorithm>
#include <cstring>


namespace bitsery {

    template <typename TReader>
    class BitPackingReader;

    template<typename Config, typename InputAdapter>
    struct BasicReader {

        using TValue = typename InputAdapter::TValue;
        using TIterator = typename InputAdapter::TIterator;// used by session reader
        using ScratchType = typename details::SCRATCH_TYPE<TValue>::type;

        static_assert(details::IsDefined<TValue>::value, "Please define adapter traits or include from <bitsery/traits/...>");
        static_assert(details::IsDefined<ScratchType>::value, "Underlying adapter value type is not supported");

        explicit BasicReader(InputAdapter adapter)
                : _bufferContext{std::move(adapter)},
                  _session{*this, _bufferContext}
        {
        }

        BasicReader(const BasicReader &) = delete;

        BasicReader &operator=(const BasicReader &) = delete;

        BasicReader(BasicReader &&) noexcept = default;

        BasicReader &operator=(BasicReader &&) noexcept = default;

        ~BasicReader() noexcept = default;


        template<size_t SIZE, typename T>
        void readBytes(T &v) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            directRead(&v, 1);
        }

        template<size_t SIZE, typename T>
        void readBuffer(T *buf, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            directRead(buf, count);
        }

        template<typename T>
        void readBits(T &, size_t ) {
            static_assert(std::is_void<T>::value,
                          "Bit-packing is not enabled.\nEnable by call to `enableBitPacking`) or create Deserializer with bit packing enabled.");
        }

        void align() {
        }

        bool isCompletedSuccessfully() const {
            return _bufferContext.isCompletedSuccessfully() && !_session.hasActiveSessions();
        }

        ReaderError getError() const {
            auto err = _bufferContext.getError();
            if (_session.hasActiveSessions() && err == ReaderError::DATA_OVERFLOW)
                return ReaderError::NO_ERROR;
            return err;
        }

        void setError(ReaderError error) {
            return _bufferContext.setError(error);
        }

        void beginSession() {
            if (getError() != ReaderError::INVALID_DATA) {
                _session.begin();
            }
        }

        void endSession() {
            if (getError() != ReaderError::INVALID_DATA) {
                _session.end();
            }
        }

    private:
        friend class BitPackingReader<BasicReader<Config, InputAdapter>>;

        InputAdapter _bufferContext;
        typename std::conditional<Config::BufferSessionsEnabled,
                session::SessionsReader<BasicReader<Config, InputAdapter>>,
                session::DisabledSessionsReader<BasicReader<Config, InputAdapter>>>::type
                _session;

        template<typename T>
        void directRead(T *v, size_t count) {
            static_assert(!std::is_const<T>::value, "");
            _bufferContext.read(reinterpret_cast<TValue *>(v), sizeof(T) * count);
            //swap each byte if nessesarry
            _swapDataBits(v, count, std::integral_constant<bool,
                    Config::NetworkEndianness != details::getSystemEndianness()>{});
        }

        template<typename T>
        void _swapDataBits(T *v, size_t count, std::true_type) {
            std::for_each(v, std::next(v, count), [this](T &x) { x = details::swap(x); });
        }

        template<typename T>
        void _swapDataBits(T *v, size_t count, std::false_type) {
            //empty function because no swap is required
        }

    };

    template<typename TReader>
    struct BitPackingReader {

        using TValue = typename TReader::TValue;
        using ScratchType = typename details::SCRATCH_TYPE<TValue>::type;

        explicit BitPackingReader(TReader& reader):_reader{reader}
        {
            static_assert(std::is_unsigned<TValue>(), "Config::BufferValueType must be unsigned");
            static_assert(std::is_unsigned<ScratchType>(), "Config::BufferScrathType must be unsigned");
            static_assert(sizeof(TValue) * 2 == sizeof(ScratchType),
                          "ScratchType must be 2x bigger than value type");
            static_assert(sizeof(TValue) == 1, "currently only supported BufferValueType is 1 byte");
        }

        BitPackingReader(const BitPackingReader&) = delete;
        BitPackingReader& operator = (const BitPackingReader&) = delete;

        BitPackingReader(BitPackingReader&& ) noexcept = default;
        BitPackingReader& operator = (BitPackingReader&& ) noexcept = default;

        ~BitPackingReader() {
            align();
        }

        template<size_t SIZE, typename T>
        void readBytes(T &v) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            using UT = typename std::make_unsigned<T>::type;
            if (!m_scratchBits)
                _reader.template readBytes<SIZE,T>(v);
            else
                readBits(reinterpret_cast<UT &>(v), details::BITS_SIZE<T>::value);
        }

        template<size_t SIZE, typename T>
        void readBuffer(T *buf, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");

            if (!m_scratchBits) {
                _reader.template readBuffer<SIZE,T>(buf, count);
            } else {
                using UT = typename std::make_unsigned<T>::type;
                //todo improve implementation
                const auto end = buf + count;
                for (auto it = buf; it != end; ++it)
                    readBits(reinterpret_cast<UT &>(*it), details::BITS_SIZE<T>::value);
            }
        }


        template<typename T>
        void readBits(T &v, size_t bitsCount) {
            static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
            readBitsInternal(v, bitsCount);
        }

        void align() {
            if (m_scratchBits) {
                ScratchType tmp{};
                readBitsInternal(tmp, m_scratchBits);
                if (tmp)
                    setError(ReaderError::INVALID_DATA);
            }
        }

        bool isCompletedSuccessfully() const {
            return _reader.isCompletedSuccessfully();
        }

        ReaderError getError() const {
            return _reader.getError();
        }

        void setError(ReaderError error) {
            _reader.setError(error);
        }

        void beginSession() {
            align();
            _reader.beginSession();
        }

        void endSession() {
            align();
            _reader.endSession();
        }

    private:
        TReader& _reader;
        ScratchType m_scratch{};
        size_t m_scratchBits{};

        template<typename T>
        void readBitsInternal(T &v, size_t size) {
            auto bitsLeft = size;
            T res{};
            while (bitsLeft > 0) {
                auto bits = std::min(bitsLeft, details::BITS_SIZE<TValue>::value);
                if (m_scratchBits < bits) {
                    TValue tmp;
                    _reader.template readBytes<sizeof(TValue), TValue>(tmp);
                    m_scratch |= static_cast<ScratchType>(tmp) << m_scratchBits;
                    m_scratchBits += details::BITS_SIZE<TValue>::value;
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

    };
}

#endif //BITSERY_BUFFER_READER_H
