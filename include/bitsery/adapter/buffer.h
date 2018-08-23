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

#ifndef BITSERY_ADAPTER_BUFFER_H
#define BITSERY_ADAPTER_BUFFER_H

#include "../details/adapter_common.h"
#include "../traits/core/traits.h"

namespace bitsery {

    //base class that stores container iterators, and is required for session support (for reading sessions data)
    template<typename Buffer>
    class BufferIterators {
    protected:
        using TIterator = typename traits::BufferAdapterTraits<Buffer>::TIterator;

        BufferIterators(TIterator begin, TIterator end)
                : posIt{begin},
                  endIt{end} {
        }

        friend details::SessionAccess;

        TIterator posIt;
        TIterator endIt;
    };

    template<typename Buffer>
    class InputBufferAdapter : public BufferIterators<Buffer> {
    public:

        using TIterator = typename BufferIterators<Buffer>::TIterator;
        using TValue = typename traits::BufferAdapterTraits<Buffer>::TValue;
        static_assert(details::IsDefined<TValue>::value,
                      "Please define BufferAdapterTraits or include from <bitsery/traits/...>");
        static_assert(traits::ContainerTraits<Buffer>::isContiguous,
                      "BufferAdapter only works with contiguous containers");

        InputBufferAdapter(TIterator begin, TIterator end)
                : BufferIterators<Buffer>(begin, end) {
        }

        InputBufferAdapter(TIterator begin, size_t size)
                : BufferIterators<Buffer>(begin, std::next(begin, size)) {
        }

        void read(TValue *data, size_t size) {
            //for optimization
            auto tmp = this->posIt;
            this->posIt += size;
            if (std::distance(this->posIt, this->endIt) >= 0) {
                std::memcpy(data, std::addressof(*tmp), size);
            } else {
                this->posIt -= size;
                //set everything to zeros
                std::memset(data, 0, size);
                if (error() == ReaderError::NoError)
                    setError(ReaderError::DataOverflow);
            }
        }

        ReaderError error() const {
            auto res = std::distance(this->endIt, this->posIt);
            if (res > 0) {
                auto err = static_cast<ReaderError>(res);
                return err;
            }
            return ReaderError::NoError;
        }

        void setError(ReaderError error) {
            this->endIt = this->posIt;
            //to avoid creating temporary for error state, mark an error by passing posIt after the endIt
            std::advance(this->posIt, static_cast<size_t>(error));
        }

        bool isCompletedSuccessfully() const {
            return this->posIt == this->endIt;
        }
    };

    template<typename Buffer>
    class UnsafeInputBufferAdapter : public BufferIterators<Buffer> {
    public:

        using TIterator = typename BufferIterators<Buffer>::TIterator;
        using TValue = typename traits::BufferAdapterTraits<Buffer>::TValue;
        static_assert(details::IsDefined<TValue>::value,
                      "Please define BufferAdapterTraits or include from <bitsery/traits/...>");
        static_assert(traits::ContainerTraits<Buffer>::isContiguous,
                      "BufferAdapter only works with contiguous containers");

        UnsafeInputBufferAdapter(TIterator begin, TIterator end) : BufferIterators<Buffer>(begin, end) {
        }

        UnsafeInputBufferAdapter(TIterator begin, size_t size)
                : BufferIterators<Buffer>(begin, std::next(begin, size)) {
        }

        void read(TValue *data, size_t size) {
            //for optimization
            auto tmp = this->posIt;
            this->posIt += size;
            assert(std::distance(this->posIt, this->endIt) >= 0);
            std::memcpy(data, std::addressof(*tmp), size);
        }

        ReaderError error() const {
            return err;
        }

        void setError(ReaderError error) {
            err = error;
        }

        bool isCompletedSuccessfully() const {
            return this->posIt == this->endIt;
        }

    private:
        ReaderError err = ReaderError::NoError;
    };

    template<typename Buffer>
    class OutputBufferAdapter {
    public:

        using TIterator = typename traits::BufferAdapterTraits<Buffer>::TIterator;
        using TValue = typename traits::BufferAdapterTraits<Buffer>::TValue;

        static_assert(details::IsDefined<TValue>::value,
                      "Please define BufferAdapterTraits or include from <bitsery/traits/...>");
        static_assert(traits::ContainerTraits<Buffer>::isContiguous,
                      "BufferAdapter only works with contiguous containers");

        OutputBufferAdapter(Buffer &buffer)
                : _buffer{std::addressof(buffer)} {

            init(TResizable{});
        }

        void write(const TValue *data, size_t size) {
            writeInternal(data, size, TResizable{});
        }

        void flush() {
            //this function might be useful for stream adapters
        }

        size_t writtenBytesCount() const {
            return static_cast<size_t>(std::distance(std::begin(*_buffer), _outIt));
        }

    private:
        using TResizable = std::integral_constant<bool, traits::ContainerTraits<Buffer>::isResizable>;

        Buffer *_buffer;
        TIterator _outIt{};
        TIterator _end{};

        /*
         * resizable buffer
         */

        void init(std::true_type) {
            //resize buffer immediately, because we need output iterator at valid position
            if (traits::ContainerTraits<Buffer>::size(*_buffer) == 0u) {
                traits::BufferAdapterTraits<Buffer>::increaseBufferSize(*_buffer);
            }
            _end = std::end(*_buffer);
            _outIt = std::begin(*_buffer);
        }

        void writeInternal(const TValue *data, const size_t size, std::true_type) {
            //optimization
#if defined(_MSC_VER) && (_ITERATOR_DEBUG_LEVEL > 0)
            using TDistance = typename std::iterator_traits<TIterator>::difference_type;
            if (std::distance(_outIt , _end) >= static_cast<TDistance>(size)) {
                std::memcpy(std::addressof(*_outIt), data, size);
                _outIt += size;
#else
            auto tmp = _outIt;
            _outIt += size;
            if (std::distance(_outIt, _end) >= 0) {
                std::memcpy(std::addressof(*tmp), data, size);
#endif
            } else {
#if defined(_MSC_VER) && (_ITERATOR_DEBUG_LEVEL > 0)

#else
                _outIt -= size;
#endif
                //get current position before invalidating iterators
                const auto pos = std::distance(std::begin(*_buffer), _outIt);
                //increase container size
                traits::BufferAdapterTraits<Buffer>::increaseBufferSize(*_buffer);
                //restore iterators
                _end = std::end(*_buffer);
                _outIt = std::next(std::begin(*_buffer), pos);

                writeInternal(data, size, std::true_type{});
            }
        }

        /*
         * non resizable buffer
         */
        void init(std::false_type) {
            _outIt = std::begin(*_buffer);
            _end = std::end(*_buffer);
        }

        void writeInternal(const TValue *data, size_t size, std::false_type) {
            //optimization
            auto tmp = _outIt;
            _outIt += size;
            assert(std::distance(_outIt, _end) >= 0);
            memcpy(std::addressof(*tmp), data, size);
        }
    };

}

#endif //BITSERY_ADAPTER_BUFFER_H
