//
// Created by fraillt on 17.1.5.
//

#include <gmock/gmock.h>
#include "SerializationTestUtils.h"

using testing::Eq;

template <typename T>
bool SerializeDeserializeValue(const T& v) {
    T res{};
    SerializationContext ctx;
    ctx.createSerializer().value(v);
    ctx.createDeserializer().value(res);
    return v == res;
}

TEST(SerializeValues, IntegerTypes) {
    EXPECT_THAT(SerializeDeserializeValue(-449874), Eq(true));
    EXPECT_THAT(SerializeDeserializeValue(34u), Eq(true));
}

TEST(SerializeValues, EnumTypes) {
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

TEST(SerializeValues, FloatingPointTypes) {
    EXPECT_THAT(SerializeDeserializeValue(-484.465), Eq(true));
    EXPECT_THAT(SerializeDeserializeValue(0.00000015f), Eq(true));
}

TEST(SerializeValues, ExplicitTypeSize) {
    int v{23472};
    int res;
    constexpr size_t TSIZE = sizeof(v);

    SerializationContext ctx;
    ctx.createSerializer().value<TSIZE>(v);
    ctx.createDeserializer().value<TSIZE>(res);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(TSIZE, Eq(ctx.getBufferSize()));
}

