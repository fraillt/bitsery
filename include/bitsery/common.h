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

#include <tuple>

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
        //data will be stored in little endian, independant of host.
        static constexpr EndiannessType NetworkEndianness = EndiannessType::LittleEndian;
        //this functionality allows to support backward/forward compatibility
        //however reading from streams is not supported, because this functionality requires random access to buffer.
        static constexpr bool BufferSessionsEnabled = false;
        //list of contexts that will be instanciated internally within serializer/deserializer.
        //contexts must be default constructable.
        //internal context has priority, if external context with the same type exists.
        using InternalContext = std::tuple<>;
    };

}

#endif //BITSERY_COMMON_H
