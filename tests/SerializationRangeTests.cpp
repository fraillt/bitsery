//
// Created by fraillt on 17.2.15.
//

#include <gmock/gmock.h>
#include "SerializationTestUtils.h"
using namespace testing;

TEST(Ranges, IntegralRanges) {
    constexpr RangeSpec<int> r1{0, 31};
    static_assert(r1.bitsRequired() == 5);
    EXPECT_TRUE(r1.isValid(0));
    EXPECT_TRUE(r1.isValid(15));
    EXPECT_TRUE(r1.isValid(31));
    EXPECT_FALSE(r1.isValid(-1));
    EXPECT_FALSE(r1.isValid(32));

    constexpr RangeSpec<MyEnumClass> r2{MyEnumClass::E1, MyEnumClass::E4};
    EXPECT_TRUE(r2.isValid(MyEnumClass::E2));
    EXPECT_FALSE(r2.isValid(MyEnumClass::E5));

    int x= 0;
    RangeSpec<int> r3{x,3};
    EXPECT_THAT(r3.bitsRequired(), Eq(2));


    SerializationContext ctx;
//    ctx.createSerializer().range(486, {0,900});
//    ctx.createSerializer().range(MyEnumClass::E4, {MyEnumClass::E1,MyEnumClass::E6});
//    ctx.createSerializer().range(4.5f, {0.0f,10.0f, 10});
//    ctx.createSerializer().range(4.5f, {0.0f,10.0f, 0.001f});
}