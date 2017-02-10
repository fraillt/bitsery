//
// Created by fraillt on 17.2.7.
//


#include <gmock/gmock.h>
#include "SerializationTestUtils.h"
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

    ctx.createSerializer().container(this->src);
    ctx.createDeserializer().container(this->res);

    EXPECT_THAT(ctx.getBufferSize(), Eq(this->getExpectedBufSize(ctx)));
    EXPECT_THAT(this->res, ContainerEq(this->src));
}

TYPED_TEST(SerializeContainerArthmeticTypes, ValuesWithExplicitSize) {
    SerializationContext ctx{};
    using TValue = typename TestFixture::TValue;

    ctx.createSerializer().container<sizeof(TValue)>(this->src);
    ctx.createDeserializer().container<sizeof(TValue)>(this->res);

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
    });
    auto des = ctx.createDeserializer();
    des.container(this->res, [&des](auto&v ) {
        des.value(v);
        //increment by 1 after reading
        v++;
    });
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

    ctx.createSerializer().container(this->src);
    ctx.createDeserializer().container(this->res);

    EXPECT_THAT(ctx.getBufferSize(), Eq(this->getExpectedBufSize(ctx)));
    EXPECT_THAT(this->res, ContainerEq(this->src));
}


TYPED_TEST(SerializeContainerCompositeTypes, CustomFunctionThatDoNothing) {
    SerializationContext ctx{};

    auto emptyFnc = [](auto v) {};
    ctx.createSerializer().container(this->src, emptyFnc);
    ctx.createDeserializer().container(this->res, emptyFnc);

    EXPECT_THAT(ctx.getBufferSize(), Eq(ctx.containerSizeSerializedBytesCount(this->src.size())));
}


