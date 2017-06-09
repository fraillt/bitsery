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
using namespace testing;

TEST(SerializeSubstitution, WhenSubstitutedThenOnlyWriteIndexUsingMinRequiredBits) {
    int32_t v = 4849;
    int32_t res;
    constexpr size_t N = 3;
    std::array<int32_t,N> subsitution{485,4849,89};
    SerializationContext ctx;
    ctx.createSerializer().substitution<4>(v, subsitution);
    ctx.createDeserializer().substitution<4>(res, subsitution);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(1));

    SerializationContext ctx1;
    ctx1.createSerializer().substitution<4>(v, subsitution);
    auto des = ctx1.createDeserializer();
    des.range(res, {0, N + 1});
    EXPECT_THAT(res, Eq(2));
}

TEST(SerializeSubstitution, WhenNoSubstitutionThenWriteZeroBitsAndValueOrObject) {
    int16_t v = 8945;
    int16_t res;
    std::array<int16_t,3> subsitution{485,4849,89};
    SerializationContext ctx;
    ctx.createSerializer().substitution<2>(v, subsitution);
    ctx.createDeserializer().substitution<2>(res, subsitution);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(sizeof(int16_t)+1));
}

TEST(SerializeSubstitution, CustomTypeSubstituted) {
    MyStruct1 v = {12,10};
    MyStruct1 res;
    constexpr size_t N = 4;
    std::array<MyStruct1, N> subsitution = {
            MyStruct1{12,10}, MyStruct1{485, 454},
            MyStruct1{4849,89}, MyStruct1{0,1}};
    SerializationContext ctx;
    ctx.createSerializer().substitution(v, subsitution);
    ctx.createDeserializer().substitution(res, subsitution);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
}

TEST(SerializeSubstitution, CustomTypeNotSubstituted) {
    MyStruct1 v = {8945,4456};
    MyStruct1 res;
    constexpr size_t N = 4;
    std::array<MyStruct1, N> subsitution = {
            MyStruct1{12,10}, MyStruct1{485, 454},
            MyStruct1{4849,89}, MyStruct1{0,1}};
    SerializationContext ctx;
    ctx.createSerializer().substitution(v, subsitution);
    ctx.createDeserializer().substitution(res, subsitution);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(MyStruct1::SIZE + 1));
}

TEST(SerializeSubstitution, CustomFunctionNotSubstituted) {
    MyStruct1 v = {8945,4456};
    MyStruct1 res;
    constexpr size_t N = 4;
    std::array<MyStruct1, N> subsitution = {
            MyStruct1{12,10}, MyStruct1{485, 454},
            MyStruct1{4849,89}, MyStruct1{0,1}};

    auto rangeForValue = bitsery::RangeSpec<int>(0, 10000);
    auto rangeForIndex = bitsery::RangeSpec<size_t>{0, N+1};

    SerializationContext ctx;
    auto ser = ctx.createSerializer();

    //lambdas differ only in capture clauses, it would make sense to use std::bind, but debugger crashes when it sees std::bind...
    auto serLambda = [rangeForValue](auto& s, const MyStruct1& v) {
        s.range(v.i1, rangeForValue);
        s.range(v.i2, rangeForValue);
    };
    ser.substitution(v, subsitution, serLambda);

    auto des = ctx.createDeserializer();
    auto desLambda = [rangeForValue](auto& s, MyStruct1& v) {
        s.range(v.i1, rangeForValue);
        s.range(v.i2, rangeForValue);
    };
    des.substitution(res, subsitution, desLambda);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq((rangeForIndex.bitsRequired + rangeForValue.bitsRequired * 2 - 1) / 8 + 1 ));
}

TEST(SerializeSubstitution, WhenSubstitutedThenCustomFunctionNotInvoked) {
    MyStruct1 v = {4849,89};
    MyStruct1 res;
    constexpr size_t N = 4;
    std::array<MyStruct1, N> subsitution = {
            MyStruct1{12,10}, MyStruct1{485, 454},
            MyStruct1{4849,89}, MyStruct1{0,1}};

    SerializationContext ctx;
    ctx.createSerializer().substitution(v, subsitution, [](bitsery::Serializer<bitsery::BufferWriter>& ,const MyStruct1& ) {});
    ctx.createDeserializer().substitution(res, subsitution, [](bitsery::Deserializer<bitsery::BufferReader>&, MyStruct1& ) {});

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
}
