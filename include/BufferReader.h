//
// Created by Mindaugas Vinkelis on 2016-11-11.
//

#ifndef PROJECT_TEMPLATE_BUFFER_READER_H
#define PROJECT_TEMPLATE_BUFFER_READER_H

#include "Common.h"

#include <algorithm>

struct BufferReader {

    using value_type = uint8_t;
    BufferReader(const std::vector<uint8_t>& buf):_buf{buf}, _pos{std::begin(buf)}{

    }

    template<size_t SIZE, typename T>
    bool ReadBytes(T& v) {
        static_assert(std::is_integral<T>(), "");
        static_assert(sizeof(T) == SIZE, "");
        using UT = typename std::make_unsigned<T>::type;
        return m_scratch
               ? ReadBits<SIZE * 8>(reinterpret_cast<UT&>(v))
               : directRead(&v, 1);
    }

    template<size_t SIZE, typename T>
    bool ReadBuffer(T* buf, size_t count) {
        static_assert(std::is_integral<T>(), "");
        static_assert(sizeof(T) == SIZE, "");

        if (m_scratchBits) {
            //todo implement
//            using UT = typename std::make_unsigned<T>::type;
//            WriteBits<SIZE * 8 * count>(reinterpret_cast<const UT&>(v));
        } else {
            return directRead(buf, count);
        }
        return true;
    }


    template<size_t SIZE, typename T>
    bool ReadBits(T& v) {
        static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
        static_assert(SIZE > 0 && SIZE <= BITS_SIZE<T>, "");

        const auto bytesRequired = SIZE > m_scratchBits
                              ? ((SIZE - 1 - m_scratchBits) >> 3) + 1u
                              : 0u;
        if (static_cast<size_t>(std::distance(_pos, std::end(_buf))) < bytesRequired )
            return false;
        ReadBitsInternal(v, SIZE);
        return true;
    }

    template <typename T>
    void ReadBitsInternal(T& v, size_t size) {
        auto bitsLeft = size;
        T res{};
        while (bitsLeft > 0) {
            auto bits = std::min(bitsLeft, BITS_SIZE<value_type>);
            if ( m_scratchBits < bits ) {
                value_type tmp;

                std::copy_n(_pos, 1, reinterpret_cast<value_type *>(&tmp));
                std::advance(_pos, 1);

                m_scratch |= static_cast<SCRATCH_TYPE>(tmp) << m_scratchBits;
                m_scratchBits += BITS_SIZE<value_type>;
            }
            auto shiftedRes = static_cast<T>(m_scratch & ( (static_cast<SCRATCH_TYPE>(1)<<bits) - 1 )) << (size - bitsLeft);
            res |=  shiftedRes;
            m_scratch >>= bits;
            m_scratchBits -= bits;
            bitsLeft -= bits;
        }
        v = res;
    }

    bool isCompleted() const {
        return _pos == std::end(_buf);
    }

private:
    const std::vector<value_type>& _buf;
    decltype(std::begin(_buf)) _pos;
    template <typename T>
    bool directRead(T* v, size_t count) {
		static_assert(!std::is_const<T>::value, "");
        const auto bytesCount = sizeof(T) * count;
        if (static_cast<size_t>(std::distance(_pos, std::end(_buf))) < bytesCount)
            return false;
        std::copy_n(_pos, bytesCount, reinterpret_cast<value_type *>(v));
        std::advance(_pos, bytesCount);
        return true;
    }


    using SCRATCH_TYPE = typename BIGGER_TYPE<value_type>::type;

    SCRATCH_TYPE m_scratch{};
    size_t m_scratchBits{};									///< Number of bits currently in the scratch buffer. If the user wants to read more bits than this, we have to go fetch another dword from memory.

};



#endif //PROJECT_TEMPLATE_BUFFER_READER_H
