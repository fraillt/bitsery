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

#ifndef BITSERY_ADAPTER_READER_H
#define BITSERY_ADAPTER_READER_H

#include "details/sessions.h"
#include <algorithm>
#include <cstring>

namespace bitsery {

    template <typename TReader>
    class AdapterReaderBitPackingWrapper;

    template<typename InputAdapter, typename Config>
    struct AdapterReader {
        //this is required by deserializer
        static constexpr bool BitPackingEnabled = false;
        using TConfig = Config;
        using TValue = typename InputAdapter::TValue;

        static_assert(details::IsDefined<TValue>::value, "Please define adapter traits or include from <bitsery/traits/...>");

        using TIterator = typename InputAdapter::TIterator;// used by session reader

        explicit AdapterReader(InputAdapter&& adapter)
                : _inputAdapter{std::move(adapter)},
                  _session{*this, _inputAdapter}
        {
        }

        AdapterReader(const AdapterReader &) = delete;

        AdapterReader &operator=(const AdapterReader &) = delete;

        //todo add conditional noexcept
        AdapterReader(AdapterReader &&) = default;

        AdapterReader &operator=(AdapterReader &&) = default;

        ~AdapterReader() noexcept = default;

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
            return _inputAdapter.isCompletedSuccessfully() && !_session.hasActiveSessions();
        }

        ReaderError error() const {
            auto err = _inputAdapter.error();
            if (err == ReaderError::DataOverflow && _session.hasActiveSessions())
                return ReaderError::NoError;
            return err;
        }

        void setError(ReaderError error) {
            if (this->error() == ReaderError::NoError)
                _inputAdapter.setError(error);
        }

        void beginSession() {
            if (error() == ReaderError::NoError) {
                _session.begin();
            }
        }

        void endSession() {
            if (error() == ReaderError::NoError) {
                _session.end();
            }
        }

    private:
        friend class AdapterReaderBitPackingWrapper<AdapterReader<InputAdapter, Config>>;

        InputAdapter _inputAdapter;
        typename std::conditional<Config::BufferSessionsEnabled,
                session::SessionsReader<AdapterReader<InputAdapter, Config>>,
        session::DisabledSessionsReader<AdapterReader<InputAdapter, Config>>>::type
                _session;

        template<typename T>
        void directRead(T *v, size_t count) {
            static_assert(!std::is_const<T>::value, "");
            _inputAdapter.read(reinterpret_cast<TValue *>(v), sizeof(T) * count);
            //swap each byte if nessesarry
            _swapDataBits(v, count, std::integral_constant<bool,
                    Config::NetworkEndianness != details::getSystemEndianness()>{});
        }

        template<typename T>
        void _swapDataBits(T *v, size_t count, std::true_type) {
            std::for_each(v, std::next(v, count), [this](T &x) { x = details::swap(x); });
        }

        template<typename T>
        void _swapDataBits(T *, size_t , std::false_type) {
            //empty function because no swap is required
        }

    };

    template<typename TReader>
    class AdapterReaderBitPackingWrapper {
    public:
        //this is required by deserializer
        static constexpr bool BitPackingEnabled = true;
        using TConfig = typename TReader::TConfig;
        //make TValue unsigned for bitpacking
        using UnsignedValue = typename std::make_unsigned<typename TReader::TValue>::type;
        using ScratchType = typename details::ScratchType<UnsignedValue>::type;
        static_assert(details::IsDefined<ScratchType>::value, "Underlying adapter value type is not supported");

        explicit AdapterReaderBitPackingWrapper(TReader& reader):_reader{reader}
        {
        }

        AdapterReaderBitPackingWrapper(const AdapterReaderBitPackingWrapper&) = delete;
        AdapterReaderBitPackingWrapper& operator = (const AdapterReaderBitPackingWrapper&) = delete;

        AdapterReaderBitPackingWrapper(AdapterReaderBitPackingWrapper&& ) noexcept = default;
        AdapterReaderBitPackingWrapper& operator = (AdapterReaderBitPackingWrapper&& ) noexcept = default;

        ~AdapterReaderBitPackingWrapper() {
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
                readBits(reinterpret_cast<UT &>(v), details::BitsSize<T>::value);
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
                    readBits(reinterpret_cast<UT &>(*it), details::BitsSize<T>::value);
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
                    setError(ReaderError::InvalidData);
            }
        }

        bool isCompletedSuccessfully() const {
            return _reader.isCompletedSuccessfully();
        }

        ReaderError error() const {
            return _reader.error();
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
                auto bits = (std::min)(bitsLeft, details::BitsSize<UnsignedValue>::value);
                if (m_scratchBits < bits) {
                    UnsignedValue tmp;
                    _reader.template readBytes<sizeof(UnsignedValue), UnsignedValue>(tmp);
                    m_scratch |= static_cast<ScratchType>(tmp) << m_scratchBits;
                    m_scratchBits += details::BitsSize<UnsignedValue>::value;
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

#endif //BITSERY_ADAPTER_READER_H
