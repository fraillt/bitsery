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


#ifndef BITSERY_ADAPTERS_INPUT_BUFFER_ADAPTER_H
#define BITSERY_ADAPTERS_INPUT_BUFFER_ADAPTER_H

#include "../details/buffer_common.h"
#include "../traits/core/traits.h"

namespace bitsery {

    template <typename Buffer>
    class InputBufferAdapter {
    public:

        using TValue = typename traits::BufferAdapterTraits<Buffer>::TValue;
        using TIterator = typename traits::BufferAdapterTraits<Buffer>::TIterator;
        static_assert(details::IsDefined<TValue>::value, "Please define BufferAdapterTraits or include from <bitsery/traits/...>");

        InputBufferAdapter(TIterator begin, TIterator end)
                :_pos{begin},
                 _end{end}
        {
        }

        InputBufferAdapter(TIterator begin, size_t size)
                :InputBufferAdapter(begin, std::next(begin, size))
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

                if (getError() == ReaderError::NO_ERROR)
                    setError(ReaderError::DATA_OVERFLOW);
            }
        }

        ReaderError getError() const {
            auto res = std::distance(_end, _pos);
            if (res > 0) {
                auto err = static_cast<ReaderError>(res);
                return err;
            }
            return ReaderError::NO_ERROR;
        }

        void setError(ReaderError error) {
            _end = _pos;
            //to avoid creating temporary for error state, mark an error by passing _pos after the _end
            std::advance(_pos, static_cast<size_t>(error));
        }

        bool isCompletedSuccessfully() const {
            return _pos == _end;
        }
    private:
        friend details::SessionAccess;

        TIterator _pos;
        TIterator _end;
    };


    template<typename Container>
    class OutputBufferAdapter {
    public:

        using TIterator = typename traits::BufferAdapterTraits<Container>::TIterator;
        using TValue = typename traits::BufferAdapterTraits<Container>::TValue;

        static_assert(details::IsDefined<TValue>::value, "Please define BufferAdapterTraits or include from <bitsery/traits/...>");

        OutputBufferAdapter(Container &buffer)
                : _buffer{buffer}
        {

            init(TResizable{});
        }


        void write(const TValue *data, size_t size) {
            writeInternal(data, size, TResizable{});
        }

        size_t getWrittenBytesCount() const {
            return static_cast<size_t>(std::distance(std::begin(_buffer), _outIt));
        }

    private:
        using TResizable = std::integral_constant<bool, traits::ContainerTraits<Container>::isResizable>;

        Container &_buffer;
        TIterator _outIt;
        TIterator _end;

        /*
         * resizable buffer
         */

        void init(std::true_type) {
            //resize buffer immediately, because we need output iterator at valid position
            if (traits::ContainerTraits<Container>::size(_buffer) == 0u) {
                traits::BufferAdapterTraits<Container>::increaseBufferSize(_buffer);
            }
            _end = std::end(_buffer);
            _outIt = std::begin(_buffer);
        }

        void writeInternal(const TValue *data, size_t size, std::true_type) {
            //optimization
            auto tmp = _outIt;
            _outIt += size;
            if (std::distance(_outIt , _end) >= 0) {
                std::memcpy(std::addressof(*tmp), data, size);
            } else {
                _outIt -= size;
                //get current position before invalidating iterators
                const auto pos = std::distance(std::begin(_buffer), _outIt);
                //increase container size
                traits::BufferAdapterTraits<Container>::increaseBufferSize(_buffer);
                //restore iterators
                _end = std::end(_buffer);
                _outIt = std::next(std::begin(_buffer), pos);

                writeInternal(data, size, std::true_type{});
            }
        }

        /*
         * non resizable buffer
         */
        void init(std::false_type) {
            _outIt = std::begin(_buffer);
            _end = std::end(_buffer);
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

#endif //BITSERY_ADAPTERS_INPUT_BUFFER_ADAPTER_H
