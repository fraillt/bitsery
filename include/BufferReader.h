//
// Created by Mindaugas Vinkelis on 2016-11-11.
//

#ifndef PROJECT_TEMPLATE_BUFFER_READER_H
#define PROJECT_TEMPLATE_BUFFER_READER_H

#include "Common.h"

#include <cassert>
#include <algorithm>

struct BufferReader {

    using value_type = uint8_t;
    BufferReader(const std::vector<uint8_t>& buf):_buf{buf}, _pos{std::begin(buf)}{

    }

    template<size_t SIZE, typename T>
    bool readBytes(T& v) {
        static_assert(std::is_integral<T>(), "");
        static_assert(sizeof(T) == SIZE, "");
        using UT = typename std::make_unsigned<T>::type;
        return !m_scratch
               ? directRead(&v, 1)
               : readBits(reinterpret_cast<UT&>(v), BITS_SIZE<T>);
    }

    template<size_t SIZE, typename T>
    bool readBuffer(T* buf, size_t count) {
        static_assert(std::is_integral<T>(), "");
        static_assert(sizeof(T) == SIZE, "");

        if (!m_scratchBits) {
            return directRead(buf, count);
        } else {
            using UT = typename std::make_unsigned<T>::type;
            //todo improve implementation
            const auto end = buf + count;
            for (auto it = buf; it != end; ++it) {
                if (!readBits(reinterpret_cast<UT&>(*it), BITS_SIZE<T>))
                    return false;
            }
        }
        return true;
    }


    template<typename T>
    bool readBits(T& v, size_t bitsCount) {
        static_assert(std::is_integral<T>() && std::is_unsigned<T>(), "");
        assert(bitsCount <= BITS_SIZE<T>);

        const auto bytesRequired = bitsCount > m_scratchBits
                              ? ((bitsCount - 1 - m_scratchBits) >> 3) + 1u
                              : 0u;
        if (static_cast<size_t>(std::distance(_pos, std::end(_buf))) < bytesRequired )
            return false;
        readBitsInternal(v, bitsCount);
        return true;
    }

    bool align() {
        if ( m_scratchBits ) {
            SCRATCH_TYPE tmp{};
            readBitsInternal(tmp, BITS_SIZE<value_type> - m_scratchBits);
            return tmp == 0;
        }
        return true;
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

    template <typename T>
    void readBitsInternal(T& v, size_t size) {
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

    using SCRATCH_TYPE = typename BIGGER_TYPE<value_type>::type;

    SCRATCH_TYPE m_scratch{};
    size_t m_scratchBits{};									///< Number of bits currently in the scratch buffer. If the user wants to read more bits than this, we have to go fetch another dword from memory.

};



#endif //PROJECT_TEMPLATE_BUFFER_READER_H
