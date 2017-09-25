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

#include "details/buffer_common.h"
#include <algorithm>
#include <cstring>

namespace bitsery {

    template<typename Config>
    struct BasicBufferReader {

        using BufferType = typename Config::BufferType;
        using ValueType = typename details::ContainerTraits<BufferType>::TValue;
        using BufferIteratorType = typename details::BufferContainerTraits<BufferType>::TIterator;
        using ScratchType = typename details::SCRATCH_TYPE<ValueType>::type;

        BasicBufferReader(BufferIteratorType data, BufferIteratorType end)
                : _bufferContext{data, end},
                  _session{*this, _bufferContext}
        {
            static_assert(std::is_unsigned<ValueType>(), "Config::BufferValueType must be unsigned");
            static_assert(std::is_unsigned<ScratchType>(), "Config::BufferScrathType must be unsigned");
            static_assert(sizeof(ValueType) * 2 == sizeof(ScratchType),
                          "ScratchType must be 2x bigger than value type");
            static_assert(sizeof(ValueType) == 1, "currently only supported BufferValueType is 1 byte");
        }

        BasicBufferReader(BufferRange<BufferIteratorType> range)
                :BasicBufferReader(range.begin(), range.end()) {
            static_assert(std::is_same<
                                  typename std::iterator_traits<BufferIteratorType>::iterator_category,
                                  std::random_access_iterator_tag>::value,
            "BufferReader only accepts random access iterators");
        }

        BasicBufferReader(const BasicBufferReader &) = delete;

        BasicBufferReader &operator=(const BasicBufferReader &) = delete;

        BasicBufferReader(BasicBufferReader &&) noexcept = default;

        BasicBufferReader &operator=(BasicBufferReader &&) noexcept = default;

        ~BasicBufferReader() noexcept = default;


        template<size_t SIZE, typename T>
        void readBytes(T &v) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");
            using UT = typename std::make_unsigned<T>::type;
            if (!m_scratchBits)
                directRead(&v, 1);
            else
                readBits(reinterpret_cast<UT &>(v), details::BITS_SIZE<T>::value);
        }

        template<size_t SIZE, typename T>
        void readBuffer(T *buf, size_t count) {
            static_assert(std::is_integral<T>(), "");
            static_assert(sizeof(T) == SIZE, "");

            if (!m_scratchBits) {
                directRead(buf, count);
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
                    setError(BufferReaderError::INVALID_BUFFER_DATA);
            }
        }

        bool isCompletedSuccessfully() const {
            return _bufferContext.isCompletedSuccessfully() && !_session.hasActiveSessions();
        }

        BufferReaderError getError() const {
            auto err = _bufferContext.getError();
            if (_session.hasActiveSessions() && err == BufferReaderError::BUFFER_OVERFLOW)
                return BufferReaderError::NO_ERROR;
            return err;
        }

        void setError(BufferReaderError error) {
            return _bufferContext.setError(error);
        }

        void beginSession() {
            align();
            if (getError() != BufferReaderError::INVALID_BUFFER_DATA) {
                _session.begin();
            }
        }

        void endSession() {
            align();
            if (getError() != BufferReaderError::INVALID_BUFFER_DATA) {
                _session.end();
            }
        }

    private:
        details::ReadBufferContext<BufferType> _bufferContext;
        ScratchType m_scratch{};
        size_t m_scratchBits{};
        typename std::conditional<Config::BufferSessionsEnabled,
                details::BufferSessionsReader<BasicBufferReader<Config>, details::ReadBufferContext<BufferType>>,
                details::DisabledBufferSessionsReader<Config>>::type
                _session;

        template<typename T>
        void directRead(T *v, size_t count) {
            static_assert(!std::is_const<T>::value, "");
            _bufferContext.read(reinterpret_cast<ValueType *>(v), sizeof(T) * count);
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

        template<typename T>
        void readBitsInternal(T &v, size_t size) {
            auto bitsLeft = size;
            T res{};
            while (bitsLeft > 0) {
                auto bits = std::min(bitsLeft, details::BITS_SIZE<ValueType>::value);
                if (m_scratchBits < bits) {
                    ValueType tmp;
                    directRead(&tmp, 1);
                    m_scratch |= static_cast<ScratchType>(tmp) << m_scratchBits;
                    m_scratchBits += details::BITS_SIZE<ValueType>::value;
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
    //helper type
    using BufferReader = BasicBufferReader<DefaultConfig>;
}

#endif //BITSERY_BUFFER_READER_H
