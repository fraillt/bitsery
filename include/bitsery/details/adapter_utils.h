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

#ifndef BITSERY_DETAILS_ADAPTER_UTILS_H
#define BITSERY_DETAILS_ADAPTER_UTILS_H

#include <cassert>
#include <cstdint>
#include <cstddef>

namespace bitsery {

    enum class ReaderError {
        NoError,
        ReadingError, // this might be used with stream adapter
        DataOverflow,
        InvalidData,
        InvalidPointer
    };

    namespace details {
/*
 * size read/write functions
 */
        template <typename Reader>
        void readSize(Reader& r, size_t& size, size_t maxSize) {
            uint8_t hb{};
            r.template readBytes<1>(hb);
            if (hb < 0x80u) {
                size = hb;
            } else {
                uint8_t lb{};
                r.template readBytes<1>(lb);
                if (hb & 0x40u) {
                    uint16_t lw{};
                    r.template readBytes<2>(lw);
                    size = ((((hb & 0x3Fu) << 8) | lb) << 16) | lw;
                } else {
                    size = ((hb & 0x7Fu) << 8) | lb;
                }
            }
            if (size > maxSize) {
                r.setError(ReaderError::InvalidData);
                size = {};
            }
        }

        template <typename Writter>
        void writeSize(Writter& w, const size_t size) {
            if (size < 0x80u) {
                w.template writeBytes<1>(static_cast<uint8_t>(size));
            } else {
                if (size < 0x4000u) {
                    w.template writeBytes<1>(static_cast<uint8_t>((size >> 8) | 0x80u));
                    w.template writeBytes<1>(static_cast<uint8_t>(size));
                } else {
                    assert(size < 0x40000000u);
                    w.template writeBytes<1>(static_cast<uint8_t>((size >> 24) | 0xC0u));
                    w.template writeBytes<1>(static_cast<uint8_t>(size >> 16));
                    w.template writeBytes<2>(static_cast<uint16_t>(size));
                }
            }
        }

    }
}

#endif //BITSERY_DETAILS_ADAPTER_UTILS_H
