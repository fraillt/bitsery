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


#ifndef BITSERY_ADAPTERS_DYNAMIC_STREAM_H
#define BITSERY_ADAPTERS_DYNAMIC_STREAM_H

#include "../details/adapter_common.h"
#include "../traits/core/traits.h"
#include <ios>


namespace bitsery {

    template <typename TChar, typename CharTraits>
    class BasicInputStreamAdapter {
    public:
        using TValue = TChar;
        using TIterator = void;//TIterator is used with sessions, but streams cannot be used with sessions

        BasicInputStreamAdapter(std::basic_ios<TChar, CharTraits>& istream)
                :_ios{istream} {}

        void read(TValue* data, size_t size) {
            _ios.rdbuf()->sgetn( data , size );
        }

        ReaderError error() const {
            if (!_ios.bad())
                return ReaderError::NoError;
            return _ios.eof()
                   ? ReaderError::DataOverflow
                   : ReaderError::ReadingError;
        }
        bool isCompletedSuccessfully() const {
            if (error() == ReaderError::NoError) {
                return _ios.rdbuf()->sgetc() == CharTraits::eof();
            }
            return false;
        }
        void setError(ReaderError error) {
            //has no effect when using
        }

    private:
        std::basic_ios<TChar, CharTraits>& _ios;
    };

    template <typename TChar, typename CharTraits>
    class BasicOutputStreamAdapter {
    public:
        using TValue = TChar;
        using TIterator = void;//TIterator is used with sessions, but streams cannot be used with sessions

        BasicOutputStreamAdapter(std::basic_ios<TChar, CharTraits>& ostream):_ios{ostream} {}

        void write(const TValue* data, size_t size) {
            //for optimization
            _ios.rdbuf()->sputn( data , size );
        }

        void flush() {
            if (auto ostream = dynamic_cast<std::basic_ostream<TChar, CharTraits>*>(&_ios))
                ostream->flush();
        }

        size_t writtenBytesCount() const {
            //streaming doesn't return written bytes
            return 0;
        }

        //this method is only for stream writing
        bool isValidState() const {
            return !_ios.bad();
        }

    private:
        std::basic_ios<TChar, CharTraits>& _ios;
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
}

#endif //BITSERY_ADAPTERS_DYNAMIC_STREAM_H
