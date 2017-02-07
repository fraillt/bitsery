//
// Created by Mindaugas Vinkelis on 2016-11-11.
//

#ifndef PROJECT_TEMPLATE_BUFFER_WRITER_H
#define PROJECT_TEMPLATE_BUFFER_WRITER_H

#include "Common.h"
#include <algorithm>

struct MeasureSize {

    template<size_t SIZE, typename T>
    void WriteBytes(const T& ) {
        static_assert(std::is_integral<T>(), "");
        static_assert(sizeof(T) == SIZE, "");
        _bytesCount += SIZE;
    }

    template<size_t SIZE, typename T>
    void WriteBits(const T& )  {
        static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
        static_assert(SIZE > 0 && SIZE <= BITS_SIZE<T>, "");
        _bytesCount += SIZE * 8;
    }

    template<size_t SIZE, typename T>
    void WriteBuffer(const T* , size_t count) {
        static_assert(std::is_integral<T>(), "");
        static_assert(sizeof(T) == SIZE, "");
        _bytesCount += SIZE * count;
    }

    size_t getSize() const {
        return _bytesCount;
    }
private:
    size_t _bytesCount{};

};


struct BufferWriter {
    using value_type = uint8_t;
    BufferWriter(std::vector<uint8_t>& buffer):_buf{buffer}, _outIt{std::back_inserter(buffer)} {

    }

    template<size_t SIZE, typename T>
    void WriteBytes(const T& v) {
        static_assert(std::is_integral<T>(), "");
        static_assert(sizeof(T) == SIZE, "");

        if (m_scratchBits) {
            using UT = typename std::make_unsigned<T>::type;
            WriteBits<SIZE * 8>(reinterpret_cast<const UT&>(v));
        } else {
            directWrite(&v,1);
        }
    }

    template<size_t SIZE, typename T>
    void WriteBuffer(const T* buf, size_t count) {
        static_assert(std::is_integral<T>(), "");
        static_assert(sizeof(T) == SIZE, "");
        if (m_scratchBits) {
            //todo implement
//            using UT = typename std::make_unsigned<T>::type;
//            WriteBits<SIZE * 8 * count>(reinterpret_cast<const UT&>(v));
        } else {
            directWrite(buf, count);
        }

    }

    template<size_t SIZE, typename T>
    void WriteBits(const T& v)  {
        static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
        static_assert(SIZE > 0 && SIZE <= BITS_SIZE<T>, "");
        WriteBitsInternal(v, SIZE);
    }

    template <typename T>
    void WriteBitsInternal(const T& v, size_t size) {
        auto value = v;
        auto bitsLeft = size;
        while (bitsLeft > 0) {
            auto bits = std::min(bitsLeft, BITS_SIZE<value_type>);
            m_scratch |= static_cast<SCRATCH_TYPE>( value ) << m_scratchBits;
            m_scratchBits += bits;
            if ( m_scratchBits >= BITS_SIZE<value_type> ) {
                auto tmp = static_cast<value_type>(m_scratch & bufTypeMask);
                directWrite(&tmp, 1);
                m_scratch >>= BITS_SIZE<value_type>;
                m_scratchBits -= BITS_SIZE<value_type>;

                value >>= BITS_SIZE<value_type>;
            }
            bitsLeft -= bits;
        }
    }

    void Flush() {
        if ( m_scratchBits )
        {
            auto tmp = static_cast<value_type>( m_scratch & bufTypeMask );
            directWrite(&tmp, 1);
            m_scratch >>= m_scratchBits;
            m_scratchBits -= m_scratchBits;
        }
    }


private:

    template <typename T>
    void directWrite(const T* v, size_t count) {
        const auto bytesSize = sizeof(T) * count;
        const auto pos = _buf.size();
        _buf.resize(pos + bytesSize);
        std::copy_n(reinterpret_cast<const value_type *>(v), bytesSize, _buf.data()+pos);
    }

    const value_type bufTypeMask = 0xFF;
    using SCRATCH_TYPE = typename BIGGER_TYPE<value_type>::type;
    std::vector<value_type>& _buf;
    std::back_insert_iterator<std::vector<value_type>> _outIt;
    SCRATCH_TYPE m_scratch{};
    size_t m_scratchBits{};


    //size_t _bufSize{};

};


#endif //PROJECT_TEMPLATE_BUFFER_WRITER_H
