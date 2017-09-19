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

#ifndef BITSERY_DETAILS_BUFFER_COMMON_H
#define BITSERY_DETAILS_BUFFER_COMMON_H

#include <algorithm>
#include <utility>
#include <cassert>
#include <vector>
#include <stack>
#include <cstring>
#include "both_common.h"
#include "traits.h"

namespace bitsery {

/*
 * endianess
 */
    enum class EndiannessType {
        LittleEndian,
        BigEndian
    };

    template<typename I>
    struct BufferRange : std::pair<I, I> {
        using std::pair<I, I>::pair;
        I begin() const { return this->first; }
        I end() const { return this->second; }
    };

    enum class BufferReaderError {
        NO_ERROR,
        BUFFER_OVERFLOW,
        INVALID_BUFFER_DATA
    };

    namespace details {

        template<typename T>
        struct BITS_SIZE:public std::integral_constant<size_t, sizeof(T) << 3> {

        };

        //add swap functions to class, to avoid compilation warning about unused functions
        struct swapImpl {
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
            return swapImpl::exec(static_cast<UT>(value));
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
        struct SCRATCH_TYPE {
        };

        template<>
        struct SCRATCH_TYPE<uint8_t> {
            using type = uint16_t;
        };

        template<>
        struct SCRATCH_TYPE<uint16_t> {
            using type = uint32_t;
        };

        template<>
        struct SCRATCH_TYPE<uint32_t> {
            using type = uint64_t;
        };

//        struct BufferSessionInfo {
//            size_t depth;
//            size_t offset;
//        };

        class BufferSessionsWriter {
        public:
            void begin() {
                //write position
                _sessionIndex.push(_sessions.size());
                _sessions.emplace_back(0);
            }
            void end(size_t pos) {
                assert(!_sessionIndex.empty());
                //change position to session end
                auto sessionIt = std::next(std::begin(_sessions), _sessionIndex.top());
                _sessionIndex.pop();
                *sessionIt = pos;
            }
            template <typename TWriter>
            void flushSessions(TWriter& writer) {
                if (_sessions.size()) {
                    assert(_sessionIndex.empty());
                    auto range = writer.getWrittenRange();
                    auto dataSize = static_cast<size_t>(std::distance(range.begin(), range.end()));
                    for(auto& s:_sessions) {
                        details::writeSize(writer, s);
                    }
                    _sessions.clear();

                    range = writer.getWrittenRange();
                    auto totalSize = static_cast<size_t>(std::distance(range.begin(), range.end()));
                    //write offset where actual data ends
                    auto sessionsOffset = totalSize - dataSize + 2;//2 bytes for offset data
                    if (sessionsOffset < 0x8000u) {
                        writer.template writeBytes<2>(static_cast<uint16_t>(sessionsOffset));
                    } else {
                        //size doesnt fit in 2 bytes, write 4 bytes instead
                        sessionsOffset+=2;
                        uint16_t low = static_cast<uint16_t>(sessionsOffset);
                        //mark most significant bit, that size is 4 bytes
                        uint16_t high = static_cast<uint16_t>(0x8000u | (sessionsOffset >> 16));
                        writer.template writeBytes<2>(low);
                        writer.template writeBytes<2>(high);
                    }
                }
            }
        private:
            std::vector<size_t> _sessions{};
            std::stack<size_t> _sessionIndex;
        };

        template <typename TReader, typename TIterator>
        struct BufferSessionsReader {
            TIterator _bufBegin;
            BufferSessionsReader(TReader& r, TIterator& begin, TIterator& end)
                    :_reader{r},
                     _pos{begin},
                     _end{end}
            {
                _bufBegin = begin;
            }
            void begin() {
                if (_sessions.empty())
                    initializeSessions();
                //save end position for current session
                _sessionsStack.push(_end);
                if (_nextSessionIt != std::end(_sessions)) {
                    if (std::distance(_pos, _end) > 0) {
                        //set end position for new session
                        auto newEnd = std::next(_bufBegin, *_nextSessionIt);
                        if (std::distance(newEnd, _end) < 0)
                        {
                            //new session cannot end further than current end
                            _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                            return;
                        }
                        _end = newEnd;
                        ++_nextSessionIt;
                    }
                    //if we reached the end, means that there is no more data to read, hence there is no more sessions to advance to
                } else {
                    //there is no data to read anymore
                    //pos == end or buffer overflow while session is active
                    if (!(_pos == _end || _reader.getError() == BufferReaderError::NO_ERROR)) {
                        _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                    }
                }
            }

            void end() {
                if (!_sessionsStack.empty()) {
                    //move position to the end of session
                    //can additionaly be checked for session data versioning
                    //_pos == _end : same versions
                    //distance(_pos,_end) > 0: reading newer version
                    //getError() == BUFFER_OVERFLOW: reading older version
                    auto dist = std::distance(_pos, _end);
                    if (dist > 0) {
                        //newer version might have some inner sessions, try to find the one after current ends
                        auto currPos = static_cast<size_t>(std::distance(_bufBegin, _end));
                        for (; _nextSessionIt != std::end(_sessions); ++_nextSessionIt) {
                            if (*_nextSessionIt > currPos)
                                break;
                        }
                    }
                    _pos = _end;
                    //restore end position
                    _end = _sessionsStack.top();
                    _sessionsStack.pop();
                }
            }

