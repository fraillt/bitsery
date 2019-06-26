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


    template<typename Buffer>
    class BufferIterators {
        static constexpr bool isConstBuffer = std::is_const<Buffer>::value;
        using BuffNonConst = typename std::remove_const<Buffer>::type;
    public:
        BufferIterators(const BufferIterators&) = delete;
        BufferIterators& operator=(const BufferIterators&) = delete;
        BufferIterators(BufferIterators&&) = default;
        BufferIterators& operator=(BufferIterators&&) = default;
        virtual ~BufferIterators() = default;
    protected:

        using TIterator = typename std::conditional<isConstBuffer,
                typename traits::BufferAdapterTraits<BuffNonConst>::TConstIterator,
                typename traits::BufferAdapterTraits<BuffNonConst>::TIterator>::type;
        static_assert(details::IsDefined<TIterator>::value,
                      "Please define BufferAdapterTraits or include from <bitsery/traits/...>");
        BufferIterators(TIterator begin, TIterator end)
                : beginIt{begin},
                  posIt{begin},
                  endIt{end} {
        }

        TIterator beginIt;
        TIterator posIt;
        TIterator endIt;
    };

    template<typename Buffer, typename Config = DefaultConfig>
    class InputBufferAdapter: public BufferIterators<Buffer>,
        public details::InputAdapterBaseCRTP<InputBufferAdapter<Buffer,Config>> {
    public:
        friend details::InputAdapterBaseCRTP<InputBufferAdapter<Buffer,Config>>;
        using TConfig = Config;
        using TIterator = typename BufferIterators<Buffer>::TIterator;
        using TValue = typename traits::BufferAdapterTraits<typename std::remove_const<Buffer>::type>::TValue;
        static_assert(details::IsDefined<TValue>::value,
                      "Please define BufferAdapterTraits or include from <bitsery/traits/...>");
        static_assert(traits::ContainerTraits<typename std::remove_const<Buffer>::type>::isContiguous,
                      "BufferAdapter only works with contiguous containers");

        InputBufferAdapter(TIterator begin, TIterator endIt)
                : BufferIterators<Buffer>(begin, endIt),
                    _endReadPos{endIt} {

        }

        InputBufferAdapter(TIterator begin, size_t size)
                : BufferIterators<Buffer>(begin, std::next(begin, size)),
                  _endReadPos{std::next(begin, size)} {
        }

        InputBufferAdapter(const InputBufferAdapter&) = delete;
        InputBufferAdapter& operator=(const InputBufferAdapter&) = delete;

        InputBufferAdapter(InputBufferAdapter&&) = default;
        InputBufferAdapter& operator = (InputBufferAdapter&&) = default;

        void currentReadPos(size_t pos) {
            currentReadPosChecked(pos, std::integral_constant<bool, Config::CheckAdapterErrors>{});
        }

        size_t currentReadPos() const {
            return static_cast<size_t>(std::distance(this->beginIt, this->posIt));
        }

        void currentReadEndPos(size_t pos) {
            // assert that CheckAdapterErrors is enabled, otherwise it will simply will not work even if data and buffer is not corrupted
            static_assert(Config::CheckAdapterErrors, "Please enable CheckAdapterErrors to use this functionality.");
            const auto buffSize = static_cast<size_t>(std::distance(this->beginIt, this->endIt));
            if (buffSize >= pos) {
                _overflowOnReadEndPos = pos == 0;
                if (pos == 0)
                    pos = buffSize;
                _endReadPos = std::next(this->beginIt, pos);
            } else {
                error(ReaderError::DataOverflow);
            }
        }

        size_t currentReadEndPos() const {
            if (_overflowOnReadEndPos)
                return 0;
            return static_cast<size_t>(std::distance(this->beginIt, _endReadPos));
        }

        ReaderError error() const {
            return _err;
        }

        void error(ReaderError error) {
            if (_err == ReaderError::NoError) {
                _err = error;
                _endReadPos = this->endIt;
                this->beginIt = this->endIt;
                this->posIt = this->endIt;
            }
        }

        bool isCompletedSuccessfully() const {
            return this->posIt == this->endIt && _err == ReaderError::NoError;
        }

    private:

        void readChecked(TValue *data, size_t size, std::false_type) {
            //for optimization
            auto tmp = this->posIt;
            this->posIt += size;
            assert(std::distance(this->posIt, this->endIt) >= 0);
            std::memcpy(data, std::addressof(*tmp), size);
        }

        void readChecked(TValue *data, size_t size, std::true_type) {
            //for optimization
            auto tmp = this->posIt;
            this->posIt += size;
            if (std::distance(this->posIt, _endReadPos) >= 0) {
                std::memcpy(data, std::addressof(*tmp), size);
            } else {
                this->posIt -= size;
                //set everything to zeros
                std::memset(data, 0, size);
                if (_overflowOnReadEndPos)
                    error(ReaderError::DataOverflow);
            }
        }

        void readInternal(TValue *data, size_t size) {
            readChecked(data, size, std::integral_constant<bool, Config::CheckAdapterErrors>{});
        }

        void currentReadPosChecked(size_t pos, std::true_type) {
            if (static_cast<size_t>(std::distance(this->beginIt, this->endIt)) >= pos) {
                this->posIt = std::next(this->beginIt, pos);
            } else {
                error(ReaderError::DataOverflow);
            }
        }

        void currentReadPosChecked(size_t pos, std::false_type) {
            this->posIt = std::next(this->beginIt, pos);
        }

        TIterator _endReadPos;
        ReaderError _err = ReaderError::NoError;
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
                : _buffer{std::addressof(buffer)} {

            init(TResizable{});
        }

        OutputBufferAdapter(const OutputBufferAdapter&) = delete;
        OutputBufferAdapter& operator=(const OutputBufferAdapter&) = delete;
        OutputBufferAdapter(OutputBufferAdapter&&) = default;
        OutputBufferAdapter& operator = (OutputBufferAdapter&&) = default;

        void currentWritePos(size_t pos) {
            const auto currPos =static_cast<size_t>(std::distance(std::begin(*_buffer), _outIt));
            const auto maxPos = currPos > pos ? currPos : pos;
            if (maxPos > _biggestCurrentPos) {
                _biggestCurrentPos = maxPos;
            }
            setCurrentWritePos(pos, TResizable{});
        }

        size_t currentWritePos() const {
            return static_cast<size_t> (std::distance(std::begin(*_buffer), _outIt));
        }

        void flush() {
            //this function might be useful for stream adapters
        }

        size_t writtenBytesCount() const {
            const auto pos =static_cast<size_t>(std::distance(std::begin(*_buffer), _outIt));
            return pos > _biggestCurrentPos ? pos : _biggestCurrentPos;
        }

    private:
        using TResizable = std::integral_constant<bool, traits::ContainerTraits<Buffer>::isResizable>;

        void writeInternal(const TValue *data, size_t size) {
            writeInternalImpl(data, size, TResizable{});
        }

        Buffer* _buffer;
        TIterator _outIt{};
        TIterator _end{};
        size_t _biggestCurrentPos{};

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

        void writeInternalImpl(const TValue *data, const size_t size, std::true_type) {
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

                writeInternalImpl(data, size, std::true_type{});
            }
        }

        void setCurrentWritePos(size_t pos, std::true_type) {
            const auto begin = std::begin(*_buffer);
            if (static_cast<size_t>(std::distance(begin, std::end(*_buffer))) >= pos) {
                _outIt = std::next(begin, pos);
            } else {
                traits::BufferAdapterTraits<Buffer>::increaseBufferSize(*_buffer);
                setCurrentWritePos(pos, std::true_type{});
            }
        }

        /*
         * non resizable buffer
         */
        void init(std::false_type) {
            _outIt = std::begin(*_buffer);
            _end = std::end(*_buffer);
        }

        void writeInternalImpl(const TValue *data, size_t size, std::false_type) {
            //optimization
            auto tmp = _outIt;
            _outIt += size;
            assert(std::distance(_outIt, _end) >= 0);
            memcpy(std::addressof(*tmp), data, size);
        }

        void setCurrentWritePos(size_t pos, std::false_type) {
            const auto begin = std::begin(*_buffer);
            assert(static_cast<size_t>(std::distance(begin, std::end(*_buffer))) >= pos);
            _outIt = std::next(begin, pos);
        }
    };

}

#endif //BITSERY_ADAPTER_BUFFER_H
