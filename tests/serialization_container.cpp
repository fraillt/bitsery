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
#include <numeric>
#include <deque>
#include <list>

using testing::ContainerEq;
using testing::Eq;

template <typename Container>
Container getFilledContainer() {
    return {1,2,3,4,5,78,456,8,54};
}

template <typename T>
class SerializeContainerArthmeticTypes:public testing::Test {
public:
    using TContainer = T;
    using TValue = typename T::value_type;

    const TContainer src= getFilledContainer<TContainer>() ;
    TContainer res{};

    size_t getExpectedBufSize(const SerializationContext& ctx) const {
        return ctx.containerSizeSerializedBytesCount(src.size()) + src.size() * sizeof(TValue);
    }
};
//std::forward_list is not supported, because it doesn't have size() method
using SequenceContainersWithArthmeticTypes = ::testing::Types<
        std::vector<int>,
        std::list<float>,
        std::deque<unsigned short>>;

TYPED_TEST_CASE(SerializeContainerArthmeticTypes, SequenceContainersWithArthmeticTypes);

TYPED_TEST(SerializeContainerArthmeticTypes, Values) {
    SerializationContext ctx{};

    ctx.createSerializer().container(this->src, 1000);
    ctx.createDeserializer().container(this->res, 1000);

    EXPECT_THAT(ctx.getBufferSize(), Eq(this->getExpectedBufSize(ctx)));
    EXPECT_THAT(this->res, ContainerEq(this->src));
}

TYPED_TEST(SerializeContainerArthmeticTypes, ValuesWithExplicitSize) {
    SerializationContext ctx{};
    using TValue = typename TestFixture::TValue;

    ctx.createSerializer().container<sizeof(TValue)>(this->src, 1000);
    ctx.createDeserializer().container<sizeof(TValue)>(this->res, 1000);

    EXPECT_THAT(ctx.getBufferSize(), Eq(this->getExpectedBufSize(ctx)));
    EXPECT_THAT(this->res, ContainerEq(this->src));
}

TYPED_TEST(SerializeContainerArthmeticTypes, CustomFunctionIncrements) {
    SerializationContext ctx{};

    auto ser = ctx.createSerializer();
    ser.container(this->src, [&ser](auto v ) {
        //increment by 1 before writing
        v++;
        ser.value(v);
    }, 1000);
    auto des = ctx.createDeserializer();
    des.container(this->res, [&des](auto&v ) {
        des.value(v);
        //increment by 1 after reading
        v++;
    }, 1000);
    //decrement result by 2, before comparing for eq
    for(auto& v:this->res)
        v -= 2;

    EXPECT_THAT(ctx.getBufferSize(), Eq(this->getExpectedBufSize(ctx)));
    EXPECT_THAT(this->res, ContainerEq(this->src));
}




template <typename T>
class SerializeContainerCompositeTypes:public testing::Test {
public:
    using TContainer = T;
    using TValue = typename T::value_type;

    const TContainer src= getFilledContainer<TContainer>();
    TContainer res{};
    size_t getExpectedBufSize(const SerializationContext& ctx) const {
        return ctx.containerSizeSerializedBytesCount(src.size()) + src.size() * TValue::SIZE;
    }
};

template <>
std::vector<MyStruct1> getFilledContainer<std::vector<MyStruct1>>() {
    return {
            {0,1},
            {2,3},
            {4,5},
            {6,7},
            {8,9},
            {11,34},
            {5134,1532}
    };
}

template <>
std::list<MyStruct2> getFilledContainer<std::list<MyStruct2>>() {
    return {
            {MyStruct2::V1, {0,1}} ,
            {MyStruct2::V3, {-45,45}}
    };
}

using SequenceContainersWithCompositeTypes = ::testing::Types<
        std::vector<MyStruct1>,
        std::list<MyStruct2>>;

TYPED_TEST_CASE(SerializeContainerCompositeTypes, SequenceContainersWithCompositeTypes);

TYPED_TEST(SerializeContainerCompositeTypes, DefaultSerializeFunction) {
    SerializationContext ctx{};

    ctx.createSerializer().container(this->src, 1000);
    ctx.createDeserializer().container(this->res, 1000);

    EXPECT_THAT(ctx.getBufferSize(), Eq(this->getExpectedBufSize(ctx)));
    EXPECT_THAT(this->res, ContainerEq(this->src));
}


TYPED_TEST(SerializeContainerCompositeTypes, CustomFunctionThatDoNothing) {
    SerializationContext ctx{};

    auto emptyFnc = [](auto v) {};
    ctx.createSerializer().container(this->src, emptyFnc, 1000);
    ctx.createDeserializer().container(this->res, emptyFnc, 1000);

    EXPECT_THAT(ctx.getBufferSize(), Eq(ctx.containerSizeSerializedBytesCount(this->src.size())));
}


