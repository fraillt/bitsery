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
#include "SerializationTestUtils.h"
using namespace testing;
using bitsery::RangeSpec;
using bitsery::BitsConstraint;

TEST(SerializeRange, RequiredBitsIsConstexpr) {
    constexpr RangeSpec<int> r1{0, 31};
    static_assert(r1.bitsRequired == 5);

    constexpr RangeSpec<MyEnumClass> r2{MyEnumClass::E1, MyEnumClass::E4};
    static_assert(r2.bitsRequired == 2);

    constexpr RangeSpec<double> r3{-1.0,1.0, BitsConstraint{5u}};
    //EXPECT_THAT(r1.bitsRequired, Eq(5));
    static_assert(r3.bitsRequired == 5);

    constexpr RangeSpec<float> r4{-1.0f,1.0f, 0.01f};
    static_assert(r4.bitsRequired == 8);

}

TEST(SerializeRange, IntegerNegative) {
    SerializationContext ctx;
    constexpr RangeSpec<int> r1{-50, 50};
    int t1{-8};
    int res1;

    ctx.createSerializer().range(t1, r1);
    ctx.createDeserializer().range(res1, r1);

    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
    EXPECT_THAT(res1, Eq(t1));

}

TEST(SerializeRange, IntegerPositive) {
    SerializationContext ctx;
    constexpr RangeSpec<unsigned> r1{4, 10};
    unsigned t1{8};
    unsigned res1;

    ctx.createSerializer().range(t1, r1);
    ctx.createDeserializer().range(res1, r1);

    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
    EXPECT_THAT(res1, Eq(t1));

}

TEST(SerializeRange, EnumTypes) {
    SerializationContext ctx;
    constexpr RangeSpec<MyEnumClass> r1{MyEnumClass::E2, MyEnumClass::E4};
    MyEnumClass t1{MyEnumClass::E2};
    MyEnumClass res1;

    ctx.createSerializer().range(t1, r1);
    ctx.createDeserializer().range(res1, r1);

    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
    EXPECT_THAT(res1, Eq(t1));

}

TEST(SerializeRange, FloatUsingPrecisionConstraint1) {
    SerializationContext ctx;
    constexpr float precision{0.01f};
    constexpr float min{-1.0f};
    constexpr float max{1.0f};
    float t1{0.5f};
    constexpr RangeSpec<float> r1{min, max, precision};

    float res1;

    ctx.createSerializer().range(t1, r1);
    ctx.createDeserializer().range(res1, r1);

    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
    EXPECT_THAT(res1, ::testing::FloatNear(t1, (max - min) * precision));
}

TEST(SerializeRange, DoubleUsingPrecisionConstraint2) {
    SerializationContext ctx;
    constexpr double precision{0.000002};
    constexpr double min{50.0};
    constexpr double max{100000.0};
    double t1{38741.0};
    constexpr RangeSpec<double> r1{min, max, precision};

    double res1;

    ctx.createSerializer().range(t1, r1);
    ctx.createDeserializer().range(res1, r1);

    EXPECT_THAT(ctx.getBufferSize(), Eq(5));
    EXPECT_THAT(res1, ::testing::DoubleNear(t1, (max - min) * precision));
}

TEST(SerializeRange, FloatUsingBitsSizeConstraint1) {
    SerializationContext ctx;
    constexpr size_t bits = 8;
    constexpr float min{-1.0f};
    constexpr float max{1.0f};
    float t1{0.5f};
    constexpr RangeSpec<float> r1{min, max, BitsConstraint(bits)};

    float res1;

    ctx.createSerializer().range(t1, r1);
    ctx.createDeserializer().range(res1, r1);

    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
    EXPECT_THAT(res1, ::testing::FloatNear(t1, (max - min) / (static_cast<bitsery::SAME_SIZE_UNSIGNED<float>>(1) << bits)));
}

TEST(SerializeRange, DoubleUsingBitsSizeConstraint2) {
    SerializationContext ctx;
    constexpr size_t bits = 50;
    constexpr double min{50.0};
    constexpr double max{100000.0};
    double t1{38741};
    constexpr RangeSpec<double> r1{min, max, BitsConstraint(bits)};

    double res1;

    ctx.createSerializer().range(t1, r1);
    ctx.createDeserializer().range(res1, r1);

    EXPECT_THAT(ctx.getBufferSize(), Eq(7));
    EXPECT_THAT(res1, ::testing::DoubleNear(t1, (max - min) / (static_cast<bitsery::SAME_SIZE_UNSIGNED<double>>(1) << bits)));
}
