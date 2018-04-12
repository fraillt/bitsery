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

    template <typename TChar, typename CharTraits>
    class BasicInputStreamAdapter {
    public:
        using TValue = TChar;
        using TIterator = void;//TIterator is used with sessions, but streams cannot be used with sessions

        BasicInputStreamAdapter(std::basic_ios<TChar, CharTraits>& istream)
                :_ios{std::addressof(istream)} {}

        template <typename T>
        void read(T& data) {
            read(reinterpret_cast<TValue*>(&data), sizeof(T));
        }

        void read(TValue* data, size_t size) {
            if (static_cast<size_t>(_ios->rdbuf()->sgetn( data , size )) != size) {
                *data = {};
                //check state, if not set by stream, set it manually
                if (_ios->good())
                    _ios->setstate(std::ios_base::eofbit);
            }

        }

        ReaderError error() const {
            if (_ios->good())
                return ReaderError::NoError;
            return _ios->eof()
                   ? ReaderError::DataOverflow
                   : ReaderError::ReadingError;
        }
        bool isCompletedSuccessfully() const {
            if (error() == ReaderError::NoError) {
                return _ios->rdbuf()->sgetc() == CharTraits::eof();
            }
            return false;
        }
        void setError(ReaderError ) {
            //has no effect when using
        }

    private:
        std::basic_ios<TChar, CharTraits>* _ios;
    };

    template <typename TChar, typename CharTraits>
    class BasicOutputStreamAdapter {
    public:
        using TValue = TChar;
        using TIterator = void;//TIterator is used with sessions, but streams cannot be used with sessions

        BasicOutputStreamAdapter(std::basic_ios<TChar, CharTraits>& ostream)
                :_ios{std::addressof(ostream)} {}

        template <typename T>
        void write(const T& data) {
            write(reinterpret_cast<const TValue*>(&data), sizeof(T));
        }

        void write(const TValue* data, size_t size) {
            //for optimization
            _ios->rdbuf()->sputn( data , size );
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

        //this method is only for stream writing
        bool isValidState() const {
            return !_ios->bad();
        }

    private:
        std::basic_ios<TChar, CharTraits>* _ios;
    };

    template <typename TChar, typename CharTraits, typename TBuffer = std::array<TChar, 256>>
    class BasicBufferedOutputStreamAdapter {
    public:
        using Buffer = TBuffer;
        using BufferIt = typename traits::BufferAdapterTraits<TBuffer>::TIterator;
        static_assert(details::IsDefined<BufferIt>::value, "Please define BufferAdapterTraits or include from <bitsery/traits/...> to use as buffer for BasicBufferedOutputStreamAdapter");
        static_assert(traits::ContainerTraits<Buffer>::isContiguous, "BasicBufferedOutputStreamAdapter only works with contiguous containers");
        using TValue = TChar;
        using TIterator = void;//TIterator is used with sessions, but streams cannot be used with sessions

        //bufferSize is used when buffer is dynamically allocated
        BasicBufferedOutputStreamAdapter(std::basic_ios<TChar, CharTraits>& ostream, size_t bufferSize = 256)
                :_adapter(ostream),
                 _buf{},
                 _outIt{}
        {
            init(bufferSize, TResizable{});
        }

        //we need to explicitly declare move logic, in case buffer is static, because after move it will be invalidated
        BasicBufferedOutputStreamAdapter(const BasicBufferedOutputStreamAdapter&) = delete;
        BasicBufferedOutputStreamAdapter& operator = (const BasicBufferedOutputStreamAdapter&) = delete;

        BasicBufferedOutputStreamAdapter(BasicBufferedOutputStreamAdapter&& rhs)
                : _adapter{std::move(rhs._adapter)},
                  _buf{},
                  _outIt{}
        {
            auto size = std::distance(std::begin(rhs._buf), rhs._outIt);
            _buf = std::move(rhs._buf);
            _outIt = std::next(std::begin(_buf), size);
        };

        BasicBufferedOutputStreamAdapter& operator = (BasicBufferedOutputStreamAdapter&& rhs) {
            _adapter = std::move(rhs._adapter);
            //get current written size, before move
            auto size = std::distance(std::begin(rhs._buf), rhs._outIt);
            _buf = std::move(rhs._buf);
            _outIt = std::next(std::begin(_buf), size);
            return *this;
        };

        ~BasicBufferedOutputStreamAdapter() = default;

        template <typename T>
        void write(const T& data) {
            auto tmp = _outIt;

#if defined(_MSC_VER) && (_ITERATOR_DEBUG_LEVEL > 0)
            using TDistance = typename std::iterator_traits<BufferIt>::difference_type;
            if (std::distance(_outIt , std::end(_buf)) >= static_cast<TDistance>(size)) {
                *reinterpret_cast<T*>(std::addressof(*tmp)) = data;
                _outIt += sizeof(T);
#else
            _outIt += sizeof(T);
            if (std::distance(_outIt , std::end(_buf)) >= 0) {
                *reinterpret_cast<T*>(std::addressof(*tmp)) = data;
#endif
            } else {
                //when buffer is full write out to stream
                _outIt = std::begin(_buf);
                _adapter.write(std::addressof(*_outIt), static_cast<size_t>(std::distance(_outIt, tmp)));
                _adapter.write(reinterpret_cast<const TValue*>(&data), sizeof(T));
            }
        }

        void write(const TValue* data, size_t size) {
            auto tmp = _outIt;

#if defined(_MSC_VER) && (_ITERATOR_DEBUG_LEVEL > 0)
            using TDistance = typename std::iterator_traits<BufferIt>::difference_type;
            if (std::distance(_outIt , std::end(_buf)) >= static_cast<TDistance>(size)) {
                std::memcpy(std::addressof(*_outIt), data, size);
                _outIt += size;
#else
            _outIt += size;
            if (std::distance(_outIt , std::end(_buf)) >= 0) {
                std::memcpy(std::addressof(*tmp), data, size);
#endif
            } else {
                //when buffer is full write out to stream
                _outIt = std::begin(_buf);
                _adapter.write(std::addressof(*_outIt), static_cast<size_t>(std::distance(_outIt, tmp)));
                _adapter.write(data, size);
            }
        }

        void flush() {
            auto begin = std::begin(_buf);
            _adapter.write(std::addressof(*begin), static_cast<size_t>(std::distance(begin, _outIt)));
            _outIt = begin;
            _adapter.flush();
        }

        size_t writtenBytesCount() const {
            return _adapter.writtenBytesCount();
        }

        //this method is only for stream writing
        bool isValidState() const {
            return _adapter.isValidState();
        }

    private:
        using TResizable = std::integral_constant<bool, traits::ContainerTraits<TBuffer>::isResizable>;

        void init (size_t bufferSize, std::true_type) {
            _buf.resize(bufferSize);
            _outIt = std::begin(_buf);
        }
        void init (size_t, std::false_type) {
            _outIt = std::begin(_buf);
        }

        BasicOutputStreamAdapter<TChar, CharTraits> _adapter;
        TBuffer _buf;
        BufferIt _outIt;
    };

    template <typename TChar, typename CharTraits>
    class BasicIOStreamAdapter:public BasicInputStreamAdapter<TChar, CharTraits>, public BasicOutputStreamAdapter<TChar, CharTraits> {
    public:
        using TValue = TChar;
        using TIterator = void;//TIterator is used with sessions, but streams cannot be used with sessions

        //both bases contain reference to same iostream, so no need to do anything
        BasicIOStreamAdapter(std::basic_ios<TChar, CharTraits>& iostream)
                :BasicInputStreamAdapter<TChar, CharTraits>{iostream},
                 BasicOutputStreamAdapter<TChar, CharTraits>{iostream} {

        }
    };

    //helper types for most common implementations for std streams
    using OutputStreamAdapter = BasicOutputStreamAdapter<char, std::char_traits<char>>;
    using InputStreamAdapter = BasicInputStreamAdapter<char, std::char_traits<char>>;
    using IOStreamAdapter = BasicIOStreamAdapter<char, std::char_traits<char>>;

    using OutputBufferedStreamAdapter = BasicBufferedOutputStreamAdapter<char, std::char_traits<char>>;
}

#endif //BITSERY_ADAPTER_STREAM_H
