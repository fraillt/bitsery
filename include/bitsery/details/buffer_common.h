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

#include "../common.h"

namespace bitsery {

    template<typename I>
    struct BufferRange : std::pair<I, I> {
        using std::pair<I, I>::pair;
        I begin() const { return this->first; }
        I end() const { return this->second; }
    };

    namespace details {

        template<typename T>
        struct BITS_SIZE:public std::integral_constant<size_t, sizeof(T) << 3> {

        };

        //add swap functions to class, to avoid compilation warning about unused functions
        struct SwapImpl {
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
            return SwapImpl::exec(static_cast<UT>(value));
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

        template <typename Config>
        struct DisabledBufferSessionsWriter {
            template <typename TWriter>
            void begin(TWriter& ) {
                static_assert(Config::BufferSessionsEnabled, "Buffer sessions is disabled, enable it via configuration");
            }
            template <typename TWriter>
            void end(TWriter& ) {
                static_assert(Config::BufferSessionsEnabled, "Buffer sessions is disabled, enable it via configuration");
            }
            template <typename TWriter>
            void flushSessions(TWriter& ) {
            }
        };


        template <typename Config>
        struct DisabledBufferSessionsReader {
            template <typename TReader, typename TBufferContext>
            DisabledBufferSessionsReader(TReader& , TBufferContext& ) {
            }

            void begin() {
                static_assert(Config::BufferSessionsEnabled, "Buffer sessions is disabled, enable it via configuration");
            }

            void end() {
                static_assert(Config::BufferSessionsEnabled, "Buffer sessions is disabled, enable it via configuration");
            }

            bool hasActiveSessions() const {
                return false;
            }
        };

        template <typename TWriter>
        class BufferSessionsWriter {
        public:

            void begin(TWriter& ) {
                //write position
                _sessionIndex.push(_sessions.size());
                _sessions.emplace_back(0);
            }

            void end(TWriter& writer) {
                assert(!_sessionIndex.empty());
                //change position to session end
                auto sessionIt = std::next(std::begin(_sessions), _sessionIndex.top());
                _sessionIndex.pop();
                auto range = writer.getWrittenRange();
                *sessionIt = static_cast<size_t>(std::distance(range.begin(), range.end()));
            }

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

        template <typename TReader, typename TBufferContext>
        struct BufferSessionsReader {
            using TIterator = typename TReader::BufferIteratorType;

            BufferSessionsReader(TReader& r, TBufferContext& bufferContext)
                    :_reader{r},
                     _begin{bufferContext._pos},
                     _context{bufferContext}
            {
            }
            void begin() {
                if (_sessions.empty()) {
                    if (!initializeSessions())
                        return;
                }

                //save end position for current session
                _sessionsStack.push(_context._end);
                if (_nextSessionIt != std::end(_sessions)) {
                    if (std::distance(_context._pos, _context._end) > 0) {
                        //set end position for new session
                        auto newEnd = std::next(_begin, *_nextSessionIt);
                        if (std::distance(newEnd, _context._end) < 0)
                        {
                            //new session cannot end further than current end
                            _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                            return;
                        }
                        _context._end = newEnd;
                        ++_nextSessionIt;
                    }
                    //if we reached the end, means that there is no more data to read, hence there is no more sessions to advance to
                } else {
                    //there is no data to read anymore
                    //pos == end or buffer overflow while session is active
                    if (!(_context._pos == _context._end || _reader.getError() == BufferReaderError::NO_ERROR)) {
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
                    auto dist = std::distance(_context._pos, _context._end);
                    if (dist > 0) {
                        //newer version might have some inner sessions, try to find the one after current ends
                        auto currPos = static_cast<size_t>(std::distance(_begin, _context._end));
                        for (; _nextSessionIt != std::end(_sessions); ++_nextSessionIt) {
                            if (*_nextSessionIt > currPos)
                                break;
                        }
                    }
                    _context._pos = _context._end;
                    //restore end position
                    _context._end = _sessionsStack.top();
                    _sessionsStack.pop();
                }
            }

            bool hasActiveSessions() const {
                return _sessionsStack.size() > 0;
            }

        private:
            TReader& _reader;
            TIterator _begin;
            TBufferContext& _context;
            std::vector<size_t> _sessions{};
            std::vector<size_t>::iterator _nextSessionIt{};
            std::stack<TIterator> _sessionsStack{};

            bool initializeSessions() {
                //save current position
                auto currPos = _context._pos;
                //read size
                if (std::distance(_context._pos, _context._end) < 2) {
                    _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                    return false;
                }
                auto endSessionsSizesIt = std::next(_context._end, -2);
                _context._pos = endSessionsSizesIt;
                size_t sessionsOffset{};
                uint16_t high;
                _reader.template readBytes<2>(high);


                if (high >= 0x8000u) {
                    endSessionsSizesIt = std::next(endSessionsSizesIt, -2);
                    _context._pos = endSessionsSizesIt;
                    if (std::distance(_begin, _context._pos) < 0) {
                        _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                        return false;
                    }
                    uint16_t low;
                    _reader.template readBytes<2>(low);
                    //mask out last bit
                    high &= 0x7FFFu;
                    sessionsOffset = static_cast<size_t>((high << 16) | low);

                } else
                    sessionsOffset = high;

                auto bufferSize = std::distance(_begin, _context._end);
                if (static_cast<size_t>(bufferSize) < sessionsOffset) {
                    _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                    return false;
                }
                //we can initialy resizes to this value, and we'll shrink it after reading
                //read session sizes
                auto sessionsIt = std::back_inserter(_sessions);
                _context._pos = std::next(_context._end, -sessionsOffset);
                while (std::distance(_context._pos, endSessionsSizesIt) > 0) {
                    size_t size;
                    details::readSize(_reader, size, bufferSize);
                    *sessionsIt++ = size;
                }
                _sessions.shrink_to_fit();
                //set iterators to data
                _context._pos = currPos;
                _context._end = std::next(_context._end, -sessionsOffset);
                _nextSessionIt = std::begin(_sessions);//set before first session;
                return true;
            }
        };

