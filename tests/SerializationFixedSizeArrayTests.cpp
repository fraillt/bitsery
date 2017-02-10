//
// Created by fraillt on 17.2.9.
//

#include <gmock/gmock.h>
#include "SerializationTestUtils.h"
#include <type_traits>

using testing::ContainerEq;
using testing::Eq;

TEST(SerializeFSArrayStdArray, ArithmeticValues) {
    SerializationContext ctx;
    std::array<int, 4> src{5,9,15,-459};
    std::array<int, 4> res{};

    ctx.createSerializer().array(src);
    ctx.createDeserializer().array(res);

    EXPECT_THAT(ctx.getBufferSize(), Eq(src.size() * sizeof(int)));
    EXPECT_THAT(res, ContainerEq(src));

}

TEST(SerializeFSArrayStdArray, ArithmeticValuesSettingValueSizeExplicitly) {
    SerializationContext ctx;
    std::array<int, 4> src{5,9,15,-459};
    std::array<int, 4> res{};

    ctx.createSerializer().array<sizeof(int)>(src);
    ctx.createDeserializer().array<sizeof(int)>(res);

    EXPECT_THAT(ctx.getBufferSize(), Eq(src.size() * sizeof(int)));
    EXPECT_THAT(res, ContainerEq(src));

}

TEST(SerializeFSArrayStdArray, CompositeTypes) {
    SerializationContext ctx;
    std::array<MyStruct1, 7> src{
            MyStruct1{0,1}, MyStruct1{2,3}, MyStruct1{4,5}, MyStruct1{6,7},
            MyStruct1{8,9}, MyStruct1{11,34}, MyStruct1{5134,1532}};
    std::array<MyStruct1, 7> res{};

    ctx.createSerializer().array(src);
    ctx.createDeserializer().array(res);

    EXPECT_THAT(ctx.getBufferSize(), Eq(src.size() * MyStruct1::SIZE));
    EXPECT_THAT(res, ContainerEq(src));
}
//
//
TEST(SerializeFSArrayStdArray, CustomFunctionThatSerializesAnEmptyByteEveryElement) {
    SerializationContext ctx;
    std::array<MyStruct1, 7> src{
            MyStruct1{0,1}, MyStruct1{2,3}, MyStruct1{4,5}, MyStruct1{6,7},
            MyStruct1{8,9}, MyStruct1{11,34}, MyStruct1{5134,1532}};
    std::array<MyStruct1, 7> res{};


    auto ser = ctx.createSerializer();
    ser.array(src, [&ser](auto& v) {
        char tmp{};
        ser.object(v).value(tmp);
    });
    auto des = ctx.createDeserializer();
    des.array(res, [&des](auto& v) {
        char tmp{};
        des.object(v).value(tmp);
    });

    EXPECT_THAT(ctx.getBufferSize(), Eq(src.size() * (MyStruct1::SIZE + sizeof(char))));
    EXPECT_THAT(res, ContainerEq(src));
}


TEST(SerializeFSArrayCArray, ArithmeticValues) {
    SerializationContext ctx;
    int src[4]{5,9,15,-459};
    int res[4]{};

    ctx.createSerializer().array(src);
    ctx.createDeserializer().array(res);

    EXPECT_THAT(ctx.getBufferSize(), Eq(std::extent<decltype(src)>::value * sizeof(int)));
    EXPECT_THAT(res, ContainerEq(src));

}

TEST(SerializeFSArrayCArray, ArithmeticValuesSettingValueSizeExplicitly) {
    SerializationContext ctx;
    int src[4]{5,9,15,-459};
    int res[4]{};

    ctx.createSerializer().array<sizeof(int)>(src);
    ctx.createDeserializer().array<sizeof(int)>(res);

    EXPECT_THAT(ctx.getBufferSize(), Eq(std::extent<decltype(src)>::value * sizeof(int)));
    EXPECT_THAT(res, ContainerEq(src));

}

TEST(SerializeFSArrayCArray, CompositeTypes) {
    SerializationContext ctx;
    MyStruct1 src[]{
            MyStruct1{0,1}, MyStruct1{2,3}, MyStruct1{4,5}, MyStruct1{6,7},
            MyStruct1{8,9}, MyStruct1{11,34}, MyStruct1{5134,1532}};
    MyStruct1 res[7]{};

    ctx.createSerializer().array(src);
    ctx.createDeserializer().array(res);

    EXPECT_THAT(ctx.getBufferSize(), Eq(std::extent<decltype(src)>::value * MyStruct1::SIZE));
    EXPECT_THAT(res, ContainerEq(src));
}
//
//
TEST(SerializeFSArrayCArray, CustomFunctionThatSerializesAnEmptyByteEveryElement) {
    SerializationContext ctx;
    MyStruct1 src[]{
            MyStruct1{0,1}, MyStruct1{2,3}, MyStruct1{4,5}, MyStruct1{6,7},
            MyStruct1{8,9}, MyStruct1{11,34}, MyStruct1{5134,1532}};
    MyStruct1 res[7]{};


    auto ser = ctx.createSerializer();
    ser.array(src, [&ser](auto& v) {
        char tmp{};
        ser.object(v).value(tmp);
    });
    auto des = ctx.createDeserializer();
    des.array(res, [&des](auto& v) {
        char tmp{};
        des.object(v).value(tmp);
    });

    EXPECT_THAT(ctx.getBufferSize(), Eq(std::extent<decltype(src)>::value * (MyStruct1::SIZE + sizeof(char))));
    EXPECT_THAT(res, ContainerEq(src));
}
