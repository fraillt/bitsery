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

    template<typename Buffer, typename Config = DefaultConfig>
    class InputBufferAdapter: public details::InputAdapterBaseCRTP<InputBufferAdapter<Buffer,Config>> {
    public:
        friend details::InputAdapterBaseCRTP<InputBufferAdapter<Buffer,Config>>;
        using TConfig = Config;
        using TIterator = typename traits::BufferAdapterTraits<typename std::remove_const<Buffer>::type>::TConstIterator;
        using TValue = typename traits::BufferAdapterTraits<typename std::remove_const<Buffer>::type>::TValue;
        static_assert(details::IsDefined<TValue>::value,
                      "Please define BufferAdapterTraits or include from <bitsery/traits/...>");
        static_assert(traits::ContainerTraits<typename std::remove_const<Buffer>::type>::isContiguous,
                      "BufferAdapter only works with contiguous containers");

        InputBufferAdapter(TIterator beginIt, size_t size)
                : _beginIt{beginIt},
                _currOffset{0},
                _endReadOffset{size},
                _bufferSize{size} {
        };

        InputBufferAdapter(TIterator beginIt, TIterator endIt)
                :InputBufferAdapter(beginIt, static_cast<size_t>(std::distance(beginIt, endIt))) {
        }

        InputBufferAdapter(const InputBufferAdapter&) = delete;
        InputBufferAdapter& operator=(const InputBufferAdapter&) = delete;

        InputBufferAdapter(InputBufferAdapter&&) = default;
        InputBufferAdapter& operator = (InputBufferAdapter&&) = default;

        void currentReadPos(size_t pos) {
            currentReadPosChecked(pos, std::integral_constant<bool, Config::CheckAdapterErrors>{});
        }

        size_t currentReadPos() const {
            return currentReadPosChecked(std::integral_constant<bool, Config::CheckAdapterErrors>{});
        }

        void currentReadEndPos(size_t pos) {
            // assert that CheckAdapterErrors is enabled, otherwise it will simply will not work even if data and buffer is not corrupted
            static_assert(Config::CheckAdapterErrors, "Please enable CheckAdapterErrors to use this functionality.");
            if (_bufferSize >= pos && error() == ReaderError::NoError) {
                _overflowOnReadEndPos = pos == 0;
                if (pos == 0)
                    pos = _bufferSize;
                _endReadOffset = pos;
            } else {
                error(ReaderError::DataOverflow);
            }
        }

        size_t currentReadEndPos() const {
            if (_overflowOnReadEndPos)
                return 0;
            return _endReadOffset;
        }

        ReaderError error() const {
            return _currOffset <= _endReadOffset
                ? ReaderError::NoError
                : static_cast<ReaderError>(_currOffset - _endReadOffset);
        }

        void error(ReaderError error) {
            if (_currOffset <= _endReadOffset) {
                _endReadOffset = 0;
                _bufferSize = 0;
                _currOffset = static_cast<size_t>(error);
            }
        }

        bool isCompletedSuccessfully() const {
            return _currOffset == _bufferSize;
        }

    private:
        using diff_t = typename std::iterator_traits<TIterator>::difference_type;

        template <size_t SIZE>
        void readInternalValue(TValue *data) {
            readInternalValueChecked<SIZE>(data, std::integral_constant<bool, Config::CheckAdapterErrors>{});
        }

        void readInternalBuffer(TValue *data, size_t size) {
            readInternalBufferChecked(data, size, std::integral_constant<bool, Config::CheckAdapterErrors>{});
        }

        template <size_t SIZE>
        void readInternalValueChecked(TValue *data, std::false_type) {
            const size_t newOffset = _currOffset + SIZE;
            assert(newOffset <= _endReadOffset);
            std::copy_n(_beginIt + static_cast<diff_t>(_currOffset), SIZE, data);
            _currOffset = newOffset;
        }

        template <size_t SIZE>
        void readInternalValueChecked(TValue *data, std::true_type) {
            const size_t newOffset = _currOffset + SIZE;
            if (newOffset <= _endReadOffset) {
                std::copy_n(_beginIt + static_cast<diff_t>(_currOffset), SIZE, data);
                _currOffset = newOffset;
            } else {
                //set everything to zeros
                std::memset(data, 0, SIZE);
                if (_overflowOnReadEndPos)
                    error(ReaderError::DataOverflow);
            }
        }

        void readInternalBufferChecked(TValue *data, size_t size, std::false_type) {
            const size_t newOffset = _currOffset + size;
            assert(newOffset <= _endReadOffset);
            std::copy_n(_beginIt + static_cast<diff_t>(_currOffset), size, data);
            _currOffset = newOffset;
        }

        void readInternalBufferChecked(TValue *data, size_t size, std::true_type) {
            const size_t newOffset = _currOffset + size;
            if (newOffset <= _endReadOffset) {
                std::copy_n(_beginIt + static_cast<diff_t>(_currOffset), size, data);
                _currOffset = newOffset;
            } else {
                //set everything to zeros
                std::memset(data, 0, size);
                if (_overflowOnReadEndPos)
                    error(ReaderError::DataOverflow);
            }
        }

        void currentReadPosChecked(size_t pos, std::true_type) {
            if (_bufferSize >= pos && error() == ReaderError::NoError) {
                _currOffset = pos;
            } else {
                error(ReaderError::DataOverflow);
            }
        }

        void currentReadPosChecked(size_t pos, std::false_type) {
            _currOffset = pos;
        }

        size_t currentReadPosChecked(std::true_type) const {
            return error() == ReaderError::NoError ? _currOffset : 0;
        }

        size_t currentReadPosChecked(std::false_type) const {
            return _currOffset;
        }


        TIterator _beginIt;
        size_t _currOffset;
        size_t _endReadOffset;
        size_t _bufferSize;
        bool _overflowOnReadEndPos = true;
    };

    template<typename Buffer, typename Config = DefaultConfig>
    class OutputBufferAdapter: public details::OutputAdapterBaseCRTP<OutputBufferAdapter<Buffer,Config>> {
    public:
        friend details::OutputAdapterBaseCRTP<OutputBufferAdapter<Buffer,Config>>;
        using TConfig = Config;
        using TIterator = typename traits::BufferAdapterTraits<Buffer>::TIterator;
        using TValue = typename traits::BufferAdapterTraits<Buffer>::TValue;

        static_assert(details::IsDefined<TValue>::value,
                      "Please define BufferAdapterTraits or include from <bitsery/traits/...>");
        static_assert(traits::ContainerTraits<Buffer>::isContiguous,
                      "BufferAdapter only works with contiguous containers");

        OutputBufferAdapter(Buffer &buffer)
                : _buffer{std::addressof(buffer)},
                _beginIt{std::begin(buffer)} {
            init(TResizable{});
        }

        OutputBufferAdapter(const OutputBufferAdapter&) = delete;
        OutputBufferAdapter& operator=(const OutputBufferAdapter&) = delete;
        OutputBufferAdapter(OutputBufferAdapter&&) = default;
        OutputBufferAdapter& operator = (OutputBufferAdapter&&) = default;

        void currentWritePos(size_t pos) {
            const auto maxPos = _currOffset > pos ? _currOffset : pos;
            if (maxPos > _biggestCurrentPos) {
                _biggestCurrentPos = maxPos;
            }
            setCurrentWritePos(pos, TResizable{});
        }

        size_t currentWritePos() const {
            return _currOffset;
        }

        void flush() {
            //this function might be useful for stream adapters
        }

        size_t writtenBytesCount() const {
            return _currOffset > _biggestCurrentPos ? _currOffset : _biggestCurrentPos;
        }

    private:
        using TResizable = std::integral_constant<bool, traits::ContainerTraits<Buffer>::isResizable>;
        using diff_t = typename std::iterator_traits<TIterator>::difference_type;

        template <size_t SIZE>
        void writeInternalValue(const TValue *data) {
            writeInternalValueImpl<SIZE>(data, TResizable{});
        }

        void writeInternalBuffer(const TValue *data, size_t size) {
            writeInternalBufferImpl(data, size, TResizable{});
        }

        Buffer* _buffer;
        TIterator _beginIt;
        size_t _currOffset{0};
        size_t _bufferSize{0};
        size_t _biggestCurrentPos{0};

        /*
         * resizable buffer
         */

        void init(std::true_type) {
            //resize buffer immediately, because we need output iterator at valid position
            if (traits::ContainerTraits<Buffer>::size(*_buffer) == 0u) {
                traits::BufferAdapterTraits<Buffer>::increaseBufferSize(*_buffer);
            }
            updateIteratorAndSize();
        }

        template <size_t SIZE>
        void writeInternalValueImpl(const TValue *data, std::true_type) {
            const size_t newOffset = _currOffset + SIZE;
            if (newOffset <= _bufferSize) {
                std::copy_n(data, SIZE, _beginIt + static_cast<diff_t>(_currOffset));
                _currOffset = newOffset;
            } else {
                traits::BufferAdapterTraits<Buffer>::increaseBufferSize(*_buffer);
                updateIteratorAndSize();
                writeInternalValueImpl<SIZE>(data, std::true_type{});
            }
        }

        void writeInternalBufferImpl(const TValue *data, const size_t size, std::true_type) {
            const size_t newOffset = _currOffset + size;
            if (newOffset <= _bufferSize) {
                std::copy_n(data, size, _beginIt + static_cast<diff_t>(_currOffset));
                _currOffset = newOffset;
            } else {
                traits::BufferAdapterTraits<Buffer>::increaseBufferSize(*_buffer);
                updateIteratorAndSize();
                writeInternalBufferImpl(data, size, std::true_type{});
            }
        }

        void setCurrentWritePos(size_t pos, std::true_type) {
            if (pos <= _bufferSize) {
                _currOffset = pos;
            } else {
                traits::BufferAdapterTraits<Buffer>::increaseBufferSize(*_buffer);
                updateIteratorAndSize();
                setCurrentWritePos(pos, std::true_type{});
            }
        }

        /*
         * non resizable buffer
         */
        void init(std::false_type) {
            updateIteratorAndSize();
        }

        template <size_t SIZE>
        void writeInternalValueImpl(const TValue *data, std::false_type) {
            const size_t newOffset = _currOffset + SIZE;
            assert(newOffset <= _bufferSize);
            std::copy_n(data, SIZE, _beginIt + static_cast<diff_t>(_currOffset));
            _currOffset = newOffset;
        }

        void writeInternalBufferImpl(const TValue *data, size_t size, std::false_type) {
            const size_t newOffset = _currOffset + size;
            assert(newOffset <= _bufferSize);
            std::copy_n(data, size, _beginIt + static_cast<diff_t>(_currOffset));
            _currOffset = newOffset;
        }

        void setCurrentWritePos(size_t pos, std::false_type) {
            assert(pos <= _bufferSize);
            _currOffset = pos;
        }

        void updateIteratorAndSize() {
            _beginIt = std::begin(*_buffer);
            _bufferSize = traits::ContainerTraits<Buffer>::size(*_buffer);
        }
    };

}

#endif //BITSERY_ADAPTER_BUFFER_H
