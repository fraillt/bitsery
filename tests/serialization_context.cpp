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

using testing::Eq;

template <typename Context>
using MySerializer = bitsery::BasicSerializer<Writer, Context>;

template <typename Context>
using MyDeserializer = bitsery::BasicDeserializer<Reader, Context>;

using SingleTypeContext = int;
using MultipleTypesContext = std::tuple<int, float, char>;

TEST(SerializationContext, WhenUsingContextThenReturnsUnderlyingPointerOrNull) {
    Buffer buf{};
    MySerializer<SingleTypeContext> ser1{buf, nullptr};
    EXPECT_THAT(ser1.context(), ::testing::IsNull());

    MySerializer<MultipleTypesContext> ser2{buf, nullptr};
    EXPECT_THAT(ser2.context(), ::testing::IsNull());

    SingleTypeContext sctx{};
    MyDeserializer<SingleTypeContext> des1{InputAdapter{buf.begin(), 1}, &sctx};
    EXPECT_THAT(des1.context(), Eq(&sctx));

    MultipleTypesContext mctx{};
    MyDeserializer<MultipleTypesContext> des2{InputAdapter{buf.begin(), 1}, &mctx};
    EXPECT_THAT(des2.context(), Eq(&mctx));

}

TEST(SerializationContext, WhenContextIsTupleThenContextCastOverloadCastsToIndividualTupleTypes) {
    Buffer buf{};
    MySerializer<MultipleTypesContext> ser1{buf, nullptr};
    EXPECT_THAT(ser1.context<int>(), ::testing::IsNull());
    EXPECT_THAT(ser1.context<float>(), ::testing::IsNull());
    EXPECT_THAT(ser1.context<char>(), ::testing::IsNull());
}

TEST(SerializationContext, WhenContextIsNotTupleThenContextCastOverloadReturnSameType) {
    Buffer buf{};
    SingleTypeContext ctx{};
    MySerializer<SingleTypeContext> ser1{buf, &ctx};
    EXPECT_THAT(ser1.context<SingleTypeContext>(), Eq(&ctx));
}