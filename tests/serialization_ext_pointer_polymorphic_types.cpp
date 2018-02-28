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
#include <bitsery/ext/pointer.h>

using bitsery::ext::BaseClass;
using bitsery::ext::VirtualBaseClass;

using bitsery::ext::PointerOwner;
using bitsery::ext::PointerObserver;
using bitsery::ext::ReferencedByPointer;
using bitsery::ext::PointerLinkingContext;
using bitsery::ext::PointerType;

using testing::Eq;

using TContext = std::tuple<PointerLinkingContext, bitsery::ext::InheritanceContext>;
using SerContext = BasicSerializationContext<bitsery::DefaultConfig, TContext>;

/*
 * base class
 */
struct Base {
    uint8_t x{};
    virtual ~Base() = default;
};

template <typename S>
void serialize(S& s, Base& o) {
    s.value1b(o.x);
}

struct Derived1:virtual Base {
    uint8_t y1{};
};

template <typename S>
void serialize(S& s, Derived1& o) {
    s.ext(o, VirtualBaseClass<Base>{});
    s.value1b(o.y1);
}

struct Derived2:virtual Base {
    uint8_t y2{};
};

template <typename S>
void serialize(S& s, Derived2& o) {
    s.ext(o, VirtualBaseClass<Base>{});
    s.value1b(o.y2);
}

struct MultipleVirtualInheritance: Derived1, Derived2 {
    int8_t z{};
    MultipleVirtualInheritance() = default;

    MultipleVirtualInheritance(uint8_t x_, uint8_t y1_, uint8_t y2_, uint8_t z_) {
        x = x_;
        y1 = y1_;
        y2 = y2_;
        z = z_;
    }

    template <typename S>
    void serialize(S& s) {
        s.ext(*this, BaseClass<Derived1>{});
        s.ext(*this, BaseClass<Derived2>{});
        s.value1b(z);
    }

};

//define PolymorphicBase relationships for runtime polymorphism

namespace bitsery {
    namespace ext {

        template <>
        struct PolymorphicBaseClass<Base>: DerivedClasses<Derived1, Derived2> {};

        template <>
        struct PolymorphicBaseClass<Derived1>: DerivedClasses<MultipleVirtualInheritance> {};

        template <>
        struct PolymorphicBaseClass<Derived2>: DerivedClasses<MultipleVirtualInheritance> {};

    }
}


class SerializeExtensionPointerPolymorphicTypes: public testing::Test {
public:

    TContext plctx1{};
    SerContext sctx1{};

    typename SerContext::TSerializer& createSerializer() {
        return sctx1.createSerializer(&plctx1);
    }

    typename SerContext::TDeserializer& createDeserializer() {
        return sctx1.createDeserializer(&plctx1);
    }

    bool isPointerContextValid() {
        return std::get<0>(plctx1).isValid();
    }

    virtual void TearDown() override {
        EXPECT_TRUE(isPointerContextValid());
    }
};

TEST_F(SerializeExtensionPointerPolymorphicTypes, Data0Result0) {
    Base* baseData = nullptr;
    createSerializer().ext(baseData, PointerOwner{});
    Base* baseRes = nullptr;
    createDeserializer().ext(baseRes, PointerOwner{});

    EXPECT_THAT(baseRes, ::testing::IsNull());
    EXPECT_THAT(baseData, ::testing::IsNull());
}

TEST_F(SerializeExtensionPointerPolymorphicTypes, Data0Result1) {
    Base* baseData = nullptr;
    createSerializer().ext(baseData, PointerOwner{});

    Base* baseRes = new Derived1{};
    createDeserializer().ext(baseRes, PointerOwner{});

    EXPECT_THAT(baseRes, ::testing::IsNull());
    EXPECT_THAT(baseData, ::testing::IsNull());
}

