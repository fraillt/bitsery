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

#ifndef BITSERY_ADAPTER_STREAM_H
#define BITSERY_ADAPTER_STREAM_H

#include "../details/adapter_common.h"
#include "../traits/array.h"
#include <ios>

namespace bitsery {

    template <typename TChar, typename Config, typename CharTraits>
    class BasicInputStreamAdapter: public details::InputAdapterBaseCRTP<BasicInputStreamAdapter<TChar, Config, CharTraits>> {
    public:
        friend details::InputAdapterBaseCRTP<BasicInputStreamAdapter<TChar, Config, CharTraits>>;
        using TConfig = Config;
        using TValue = TChar;

        BasicInputStreamAdapter(std::basic_ios<TChar, CharTraits>& istream)
                :_ios{std::addressof(istream)} {}

        BasicInputStreamAdapter(const BasicInputStreamAdapter&) = delete;
        BasicInputStreamAdapter& operator = (const BasicInputStreamAdapter&) = delete;

        BasicInputStreamAdapter(BasicInputStreamAdapter&&) = default;
        BasicInputStreamAdapter& operator = (BasicInputStreamAdapter&&) = default;

        void currentReadPos(size_t ) {
            static_assert(std::is_void<TChar>::value, "setting read position is not supported with StreamAdapter");
        }

        size_t currentReadPos() const {
            static_assert(std::is_void<TChar>::value, "setting read position is not supported with StreamAdapter");
            return {};
        }

        void currentReadEndPos(size_t ) {
            static_assert(std::is_void<TChar>::value, "setting read position is not supported with StreamAdapter");
        }

        size_t currentReadEndPos() const {
            static_assert(std::is_void<TChar>::value, "setting read position is not supported with StreamAdapter");
            return {};
        }

        ReaderError error() const {
            return _err;
        }

        bool isCompletedSuccessfully() const {
            if (error() == ReaderError::NoError) {
                return _ios->rdbuf()->sgetc() == CharTraits::eof();
            }
            return false;
        }

        void error(ReaderError error) {
            if (_err == ReaderError::NoError) {
                _err = error;
                _zeroIfNoErrors = std::numeric_limits<size_t>::max();
            }
        }

    private:

        void readInternal(TValue* data, size_t size) {
            readChecked(data, size, std::integral_constant<bool, Config::CheckAdapterErrors>{});
        }

        void readChecked(TValue* data, size_t size, std::true_type) {
            if (size - static_cast<size_t>(_ios->rdbuf()->sgetn(data, size)) != _zeroIfNoErrors) {
                *data = {};
                if (_zeroIfNoErrors == 0) {
                    error(_ios->rdstate() == std::ios_base::badbit
                          ? ReaderError::ReadingError
                          : ReaderError::DataOverflow);
                }
            }
        }

        void readChecked(TValue* data, size_t size, std::false_type) {
            _ios->rdbuf()->sgetn(data , size);
        }

        std::basic_ios<TChar, CharTraits>* _ios;
        size_t _zeroIfNoErrors{};
        ReaderError _err = ReaderError::NoError;
    };

    template <typename TChar, typename Config, typename CharTraits>
    class BasicOutputStreamAdapter: public details::OutputAdapterBaseCRTP<BasicOutputStreamAdapter<TChar, Config, CharTraits>> {
    public:
        friend details::OutputAdapterBaseCRTP<BasicOutputStreamAdapter<TChar, Config, CharTraits>>;
        using TConfig = Config;
        using TValue = TChar;

        BasicOutputStreamAdapter(std::basic_ios<TChar, CharTraits>& ostream)
                :_ios{std::addressof(ostream)} {}

        void currentWritePos(size_t ) {
            static_assert(std::is_void<TChar>::value, "setting write position is not supported with StreamAdapter");
        }

        size_t currentWritePos() const {
            static_assert(std::is_void<TChar>::value, "setting write position is not supported with StreamAdapter");
            return {};
        }

        void flush() {
            if (auto ostream = dynamic_cast<std::basic_ostream<TChar, CharTraits>*>(_ios))
                ostream->flush();
        }

        size_t writtenBytesCount() const {
            static_assert(std::is_void<TChar>::value, "`writtenBytesCount` cannot be used with stream adapter");
            //streaming doesn't return written bytes
            return 0u;
        }

    private:

        void writeInternal(const TValue* data, size_t size) {
            //for optimization
            _ios->rdbuf()->sputn( data , size );
        }

        std::basic_ios<TChar, CharTraits>* _ios;
    };

