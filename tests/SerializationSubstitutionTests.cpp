//
// Created by fraillt on 17.2.20.
//

#include <gmock/gmock.h>
#include "SerializationTestUtils.h"
using namespace testing;

TEST(SerializeSubstitution, WhenSubstitutedThenOnlyWriteIndexUsingMinRequiredBits) {
    int v = 4849;
    int res;
    constexpr size_t N = 3;
    std::array<int,N> subsitution{485,4849,89};
    SerializationContext ctx;
    ctx.createSerializer().substitution(v, subsitution);
    ctx.createDeserializer().substitution(res, subsitution);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(1));

    SerializationContext ctx1;
    ctx1.createSerializer().substitution(v, subsitution);
    auto des = ctx1.createDeserializer();
    des.range(res, {0, N + 1});
    EXPECT_THAT(res, Eq(2));
}

TEST(SerializeSubstitution, WhenNoSubstitutionThenWriteZeroBitsAndValueOrObject) {
    int v = 8945;
    int res;
    std::array<int,3> subsitution{485,4849,89};
    SerializationContext ctx;
    ctx.createSerializer().substitution(v, subsitution);
    ctx.createDeserializer().substitution(res, subsitution);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(sizeof(int)+1));
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

TEST(SerializeSubstitution, ArithmeticTypeWithExplicitSizeNotSubstituted) {
    MyEnumClass v = MyEnumClass::E5;
    MyEnumClass res;
    constexpr size_t N = 3;
    std::array<MyEnumClass,N> subsitution{MyEnumClass::E1,MyEnumClass::E2,MyEnumClass::E3};
    SerializationContext ctx;
    ctx.createSerializer().substitution<sizeof(MyEnumClass)>(v, subsitution);
    ctx.createDeserializer().substitution<sizeof(MyEnumClass)>(res, subsitution);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(sizeof(MyEnumClass) + 1));
}

TEST(SerializeSubstitution, ArithmeticTypeWithExplicitSizeSubstituted) {
    MyEnumClass v = MyEnumClass::E1;
    MyEnumClass res;
    constexpr size_t N = 3;
    std::array<MyEnumClass,N> subsitution{MyEnumClass::E1,MyEnumClass::E2,MyEnumClass::E3};
    SerializationContext ctx;
    ctx.createSerializer().substitution<sizeof(MyEnumClass)>(v, subsitution);
    ctx.createDeserializer().substitution<sizeof(MyEnumClass)>(res, subsitution);

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
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
    auto serLambda = [&ser, rangeForValue](const MyStruct1& v) {
        ser.range(v.i1, rangeForValue);
        ser.range(v.i2, rangeForValue);
    };
    ser.substitution(v, subsitution, serLambda);

    auto des = ctx.createDeserializer();
    auto desLambda = [&des, rangeForValue](MyStruct1& v) {
        des.range(v.i1, rangeForValue);
        des.range(v.i2, rangeForValue);
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
    ctx.createSerializer().substitution(v, subsitution, [](const MyStruct1& ) {});
    ctx.createDeserializer().substitution(res, subsitution, [](MyStruct1& ) {});

    EXPECT_THAT(res, Eq(v));
    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
}
