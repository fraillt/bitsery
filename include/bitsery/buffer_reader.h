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
#include <cstring>

namespace bitsery {

    template<typename Config>
    struct BasicBufferReader {

        using BufferType = typename Config::BufferType;
        using ValueType = typename BufferType::value_type;
        using BufferIteratorType = typename BufferType::iterator;
        using ScratchType = typename details::SCRATCH_TYPE<ValueType>::type;

        BasicBufferReader(ValueType* begin, ValueType* end)
                :_pos{begin},
                 _end{end},
                 _session{*this, _pos, _end}
        {
            static_assert(std::is_unsigned<ValueType>(), "Config::BufferValueType must be unsigned");
            static_assert(std::is_unsigned<ScratchType>(), "Config::BufferScrathType must be unsigned");
            static_assert(sizeof(ValueType) * 2 == sizeof(ScratchType),
                          "ScratchType must be 2x bigger than value type");
            static_assert(sizeof(ValueType) == 1, "currently only supported BufferValueType is 1 byte");
        }

        BasicBufferReader(BufferRange<BufferIteratorType> range)
                :BasicBufferReader(std::addressof(*range.begin()), std::addressof(*range.end())) {
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
                readBits(reinterpret_cast<UT &>(v), details::BITS_SIZE<T>);
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
                    readBits(reinterpret_cast<UT &>(*it), details::BITS_SIZE<T>);
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
            return _pos == _end && !_session.hasActiveSessions();
        }

        BufferReaderError getError() const {
            auto res = std::distance(_end, _pos);
            if (res > 0) {
                auto err = static_cast<BufferReaderError>(res);
                if (_session.hasActiveSessions() && err == BufferReaderError::BUFFER_OVERFLOW)
                    return BufferReaderError::NO_ERROR;
                return err;
            }
            return BufferReaderError::NO_ERROR;
        }

        void setError(BufferReaderError error) {
            _end = _pos;
            //to avoid creating temporary for error state, mark an error by passing _pos after the _end
            std::advance(_pos, static_cast<size_t>(error));
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
        ValueType* _pos;
        ValueType* _end;
        details::BufferSessionsReader<BasicBufferReader<Config>, ValueType*> _session;
        ScratchType m_scratch{};
        size_t m_scratchBits{};                                    ///< Number of bits currently in the scratch buffer. If the user wants to read more bits than this, we have to go fetch another dword from memory.

        template<typename T>
        void directRead(T *v, size_t count) {
            static_assert(!std::is_const<T>::value, "");
            const auto bytesCount = sizeof(T) * count;

            if (std::distance(_pos, _end) >= static_cast<typename BufferType::difference_type>(bytesCount)) {

                std::memcpy(reinterpret_cast<ValueType *>(v), _pos, bytesCount);
                _pos += bytesCount;

                //swap each byte if nessesarry
                _swapDataBits(v, count, std::integral_constant<bool,
                        Config::NetworkEndianness != details::getSystemEndianness()>{});
            } else {
                //set everything to zeros
                std::memset(v, 0, bytesCount);

                if (getError() == BufferReaderError::NO_ERROR)
                    setError(BufferReaderError::BUFFER_OVERFLOW);
            }
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

    };
    //helper type
    using BufferReader = BasicBufferReader<DefaultConfig>;
}

#endif //BITSERY_BUFFER_READER_H