    template <typename TChar, typename Config, typename CharTraits, typename TBuffer = std::array<TChar, 256>>
    class BasicBufferedOutputStreamAdapter:
        public details::OutputAdapterBaseCRTP<BasicBufferedOutputStreamAdapter<TChar, Config, CharTraits, TBuffer>> {
    public:
        friend details::OutputAdapterBaseCRTP<BasicBufferedOutputStreamAdapter<TChar, Config, CharTraits, TBuffer>>;
        using TConfig = Config;
        using Buffer = TBuffer;
        using BufferIt = typename traits::BufferAdapterTraits<TBuffer>::TIterator;
        static_assert(details::IsDefined<BufferIt>::value, "Please define BufferAdapterTraits or include from <bitsery/traits/...> to use as buffer for BasicBufferedOutputStreamAdapter");
        static_assert(traits::ContainerTraits<Buffer>::isContiguous, "BasicBufferedOutputStreamAdapter only works with contiguous containers");
        using TValue = TChar;

        //bufferSize is used when buffer is dynamically allocated
        BasicBufferedOutputStreamAdapter(std::basic_ios<TChar, CharTraits>& ostream, size_t bufferSize = 256)
                :_ios(std::addressof(ostream)),
                 _buf{},
                 _outIt{}
        {
            init(bufferSize, TResizable{});
        }

        //we need to explicitly declare move logic, because after move buffer might be invalidated
        BasicBufferedOutputStreamAdapter(const BasicBufferedOutputStreamAdapter&) = delete;
        BasicBufferedOutputStreamAdapter& operator = (const BasicBufferedOutputStreamAdapter&) = delete;

        BasicBufferedOutputStreamAdapter(BasicBufferedOutputStreamAdapter&& rhs)
                : _ios{rhs._ios},
                  _buf{},
                  _outIt{}
        {
            auto size = std::distance(std::begin(rhs._buf), rhs._outIt);
            _buf = std::move(rhs._buf);
            _outIt = std::next(std::begin(_buf), size);
        };

        BasicBufferedOutputStreamAdapter& operator = (BasicBufferedOutputStreamAdapter&& rhs) {
            _ios = rhs._ios;
            //get current written size, before move
            auto size = std::distance(std::begin(rhs._buf), rhs._outIt);
            _buf = std::move(rhs._buf);
            _outIt = std::next(std::begin(_buf), size);
            return *this;
        };

        void currentWritePos(size_t ) {
            static_assert(std::is_void<TChar>::value, "setting write position is not supported with StreamAdapter");
        }

        size_t currentWritePos() const {
            static_assert(std::is_void<TChar>::value, "setting write position is not supported with StreamAdapter");
            return {};
        }

        void flush() {
            auto begin = std::begin(_buf);
            writeToStream(std::addressof(*begin), static_cast<size_t>(std::distance(begin, _outIt)));
            _outIt = begin;
            if (auto ostream = dynamic_cast<std::basic_ostream<TChar, CharTraits>*>(_ios))
                ostream->flush();
        }

        size_t writtenBytesCount() const {
            static_assert(std::is_void<TChar>::value, "`writtenBytesCount` cannot be used with stream adapter");
            //streaming doesn't return written bytes
            return 0u;
        }

    private:
        using TResizable = std::integral_constant<bool, traits::ContainerTraits<TBuffer>::isResizable>;

        void writeInternal(const TValue* data, size_t size) {
            auto tmp = _outIt;

#if defined(_MSC_VER) && (_ITERATOR_DEBUG_LEVEL > 0)
            using TDistance = typename std::iterator_traits<BufferIt>::difference_type;
            if (std::distance(_outIt , std::end(_buf)) >= static_cast<TDistance>(size)) {
                std::memcpy(std::addressof(*_outIt), data, size);
                _outIt += size;
            }
#else
            _outIt += size;
            if (std::distance(_outIt , std::end(_buf)) >= 0) {
                std::memcpy(std::addressof(*tmp), data, size);
            }
#endif
            else {
                //when buffer is full write out to stream
                _outIt = std::begin(_buf);
                writeToStream(std::addressof(*_outIt), static_cast<size_t>(std::distance(_outIt, tmp)));
                writeToStream(data, size);
            }
        }

        void writeToStream(const TValue* data, size_t size) {
            _ios->rdbuf()->sputn( data , size );
        }

        void init (size_t bufferSize, std::true_type) {
            _buf.resize(bufferSize);
            _outIt = std::begin(_buf);
        }
        void init (size_t, std::false_type) {
            _outIt = std::begin(_buf);
        }

        std::basic_ios<TChar, CharTraits>* _ios;
        TBuffer _buf;
        BufferIt _outIt;
    };

    template <typename TChar, typename Config, typename CharTraits>
    class BasicIOStreamAdapter:public BasicInputStreamAdapter<TChar, Config, CharTraits>, public BasicOutputStreamAdapter<TChar, Config, CharTraits> {
    public:
        using TValue = TChar;

        //both bases contain reference to same iostream, so no need to do anything
        BasicIOStreamAdapter(std::basic_ios<TChar, CharTraits>& iostream)
                :BasicInputStreamAdapter<TChar, Config, CharTraits>{iostream},
                 BasicOutputStreamAdapter<TChar, Config, CharTraits>{iostream} {

        }
    };

    //helper types for most common implementations for std streams
    using OutputStreamAdapter = BasicOutputStreamAdapter<char, DefaultConfig, std::char_traits<char>>;
    using InputStreamAdapter = BasicInputStreamAdapter<char, DefaultConfig, std::char_traits<char>>;
    using IOStreamAdapter = BasicIOStreamAdapter<char, DefaultConfig, std::char_traits<char>>;

    using OutputBufferedStreamAdapter = BasicBufferedOutputStreamAdapter<char, DefaultConfig, std::char_traits<char>>;
}

#endif //BITSERY_ADAPTER_STREAM_H
