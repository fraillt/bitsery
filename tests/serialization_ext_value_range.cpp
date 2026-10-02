// MIT License
//
// Copyright (c) 2017 Mindaugas Vinkelis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include "serialization_test_utils.h"
#include <bitsery/ext/value_range.h>
#include <gmock/gmock.h>
#include <limits>

using namespace testing;
using bitsery::details::RangeSpec;
using bitsery::ext::BitsConstraint;
using bitsery::ext::ValueRange;

#if __cplusplus > 201103L

TEST(SerializeExtensionValueRange, RequiredBitsIsConstexpr)
{
  constexpr RangeSpec<int> r1{ 0, 31 };
  static_assert(r1.bitsRequired == 5, "r1.bitsRequired == 5");

  constexpr RangeSpec<MyEnumClass> r2{ MyEnumClass::E1, MyEnumClass::E4 };
  static_assert(r2.bitsRequired == 2, "r2.bitsRequired == 2");

  constexpr RangeSpec<double> r3{ -1.0, 1.0, BitsConstraint{ 5u } };
  // EXPECT_THAT(r1.bitsRequired, Eq(5));
  static_assert(r3.bitsRequired == 5, "r3.bitsRequired == 5");

  constexpr RangeSpec<float> r4{ -1.0f, 1.0f, 0.01f };
  static_assert(r4.bitsRequired == 8, "r4.bitsRequired == 8");
}

#endif

using BPSer = SerializationContext::TSerializerBPEnabled;
using BPDes = SerializationContext::TDeserializerBPEnabled;

TEST(SerializeExtensionValueRange, IntegerNegative)
{
  SerializationContext ctx;
  ValueRange<int> r1{ -50, 50 };
  int t1{ -8 };
  int res1;
  ctx.createSerializer().enableBitPacking(
    [&t1, &r1](BPSer& ser) { ser.ext(t1, r1); });
  ctx.createDeserializer().enableBitPacking(
    [&res1, &r1](BPDes& des) { des.ext(res1, r1); });

  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
  EXPECT_THAT(res1, Eq(t1));
}

TEST(SerializeExtensionValueRange, IntegerPositive)
{
  SerializationContext ctx;
  ValueRange<unsigned> r1{ 4u, 10u };
  unsigned t1{ 8 };
  unsigned res1;

  ctx.createSerializer().enableBitPacking(
    [&t1, &r1](BPSer& ser) { ser.ext(t1, r1); });
  ctx.createDeserializer().enableBitPacking(
    [&res1, &r1](BPDes& des) { des.ext(res1, r1); });

  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
  EXPECT_THAT(res1, Eq(t1));
}

TEST(SerializeExtensionValueRange, EnumTypes)
{
  SerializationContext ctx;
  ValueRange<MyEnumClass> r1{ MyEnumClass::E2, MyEnumClass::E4 };
  MyEnumClass t1{ MyEnumClass::E2 };
  MyEnumClass res1;

  ctx.createSerializer().enableBitPacking(
    [&t1, &r1](BPSer& ser) { ser.ext(t1, r1); });
  ctx.createDeserializer().enableBitPacking(
    [&res1, &r1](BPDes& des) { des.ext(res1, r1); });

  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
  EXPECT_THAT(res1, Eq(t1));
}

TEST(SerializeExtensionValueRange, FloatUsingPrecisionConstraint1)
{
  SerializationContext ctx;
  constexpr float precision{ 0.01f };
  constexpr float min{ -1.0f };
  constexpr float max{ 1.0f };
  float t1{ 0.5f };
  ValueRange<float> r1{ min, max, precision };

  float res1;

  ctx.createSerializer().enableBitPacking(
    [&t1, &r1](BPSer& ser) { ser.ext(t1, r1); });
  ctx.createDeserializer().enableBitPacking(
    [&res1, &r1](BPDes& des) { des.ext(res1, r1); });

  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
  EXPECT_THAT(res1, ::testing::FloatNear(t1, (max - min) * precision));
}

TEST(SerializeExtensionValueRange, DoubleUsingPrecisionConstraint2)
{
  SerializationContext ctx;
  constexpr double precision{ 0.000002 };
  constexpr double min{ 50.0 };
  constexpr double max{ 100000.0 };
  double t1{ 38741.0 };
  ValueRange<double> r1{ min, max, precision };

  double res1;

  ctx.createSerializer().enableBitPacking(
    [&t1, &r1](BPSer& ser) { ser.ext(t1, r1); });
  ctx.createDeserializer().enableBitPacking(
    [&res1, &r1](BPDes& des) { des.ext(res1, r1); });

  EXPECT_THAT(ctx.getBufferSize(), Eq(5));
  EXPECT_THAT(res1, ::testing::DoubleNear(t1, (max - min) * precision));
}

