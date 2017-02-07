//
// Created by fraillt on 17.1.5.
//

#include <gmock/gmock.h>
#include <Deserializer.h>
#include <BufferReader.h>
#include "BufferWriter.h"
#include "Serializer.h"


using testing::Eq;

template <typename T>
bool SerializeDeserializeValue(const T& v) {
    T res{};
    std::vector<uint8_t> buf{};

    BufferWriter bw{buf};
    Serializer<BufferWriter> ser(bw);
    ser.value(v);
    bw.Flush();

    BufferReader br{buf};
    Deserializer<BufferReader> des(br);
    des.value(res);
    return v == res;
}

TEST(SerializerValues, IntegerTypes) {
    EXPECT_THAT(SerializeDeserializeValue(-449874), Eq(true));
    EXPECT_THAT(SerializeDeserializeValue(34u), Eq(true));
}

TEST(SerializerValues, EnumTypes) {
    enum E1{
        A1,B1,C1,D1
    };
    EXPECT_THAT(SerializeDeserializeValue(E1::C1), Eq(true));
    enum class E2 {
        A2,B2,C2,D2
    };
    EXPECT_THAT(SerializeDeserializeValue(E2::B2), Eq(true));
    enum class E3:short {
        A3, B3, C3=4568, D3
    };
    EXPECT_THAT(SerializeDeserializeValue(E3::C3), Eq(true));
}


TEST(SerializerValues, FloatingPointTypes) {
    EXPECT_THAT(SerializeDeserializeValue(-484.465), Eq(true));
    EXPECT_THAT(SerializeDeserializeValue(0.00000015f), Eq(true));
}

