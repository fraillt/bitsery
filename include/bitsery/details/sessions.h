//MIT License
//
//Copyright (c) 2018 Mindaugas Vinkelis
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

#ifndef BITSERY_DETAILS_SESSIONS_H
#define BITSERY_DETAILS_SESSIONS_H

#include <vector>
#include <stack>
#include "adapter_common.h"

namespace bitsery {

    namespace session {
        /*
         * writer/reader implementations that disable session support
         */
        template <typename TWriter>
        struct DisabledSessionsWriter {
            void begin(TWriter& ) {
                static_assert(std::is_void<TWriter>::value, "Sessions is disabled, enable it via configuration");
            }
            void end(TWriter& ) {
                static_assert(std::is_void<TWriter>::value, "Sessions is disabled, enable it via configuration");
            }
            void flushSessions(TWriter& ) {
            }
        };

        template <typename TReader>
        struct DisabledSessionsReader {
            template <typename TBufferContext>
            DisabledSessionsReader(TReader& , TBufferContext& ) {
            }

            void begin() {
                static_assert(std::is_void<TReader>::value, "Sessions is disabled, enable it via configuration");
            }

            void end() {
                static_assert(std::is_void<TReader>::value, "Sessions is disabled, enable it via configuration");
            }

            bool hasActiveSessions() const {
                return false;
            }
        };

        /*
         * writer/reader real implementations
         * sessions reading requires to have random access iterators, so it cannot be used with streams
         */
        template <typename TWriter>
        class SessionsWriter {
        public:

            SessionsWriter() = default;
            SessionsWriter(const SessionsWriter&) = delete;
            SessionsWriter& operator = (const SessionsWriter& ) = delete;

            SessionsWriter(SessionsWriter&&) = default;
            SessionsWriter& operator = (SessionsWriter&& ) = default;

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
                auto sessionSize = writer.writtenBytesCount();
                assert(sessionSize > 0);
                *sessionIt = sessionSize;
            }

            void flushSessions(TWriter& writer) {
                if (_sessions.size()) {
                    assert(_sessionIndex.empty());
                    auto dataSize = writer.writtenBytesCount();
                    for(auto& s:_sessions) {
                        details::writeSize(writer, s);
                    }
                    _sessions.clear();

                    auto totalSize = writer.writtenBytesCount();
                    //write offset where actual data ends
                    auto sessionsOffset = totalSize - dataSize + 4;//4 bytes for offset data
                    writer.template writeBytes<4>(static_cast<uint32_t>(sessionsOffset));
                }
            }
        private:
            std::vector<size_t> _sessions{};
            std::stack<size_t> _sessionIndex{};
        };

        template <typename TReader>
        struct SessionsReader {
            using TIterator = typename TReader::TIterator;

            template <typename InputAdapter>
            SessionsReader(TReader& r, InputAdapter& adapter)
                    :_reader{r},
                     _beginIt{details::SessionAccess::posIteratorRef<InputAdapter, TIterator>(adapter)},
                     _posItRef{details::SessionAccess::posIteratorRef<InputAdapter, TIterator>(adapter)},
                     _endItRef{details::SessionAccess::endIteratorRef<InputAdapter, TIterator>(adapter)}
            {
            }

            SessionsReader(const SessionsReader&) = delete;
            SessionsReader& operator = (const SessionsReader& ) = delete;

            SessionsReader(SessionsReader&&) = default;
            SessionsReader& operator = (SessionsReader&& ) = default;

            void begin() {
                if (_sessions.empty()) {
                    if (!initializeSessions())
                        return;
                }

                //save end position for current session
                _sessionsStack.push(_endItRef);
                if (_nextSessionIt != std::end(_sessions)) {
                    if (std::distance(_posItRef, _endItRef) > 0) {
                        //set end position for new session
                        auto newEnd = std::next(_beginIt, *_nextSessionIt);
                        if (std::distance(newEnd, _endItRef) < 0)
                        {
                            //new session cannot end further than current end
                            _reader.setError(ReaderError::InvalidData);
                            return;
                        }
                        _endItRef = newEnd;
                        ++_nextSessionIt;
                    }
                    //if we reached the end, means that there is no more data to read, hence there is no more sessions to advance to
                } else {
                    //there is no data to read anymore
                    //pos == end or buffer overflow while session is active
                    if (!(_posItRef == _endItRef || _reader.error() == ReaderError::NoError) || !(_sessionsStack.size() > 1)) {
                        _reader.setError(ReaderError::InvalidData);
                    }
                }
            }

            void end() {
                if (!_sessionsStack.empty()) {
                    //move position to the end of session
                    //can additionaly be checked for session data versioning
                    //_pos == _end : same versions
                    //distance(_pos,_end) > 0: reading newer version
                    //error() == BUFFER_OVERFLOW: reading older version
                    auto dist = std::distance(_posItRef, _endItRef);
                    if (dist > 0) {
                        //newer version might have some inner sessions, try to find the one after current ends
                        auto currPos = static_cast<size_t>(std::distance(_beginIt, _endItRef));
                        for (; _nextSessionIt != std::end(_sessions); ++_nextSessionIt) {
                            if (*_nextSessionIt > currPos)
                                break;
                        }
                    }
                    //modify pointers only if no error or buffer overflow
                    if (_reader.error() == ReaderError::NoError || _reader.error() == ReaderError::DataOverflow) {
                        _posItRef = _endItRef;
                        //restore end position
                        _endItRef = _sessionsStack.top();
                    }
                    _sessionsStack.pop();
                }
            }

            bool hasActiveSessions() const {
                return _sessionsStack.size() > 0;
            }

        private:
            TReader& _reader;
            TIterator _beginIt;
            TIterator& _posItRef;
            TIterator& _endItRef;
            std::vector<size_t> _sessions{};
            std::vector<size_t>::iterator _nextSessionIt{};
            std::stack<TIterator> _sessionsStack{};

            bool initializeSessions() {
                //save current position
                auto currPos = _posItRef;
                //read size
                if (std::distance(_posItRef, _endItRef) < 4) {
                    _reader.setError(ReaderError::InvalidData);
                    return false;
                }
                auto endSessionsSizesIt = std::next(_endItRef, -4);
                _posItRef = endSessionsSizesIt;
                uint32_t sessionsOffset{};
                _reader.template readBytes<4>(sessionsOffset);

                auto bufferSize = std::distance(_beginIt, _endItRef);
                if (static_cast<size_t>(bufferSize) < sessionsOffset) {
                    _reader.setError(ReaderError::InvalidData);
                    return false;
                }
                //we can initialy resizes to this value, and we'll shrink it after reading
                //read session sizes
                auto sessionsIt = std::back_inserter(_sessions);
                _posItRef = std::next(_endItRef, -static_cast<int32_t>(sessionsOffset));
                while (std::distance(_posItRef, endSessionsSizesIt) > 0) {
                    size_t size;
                    details::readSize(_reader, size, bufferSize);
                    *sessionsIt++ = size;
                }
                _sessions.shrink_to_fit();
                //set iterators to data
                _posItRef = currPos;
                _endItRef = std::next(_endItRef, -static_cast<int32_t>(sessionsOffset));
                _nextSessionIt = std::begin(_sessions);//set before first session;
                return true;
            }
        };

    }
}

#endif //BITSERY_DETAILS_SESSIONS_H