        //this class writes bytes and bits to underlying buffer, it has specializations for resizable and non-resizable buffers
        template<typename Buffer, bool isResizable>
        class WriteBufferContext {

        };

        template<typename Buffer>
        class WriteBufferContext<Buffer, false> {
        public:
            using TValue = typename ContainerTraits<Buffer>::TValue;
            using TIterator = typename BufferContainerTraits<Buffer>::TIterator;

            explicit WriteBufferContext(Buffer &buffer)
                    : _buffer{buffer},
                      _outIt{std::begin(buffer)},
                      _end{std::end(buffer)}
            {
            }

            void write(const TValue *data, size_t size) {
                auto tmp = _outIt;
                _outIt += size;
                assert(std::distance(_outIt, _end) >= 0);
                memcpy(std::addressof(*tmp), data, size);
            }

            BufferRange<TIterator> getWrittenRange() const {
                return BufferRange<TIterator>{std::begin(_buffer), _outIt};
            }

        private:
            Buffer &_buffer;
            TIterator _outIt;
            TIterator _end;
        };

        template<typename Buffer>
        class WriteBufferContext<Buffer, true> {
        public:
            using TValue = typename ContainerTraits<Buffer>::TValue;
            using TIterator = typename BufferContainerTraits<Buffer>::TIterator;

            explicit WriteBufferContext(Buffer &buffer)
                    : _buffer{buffer}
            {
                //resize buffer immediately, because we need output iterator at valid position
                if (ContainerTraits<Buffer>::size(buffer) == 0u) {
                    BufferContainerTraits<Buffer>::increaseBufferSize(_buffer);
                }
                getIterators(0);
            }

            void write(const TValue *data, size_t size) {
                auto tmp = _outIt;
                _outIt += size;
                if (std::distance(_outIt , _end) >= 0) {
                    std::memcpy(std::addressof(*tmp), data, size);
                } else {
                    _outIt -= size;
                    //get current position before invalidating iterators
                    const auto pos = std::distance(std::begin(_buffer), _outIt);
                    //increase container size
                    BufferContainerTraits<Buffer>::increaseBufferSize(_buffer);
                    //restore iterators
                    getIterators(static_cast<size_t>(pos));
                    write(data, size);
                }
            }

            BufferRange<TIterator> getWrittenRange() const {
                return BufferRange<TIterator>{std::begin(_buffer), _outIt};
            }

        private:

            void getIterators(size_t writePos) {
                _end = std::end(_buffer);
                _outIt = std::next(std::begin(_buffer), writePos);
            }
            Buffer &_buffer;
            TIterator _outIt;
            TIterator _end;
        };



        template <typename Buffer>
        class ReadBufferContext {
        public:
            using TValue = typename ContainerTraits<Buffer>::TValue;
            using TIterator = typename BufferContainerTraits<Buffer>::TIterator;

            ReadBufferContext(TIterator begin, TIterator end)
                    :_pos{begin},
                     _end{end}
            {

            }

            void read(TValue* data, size_t size) {
                //for optimization
                auto tmp = _pos;
                _pos += size;
                if (std::distance(_pos, _end) >= 0) {
                    std::memcpy(data, std::addressof(*tmp), size);
                } else {
                    _pos -= size;
                    //set everything to zeros
                    std::memset(data, 0, size);

                    if (getError() == BufferReaderError::NO_ERROR)
                        setError(BufferReaderError::BUFFER_OVERFLOW);
                }
            }

            BufferReaderError getError() const {
                auto res = std::distance(_end, _pos);
                if (res > 0) {
                    auto err = static_cast<BufferReaderError>(res);
                    return err;
                }
                return BufferReaderError::NO_ERROR;
            }

            void setError(BufferReaderError error) {
                _end = _pos;
                //to avoid creating temporary for error state, mark an error by passing _pos after the _end
                std::advance(_pos, static_cast<size_t>(error));
            }

            bool isCompletedSuccessfully() const {
                return _pos == _end;
            }

            TIterator _pos;
            TIterator _end;
        };

    }
}


#endif //BITSERY_DETAILS_BUFFER_COMMON_H
