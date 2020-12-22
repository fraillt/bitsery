//MIT License
//
//Copyright (c) 2020 Guillaume Godet-Bar
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

using testing::Eq;

#include <bitsery/ext/std_pair.h>

TEST(SerializeExtensionStdPair, UseDefaultSerializeFunction) {
    std::pair<MyStruct1, MyStruct2> t1{MyStruct1{-789, 45}, MyStruct2{MyStruct2::MyEnum::V3, MyStruct1{}}};
    std::pair<MyStruct1, MyStruct2> r1{};
    SerializationContext ctx;
    ctx.createSerializer().ext(t1, bitsery::ext::StdPair{});
    ctx.createDeserializer().ext(r1, bitsery::ext::StdPair{});
    EXPECT_THAT(t1, Eq(r1));
}