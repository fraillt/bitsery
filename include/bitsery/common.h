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


#ifndef BITSERY_COMMON_H
#define BITSERY_COMMON_H

#include <vector>

namespace bitsery {

/*
 * endianess
 */
    enum class EndiannessType {
        LittleEndian,
        BigEndian
    };

    //default configuration for buffer writing/reading operations
    struct DefaultConfig {
        static constexpr EndiannessType NetworkEndianness = EndiannessType::LittleEndian;
        //this functionality allows to support backward/forward compatibility for any type
        //disabling it, saves 100+bytes per BufferReader/Writer and also reduces executable size
        static constexpr bool BufferSessionsEnabled = true;
        //buffer value type must be unsigned, currently only uint8_t supported
        //fixed size buffer type also supported, for faster serialization performance
        using BufferType = std::vector<uint8_t>;

    };

}

#endif //BITSERY_COMMON_H