TEST_F(SerializeExtensionPointerPolymorphicTypes, Data1Result0) {
    Derived1 d1{};
    d1.x = 3;
    d1.y1 = 78;
    Base* baseData = &d1;
    createSerializer().ext(baseData, PointerOwner{});
    Base* baseRes = nullptr;
    createDeserializer().ext(baseRes, PointerOwner{});

    auto* data = dynamic_cast<Derived1*>(baseData);
    auto* res = dynamic_cast<Derived1*>(baseRes);

    EXPECT_THAT(baseRes, ::testing::NotNull());
    EXPECT_THAT(data, ::testing::NotNull());
    EXPECT_THAT(res, ::testing::NotNull());
    EXPECT_THAT(res->x, Eq(data->x));
    EXPECT_THAT(res->y1, Eq(data->y1));
    delete baseRes;
}

TEST_F(SerializeExtensionPointerPolymorphicTypes, Data1Result1) {
    Derived1 d1{};
    d1.x = 3;
    d1.y1 = 78;
    Base* baseData = &d1;
    createSerializer().ext(baseData, PointerOwner{});
    Base* baseRes = &d1;
    createDeserializer().ext(baseRes, PointerOwner{});

    auto* data = dynamic_cast<Derived1*>(baseData);
    auto* res = dynamic_cast<Derived1*>(baseRes);

    EXPECT_THAT(baseRes, ::testing::NotNull());
    EXPECT_THAT(data, ::testing::NotNull());
    EXPECT_THAT(res, ::testing::NotNull());
    EXPECT_THAT(res->x, Eq(data->x));
    EXPECT_THAT(res->y1, Eq(data->y1));
}

TEST_F(SerializeExtensionPointerPolymorphicTypes, ComplexTypeData1Result0) {
    MultipleVirtualInheritance md1{};
    md1.x = 3;
    md1.y1 = 78;
    md1.y2 = 14;
    md1.z = -33;
    Base* baseData = &md1;
    createSerializer().ext(baseData, PointerOwner{});
    Base* baseRes = nullptr;
    createDeserializer().ext(baseRes, PointerOwner{});

    auto* data = dynamic_cast<MultipleVirtualInheritance*>(baseData);
    auto* res = dynamic_cast<MultipleVirtualInheritance*>(baseRes);

    EXPECT_THAT(baseRes, ::testing::NotNull());
    EXPECT_THAT(data, ::testing::NotNull());
    EXPECT_THAT(res, ::testing::NotNull());
    EXPECT_THAT(res->x, Eq(data->x));
    EXPECT_THAT(res->y1, Eq(data->y1));
    EXPECT_THAT(res->y2, Eq(data->y2));
    EXPECT_THAT(res->z, Eq(data->z));
    delete baseRes;
}

TEST_F(SerializeExtensionPointerPolymorphicTypes, WhenResultIsDifferentTypeThenRecreate) {
    MultipleVirtualInheritance md1{};
    md1.x = 3;
    md1.y1 = 78;
    md1.y2 = 14;
    md1.z = -33;
    Base* baseData = &md1;
    createSerializer().ext(baseData, PointerOwner{});
    Base* baseRes = new Derived1{};
    EXPECT_THAT(dynamic_cast<MultipleVirtualInheritance*>(baseRes), ::testing::IsNull());
    createDeserializer().ext(baseRes, PointerOwner{});
    EXPECT_THAT(dynamic_cast<MultipleVirtualInheritance*>(baseRes), ::testing::NotNull());
    delete baseRes;
}

//struct UnknownType:Base {
//};
//
//template <typename S>
//void serialize(S& s, UnknownType& o) {
//    s.ext(o, VirtualBaseClass<Base>{});
//}

// todo reimplement whole polymorphism thing, because with current solution this test fails
//TEST(SerializeExtensionPointerPolymorphicTypesErrors, WhenSerializingUnknownTypeThenAssert) {
//    TContext plctx1{};
//    SerContext sctx1{};
//    UnknownType obj{};
//    UnknownType* unknownPtr = &obj;
//    EXPECT_DEATH(sctx1.createSerializer(&plctx1).ext(unknownPtr, PointerOwner{}), "");
//}
