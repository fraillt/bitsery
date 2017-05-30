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


#include <gmock/gmock.h>
#include "serialization_test_utils.h"
#include <type_traits>

using testing::ContainerEq;
using testing::Eq;

TEST(SerializeFSArrayStdArray, ArithmeticValues) {
    SerializationContext ctx;
    std::array<int, 4> src{5,9,15,-459};
    std::array<int, 4> res{};

    ctx.createSerializer().array<sizeof(int)>(src);
    ctx.createDeserializer().array<sizeof(int)>(res);

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
        ser.object(v).value1(tmp);
    });
    auto des = ctx.createDeserializer();
    des.array(res, [&des](auto& v) {
        char tmp{};
        des.object(v).value1(tmp);
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
        ser.object(v).value1(tmp);
    });
    auto des = ctx.createDeserializer();
    des.array(res, [&des](auto& v) {
        char tmp{};
        des.object(v).value1(tmp);
    });

    EXPECT_THAT(ctx.getBufferSize(), Eq(std::extent<decltype(src)>::value * (MyStruct1::SIZE + sizeof(char))));
    EXPECT_THAT(res, ContainerEq(src));
}
