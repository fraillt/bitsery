//
// Created by fraillt on 17.2.15.
//

#include <gmock/gmock.h>
#include "SerializationTestUtils.h"
using namespace testing;

TEST(Ranges, RequiredBitsIsConstexpr) {
    constexpr RangeSpec<int> r1{0, 31};
    static_assert(r1.bitsRequired() == 5);

    constexpr RangeSpec<MyEnumClass> r2{MyEnumClass::E1, MyEnumClass::E4};
    static_assert(r2.bitsRequired() == 2);

    constexpr RangeSpec<double> r3{-1.0,1.0, 5u, 0};
    static_assert(r3.bitsRequired() == 5);

    constexpr RangeSpec<float> r4{-1.0f,1.0f, 0.01f};
    static_assert(r4.bitsRequired() == 8);

}