            bool hasActiveSessions() const {
                return _sessionsStack.size() > 0;
            }

        private:
            TReader& _reader;
            TIterator& _pos;
            TIterator& _end;

            std::vector<size_t> _sessions{};
            std::vector<size_t>::iterator _nextSessionIt{};
            std::stack<TIterator> _sessionsStack{};

            void initializeSessions() {
                //save current position
                auto currPos = _pos;
                auto bufferSizeLeft = std::distance(_pos, _end);
                //read size
                if (bufferSizeLeft < 2) {
                    _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                    return;
                }
                auto endSessionsSizesIt = std::next(_end, -2);
                _pos = endSessionsSizesIt;
                size_t sessionsOffset{};
                uint16_t high;
                _reader.template readBytes<2>(high);


                if (high >= 0x8000u) {
                    if (bufferSizeLeft < 4) {
                        _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                        return;
                    }
                    endSessionsSizesIt = std::next(endSessionsSizesIt, -2);
                    _pos = endSessionsSizesIt;
                    uint16_t low;
                    _reader.template readBytes<2>(low);
                    //mask out last bit
                    high &= 0x7FFFu;
                    sessionsOffset = static_cast<size_t>((high << 16) | low);

                } else
                    sessionsOffset = high;
                if (static_cast<size_t>(bufferSizeLeft) < sessionsOffset) {
                    _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                    return;
                }
                //we can initialy resizes to this value, and we'll shrink it after reading
                //read session sizes
                auto sessionsIt = std::back_inserter(_sessions);
                _pos = std::next(_end, -sessionsOffset);
                while (std::distance(_pos, endSessionsSizesIt) > 0) {
                    size_t size;
                    details::readSize(_reader, size);
                    *sessionsIt++ = size;
                }
                _sessions.shrink_to_fit();
                //set iterators to data
                _pos = currPos;
                _end = std::next(_end, -sessionsOffset);
                _nextSessionIt = std::begin(_sessions);//set before first session;
            }
        };

        template<typename Buffer, bool isResizable>
        class WriteBufferContext {

        };

        template<typename Buffer>
        class WriteBufferContext<Buffer, false>{
        public:
            using ValueType = typename BufferContainerTraits<Buffer>::TValue;
            using IteratorType = typename BufferContainerTraits<Buffer>::TIterator;
            using DifferenceType = typename BufferContainerTraits<Buffer>::TDifference;

            explicit WriteBufferContext(Buffer &buffer)
                    : _buffer{buffer},
                      _outIt{std::addressof(*std::begin(buffer))},
                      _end{std::addressof(*std::end(buffer))}
            {
            }

            void write(const ValueType *data, size_t size) {
                assert(std::distance(_outIt, _end) >= static_cast<DifferenceType>(size));
                memcpy(_outIt, data, size);
                _outIt += size;
            }

            BufferRange<IteratorType> getWrittenRange() const {
                auto begin = std::begin(_buffer);
                return BufferRange<IteratorType>{begin, std::next(begin, _outIt - std::addressof(*begin))};
            }

        private:
            Buffer &_buffer;
            ValueType* _outIt;
            ValueType* _end;
        };

        template<typename Buffer>
        class WriteBufferContext<Buffer, true> {
        public:
            using TValue = typename BufferContainerTraits<Buffer>::TValue;
            using TIterator = typename BufferContainerTraits<Buffer>::TIterator;
            using TDifference = typename BufferContainerTraits<Buffer>::TDifference;

            explicit WriteBufferContext(Buffer &buffer)
                    : _buffer{buffer}
            {
                getIterators(0);
            }

            void write(const TValue *data, size_t size) {
                if ((_end - _outIt) >= static_cast< TDifference >(size)) {
                    std::memcpy(_outIt, data, size);
                    _outIt += size;
                } else {
                    //get current position before invalidating iterators
                    auto pos = std::distance(std::addressof(*std::begin(_buffer)), _outIt);
                    //increase container size
                    BufferContainerTraits<Buffer>::increaseBufferSize(_buffer);
                    //restore iterators
                    getIterators(pos);
                    write(data, size);
                }
            }

            BufferRange<TIterator> getWrittenRange() const {
                auto begin = std::begin(_buffer);
                return BufferRange<TIterator>{begin, std::next(begin, _outIt - std::addressof(*begin))};
            }

        private:

            void getIterators(TDifference  writePos) {
                _end = std::addressof(*std::end(_buffer));
                _outIt = std::addressof(*std::next(std::begin(_buffer), writePos));
            }
            Buffer &_buffer;
            TValue* _outIt;
            TValue* _end;
        };

    }
}


#endif //BITSERY_DETAILS_BUFFER_COMMON_H
