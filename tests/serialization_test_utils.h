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


#ifndef BITSERY_SERIALIZERTESTS_H
#define BITSERY_SERIALIZERTESTS_H

#include <bitsery/bitsery.h>
#include <memory>

struct MyStruct1 {
    MyStruct1(int v1, int v2):i1{v1}, i2{v2} {}
    MyStruct1():MyStruct1{0,0} {}
    int i1;
    int i2;
    bool operator == (const MyStruct1& rhs) const {
        return i1 == rhs.i1 && i2 == rhs.i2;
    }
    static constexpr size_t SIZE = sizeof(MyStruct1::i1) + sizeof(MyStruct1::i2);
};

SERIALIZE(MyStruct1) {
    return s.
            value(o.i1).
            value(o.i2);
}

enum class MyEnumClass {
    E1, E2, E3, E4, E5, E6
};

struct MyStruct2 {
    enum MyEnum {
        V1, V2, V3, V4, V5, V6
    };

    MyStruct2(MyEnum e, MyStruct1 s):e1{e}, s1{s} {}
    MyStruct2():MyStruct2{V1,{0,0}} {}

    MyEnum e1;
    MyStruct1 s1;
    bool operator == (const MyStruct2& rhs) const {
        return e1 == rhs.e1 && s1 == rhs.s1;
    }
    static constexpr size_t SIZE = MyStruct1::SIZE + sizeof(MyStruct2::e1);
};

SERIALIZE(MyStruct2) {
    return s.
            value(o.e1).
            object(o.s1);
}

class SerializationContext {
    std::vector<uint8_t> buf{};
    std::unique_ptr<bitsery::BufferWriter> bw;
    std::unique_ptr<bitsery::BufferReader> br;
public:
    bitsery::Serializer<bitsery::BufferWriter> createSerializer() {
        bw = std::make_unique<bitsery::BufferWriter>(buf);
        return {*bw};
    };

    size_t getBufferSize() const {
        return buf.size();
    }
    //since all containers .size() method returns size_t, it cannot be dirrectly serialized, because size_t is platform dependant
    //this function returns number of bytes writen to buffer, when reading/writing size of container
    static size_t containerSizeSerializedBytesCount(size_t elemsCount) {
        if (elemsCount < 0x80u)
            return 1;
        if (elemsCount < 0x4000u)
            return 2;
        return 4;
    }

    bitsery::Deserializer<bitsery::BufferReader> createDeserializer() {
        bw->flush();
        br = std::make_unique<bitsery::BufferReader>(buf);
        return {*br};
    };
};

#endif //BITSERY_SERIALIZERTESTS_H