TEST(SerializeExtensionValueRange, FloatUsingBitsSizeConstraint1)
{
  SerializationContext ctx;
  constexpr size_t bits = 8;
  constexpr float min{ -1.0f };
  constexpr float max{ 1.0f };
  float t1{ 0.5f };
  ValueRange<float> r1{ min, max, BitsConstraint(bits) };

  float res1;

  ctx.createSerializer().enableBitPacking(
    [&t1, &r1](BPSer& ser) { ser.ext(t1, r1); });
  ctx.createDeserializer().enableBitPacking(
    [&res1, &r1](BPDes& des) { des.ext(res1, r1); });

  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
  EXPECT_THAT(
    res1,
    ::testing::FloatNear(
      t1,
      (max - min) /
        (static_cast<bitsery::details::SameSizeUnsigned<float>>(1) << bits)));
}

TEST(SerializeExtensionValueRange, DoubleUsingBitsSizeConstraint2)
{
  SerializationContext ctx;
  constexpr size_t bits = 50;
  constexpr double min{ 50.0 };
  constexpr double max{ 100000.0 };
  double t1{ 38741 };
  ValueRange<double> r1{ min, max, BitsConstraint(bits) };

  double res1;

  ctx.createSerializer().enableBitPacking(
    [&t1, &r1](BPSer& ser) { ser.ext(t1, r1); });
  ctx.createDeserializer().enableBitPacking(
    [&res1, &r1](BPDes& des) { des.ext(res1, r1); });

  EXPECT_THAT(ctx.getBufferSize(), Eq(7));
  EXPECT_THAT(
    res1,
    ::testing::DoubleNear(
      t1,
      (max - min) /
        (static_cast<bitsery::details::SameSizeUnsigned<double>>(1) << bits)));
}

TEST(SerializeExtensionValueRange, WhenDataIsInvalidThenReturnMinimumRangeValue)
{
  SerializationContext ctx;
  ValueRange<int> r1{ 4, 10 }; // 6 is max, but 3bits required
  int res1;
  uint8_t tmp{ 0xFF }; // write all 1 so when reading 3 bits we get 7

  ctx.createSerializer().enableBitPacking(
    [&tmp](BPSer& ser) { ser.value1b(tmp); });
  ctx.createDeserializer().enableBitPacking(
    [&res1, &r1](BPDes& des) { des.ext(res1, r1); });

  EXPECT_THAT(ctx.getBufferSize(), Eq(1));
  EXPECT_THAT(res1, Eq(4));
  EXPECT_THAT(ctx.des->adapter().error(),
              Eq(bitsery::ReaderError::InvalidData));
}

template<typename T>
class FloatingPointValueRange : public testing::Test
{};

using FloatingPointTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(FloatingPointValueRange, FloatingPointTypes, );

TYPED_TEST(FloatingPointValueRange, MaximumDoesNotStopFollowingReads)
{
  SerializationContext ctx;
  const TypeParam min = static_cast<TypeParam>(-0.2);
  const TypeParam max = static_cast<TypeParam>(0.1);
  const ValueRange<TypeParam> range{ min, max,
                                    static_cast<TypeParam>(0.0005) };
  const uint32_t following{ 0x12345678 };
  TypeParam result{};
  uint32_t followingResult{};

  ctx.createSerializer().enableBitPacking([&](BPSer& ser) {
    ser.ext(max, range);
    ser.value4b(following);
  });
  ctx.createDeserializer().enableBitPacking([&](BPDes& des) {
    des.ext(result, range);
    des.value4b(followingResult);
  });

  EXPECT_THAT(result, Eq(max));
  EXPECT_THAT(followingResult, Eq(following));
  EXPECT_THAT(ctx.des->adapter().error(),
              Eq(bitsery::ReaderError::NoError));
  EXPECT_TRUE(ctx.des->adapter().isCompletedSuccessfully());
}

TYPED_TEST(FloatingPointValueRange, UpperCodesStayWithinRange)
{
  using UInt = bitsery::details::SameSizeUnsigned<TypeParam>;
  const size_t bits = std::numeric_limits<TypeParam>::digits + 2;
  const UInt maxCode = (UInt{ 1 } << bits) - 1;
  const TypeParam min = static_cast<TypeParam>(-0.2);
  const TypeParam max = static_cast<TypeParam>(0.1);
  const ValueRange<TypeParam> range{ min, max, BitsConstraint{ bits } };

  // At this precision, more than one integer code rounds to a ratio of one.
  for (const UInt code : { maxCode - UInt{ 1 }, maxCode }) {
    SerializationContext ctx;
    TypeParam result{};
    ctx.createSerializer().enableBitPacking(
      [&](BPSer& ser) { ser.adapter().writeBits(code, bits); });
    ctx.createDeserializer().enableBitPacking(
      [&](BPDes& des) { des.ext(result, range); });

    EXPECT_THAT(result, Ge(min));
    EXPECT_THAT(result, Le(max));
    EXPECT_THAT(ctx.des->adapter().error(),
                Eq(bitsery::ReaderError::NoError));
    EXPECT_TRUE(ctx.des->adapter().isCompletedSuccessfully());
  }
}
