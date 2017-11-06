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

template <typename ... Args>
struct ConfigWithContext: bitsery::DefaultConfig {
    using InternalContext = std::tuple<Args...>;
};

template <typename Context, typename ... Args>
using SerializerConfigWithContext = bitsery::BasicSerializer<
        bitsery::AdapterWriter<bitsery::OutputBufferAdapter<Buffer>, ConfigWithContext<Args...>>, Context>;

template <typename Context, typename ... Args>
using DeserializerConfigWithContext = bitsery::BasicDeserializer<
        bitsery::AdapterReader<bitsery::InputBufferAdapter<Buffer>, ConfigWithContext<Args...>>, Context>;

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

TEST(SerializationContext, SerializerDeserializerCanHaveInternalContextViaConfig) {
    Buffer buf{};
    SerializerConfigWithContext<void, float, int> ser{buf};
    EXPECT_THAT(ser.context<int>(), ::testing::NotNull());
    EXPECT_THAT(*ser.context<int>(), Eq(0));
    *ser.context<int>() = 10;
    EXPECT_THAT(*ser.context<int>(), Eq(10));

    DeserializerConfigWithContext<void, char> des{InputAdapter{buf.begin(), 1}};
    EXPECT_THAT(des.context<char>(), ::testing::NotNull());
    EXPECT_THAT(*des.context<char>(), Eq(0));
    *des.context<char>() = 10;
    EXPECT_THAT(*des.context<char>(), Eq(10));

    //new instance has new context
    SerializerConfigWithContext<void, float, int> ser2{buf};
    EXPECT_THAT(ser2.context<int>(), ::testing::NotNull());
    EXPECT_THAT(*ser2.context<int>(), Eq(0));
}

TEST(SerializationContext, WhenInternalAndExternalContextIsTheSamePriorityGoesToInternalContext) {
    Buffer buf{};
    int externalCtx = 5;

    SerializerConfigWithContext<int, float, int> ser{buf, &externalCtx};
    EXPECT_THAT(ser.context<int>(), ::testing::NotNull());
    EXPECT_THAT(*ser.context<int>(), Eq(0));
    *ser.context<int>() = 2;

    DeserializerConfigWithContext<int, int, char> des{InputAdapter{buf.begin(), 1}, &externalCtx};
    EXPECT_THAT(des.context<char>(), ::testing::NotNull());
    EXPECT_THAT(*des.context<char>(), Eq(0));
    *des.context<int>() = 3;

    EXPECT_THAT(externalCtx, Eq(5));
}

TEST(SerializationContext, ContextIfExistsReturnsNullWhenTypeDoesntExists) {
    Buffer buf{};
    std::tuple<double, short> extCtx1{};

    SerializerConfigWithContext<std::tuple<double, short>, float, int> ser{buf, &extCtx1};
    EXPECT_THAT(ser.contextOrNull<int>(), ::testing::NotNull());
    EXPECT_THAT(ser.contextOrNull<char>(), ::testing::IsNull());

    double extCtx2{};
    DeserializerConfigWithContext<double, int, char> des{InputAdapter{buf.begin(), 1}, &extCtx2};
    EXPECT_THAT(des.contextOrNull<double>(), ::testing::NotNull());
    EXPECT_THAT(des.contextOrNull<float>(), ::testing::IsNull());
}