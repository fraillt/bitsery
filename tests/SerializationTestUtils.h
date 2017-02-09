//
// Created by fraillt on 17.2.8.
//

#ifndef BITSERY_SERIALIZERTESTS_H
#define BITSERY_SERIALIZERTESTS_H

#include <gmock/gmock.h>
#include <Deserializer.h>
#include <BufferReader.h>
#include "BufferWriter.h"
#include "Serializer.h"

struct MyStruct1 {
    MyStruct1(int v1, int v2):i1{v1}, i2{v2} {}
    MyStruct1():MyStruct1{0,0} {}
    int i1;
    int i2;
    bool operator == (const MyStruct1& rhs) const {
        return i1 == rhs.i1 && i2 == rhs.i2;
    }
    static constexpr size_t SIZE = sizeof(i1) + sizeof(i2);
};

SERIALIZE(MyStruct1) {
    return s.
            value(o.i1).
            value(o.i2);
}

struct MyStruct2 {
    enum MyEnum {
        V1, V2, V3
    };

    MyStruct2(MyEnum e, MyStruct1 s):e1{e}, s1{s} {}
    MyStruct2():MyStruct2{V1,{0,0}} {}

    MyEnum e1;
    MyStruct1 s1;
    bool operator == (const MyStruct2& rhs) const {
        return e1 == rhs.e1 && s1 == rhs.s1;
    }
    static constexpr size_t SIZE = MyStruct1::SIZE + sizeof(e1);
};

SERIALIZE(MyStruct2) {
    return s.
            value(o.e1).
            object(o.s1);
}

class SerializationContext {
    std::vector<uint8_t> buf{};
    std::unique_ptr<BufferWriter> bw;
    std::unique_ptr<BufferReader> br;
public:
    Serializer<BufferWriter> createSerializer() {
        bw = std::make_unique<BufferWriter>(buf);
        return {*bw};
    };

    size_t getBufferSize() const {
        return buf.size();
    }
    //since all containers .size() method returns size_t, it cannot be dirrectly serialized, because size_t is platform dependant
    //this function returns number of bytes writen to buffer, when reading/writing size of container
    static size_t containerSizeSerializedBytesCount(size_t elemsCount) {
        return 4;
    }

    Deserializer<BufferReader> createDeserializer() {
        br = std::make_unique<BufferReader>(buf);
        return {*br};
    };
};

#endif //BITSERY_SERIALIZERTESTS_H
