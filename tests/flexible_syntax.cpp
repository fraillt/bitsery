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
#include <bitsery/flexible.h>

#include <bitsery/flexible/string.h>
#include <bitsery/flexible/array.h>
#include <bitsery/flexible/vector.h>
#include <bitsery/flexible/list.h>
#include <bitsery/flexible/forward_list.h>
#include <bitsery/flexible/deque.h>
#include <bitsery/flexible/queue.h>
#include <bitsery/flexible/stack.h>
#include <bitsery/flexible/map.h>
#include <bitsery/flexible/unordered_map.h>
#include <bitsery/flexible/set.h>
#include <bitsery/flexible/unordered_set.h>

using testing::Eq;

TEST(FlexibleSyntax, FundamentalTypesAndBool) {
    int ti = 8745;
    MyEnumClass te = MyEnumClass::E4;
    float tf = 485.042f;
    double_t td = -454184.48445;
    bool tb=true;
    SerializationContext ctx{};
    ctx.createSerializer().archive(ti,te,tf,td,tb);

    //result
    int ri{};
    MyEnumClass re{};
    float rf{};
    double_t rd{};
    bool rb{};
    ctx.createDeserializer().archive(ri,re,rf,rd,rb);

    //test
    EXPECT_THAT(ri, Eq(ti));
    EXPECT_THAT(re, Eq(te));
    EXPECT_THAT(rf, Eq(tf));
    EXPECT_THAT(rd, Eq(td));
    EXPECT_THAT(rb, Eq(tb));
}

TEST(FlexibleSyntax, UseObjectFncInsteadOfValueN) {
    int ti = 8745;
    MyEnumClass te = MyEnumClass::E4;
    float tf = 485.042f;
    double_t td = -454184.48445;
    bool tb=true;
    SerializationContext ctx;
    auto& ser = ctx.createSerializer();
    ser.object(ti);
    ser.object(te);
    ser.object(tf);
    ser.object(td);
    ser.object(tb);

    //result
    int ri{};
    MyEnumClass re{};
    float rf{};
    double_t rd{};
    bool rb{};
    auto& des = ctx.createDeserializer();
    des.object(ri);
    des.object(re);
    des.object(rf);
    des.object(rd);
    des.object(rb);

    //test
    EXPECT_THAT(ri, Eq(ti));
    EXPECT_THAT(re, Eq(te));
    EXPECT_THAT(rf, Eq(tf));
    EXPECT_THAT(rd, Eq(td));
    EXPECT_THAT(rb, Eq(tb));
}

TEST(FlexibleSyntax, MixDifferentSyntax) {
    int ti = 8745;
    MyEnumClass te = MyEnumClass::E4;
    float tf = 485.042f;
    double_t td = -454184.48445;
    bool tb=true;
    SerializationContext ctx;
    auto& ser = ctx.createSerializer();
    ser.value<sizeof(ti)>(ti);
    ser.archive(te, tf, td);
    ser.object(tb);

    //result
    int ri{};
    MyEnumClass re{};
    float rf{};
    double_t rd{};
    bool rb{};
    auto& des = ctx.createDeserializer();
    des.archive(ri, re, rf);
    des.value8b(rd);
    des.object(rb);

    //test
    EXPECT_THAT(ri, Eq(ti));
    EXPECT_THAT(re, Eq(te));
    EXPECT_THAT(rf, Eq(tf));
    EXPECT_THAT(rd, Eq(td));
    EXPECT_THAT(rb, Eq(tb));
}


template <typename T>
T procArchive(const T& testData) {
    SerializationContext ctx;
    ctx.createSerializer().archive(testData);
    T res;
    ctx.createDeserializer().archive(res);
    return res;
}

template <typename T>
T procArchiveWithMaxSize(const T& testData) {
    SerializationContext ctx;
    ctx.createSerializer().archive(bitsery::maxSize(testData, 100));
    T res;
    ctx.createDeserializer().archive(bitsery::maxSize(res, 100));
    return res;
}

TEST(FlexibleSyntax, CStyleArrayForValueTypesAsContainer) {
    const int t1[3]{8748,-484,45};
    int r1[3]{0,0,0};

    SerializationContext ctx;
    ctx.createSerializer().archive(bitsery::asContainer(t1));
    ctx.createDeserializer().archive(bitsery::asContainer(r1));

    EXPECT_THAT(r1, ::testing::ContainerEq(t1));
}

TEST(FlexibleSyntax, CStyleArrayForIntegralTypesAsText) {
    const char t1[3]{"hi"};
    char r1[3]{0,0,0};

    SerializationContext ctx;
    ctx.createSerializer().archive(bitsery::asText(t1));
    ctx.createDeserializer().archive(bitsery::asText(r1));

    EXPECT_THAT(r1, ::testing::ContainerEq(t1));
}

TEST(FlexibleSyntax, CStyleArray) {
    const MyEnumClass t1[3]{MyEnumClass::E1, MyEnumClass::E4, MyEnumClass::E2};
    MyEnumClass r1[3]{};

    SerializationContext ctx;
    ctx.createSerializer().archive(t1);
    ctx.createDeserializer().archive(r1);

    EXPECT_THAT(r1, ::testing::ContainerEq(t1));
}


TEST(FlexibleSyntax, StdString) {
    std::string t1{"my nice string"};
    std::string t2{};

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchive(t2), Eq(t2));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t2), Eq(t2));
}

TEST(FlexibleSyntax, StdArray) {
    std::array<int, 3> t1{8748,-484,45};
    std::array<int, 0> t2{};

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchive(t2), Eq(t2));

}

TEST(FlexibleSyntax, StdVector) {
    std::vector<int> t1{8748,-484,45};
    std::vector<float> t2{5.f,0.198f};

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchive(t2), Eq(t2));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t2), Eq(t2));

}

TEST(FlexibleSyntax, StdList) {
    std::list<int> t1{8748,-484,45};
    std::list<float> t2{5.f,0.198f};

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchive(t2), Eq(t2));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t2), Eq(t2));

}

TEST(FlexibleSyntax, StdForwardList) {
    std::forward_list<int> t1{8748,-484,45};
    std::forward_list<float> t2{5.f,0.198f};

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchive(t2), Eq(t2));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t2), Eq(t2));

}

TEST(FlexibleSyntax, StdDeque) {
    std::deque<int> t1{8748,-484,45};
    std::deque<float> t2{5.f,0.198f};

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchive(t2), Eq(t2));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t2), Eq(t2));

}

TEST(FlexibleSyntax, StdQueue) {
    std::queue<std::string> t1;
    t1.push("first");
    t1.push("second string");

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));

}

TEST(FlexibleSyntax, StdPriorityQueue) {
    std::priority_queue<std::string> t1;
    t1.push("first");
    t1.push("second string");
    t1.push("third");
    t1.push("fourth");
    auto r1 = procArchive(t1);
    //we cannot compare priority queue directly

    EXPECT_THAT(r1.size(), Eq(t1.size()));
    for (auto i = 0u; i < r1.size(); ++i) {
        EXPECT_THAT(r1.top(), Eq(t1.top()));
        r1.pop();
        t1.pop();
    }

}

TEST(FlexibleSyntax, StdStack) {
    std::stack<std::string> t1;
    t1.push("first");
    t1.push("second string");

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));

}

TEST(FlexibleSyntax, StdUnorderedMap) {
    std::unordered_map<int, int> t1;
    t1.emplace(3423,624);
    t1.emplace(-5484,-845);

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));
}

TEST(FlexibleSyntax, StdUnorderedMultiMap) {
    std::unordered_multimap<std::string, int> t1;
    t1.emplace("one",624);
    t1.emplace("two",-845);
    t1.emplace("one",897);

    EXPECT_TRUE(procArchive(t1) == t1);
    EXPECT_TRUE(procArchiveWithMaxSize(t1) == t1);
}

TEST(FlexibleSyntax, StdMap) {
    std::map<int, int> t1;
    t1.emplace(3423,624);
    t1.emplace(-5484,-845);

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));
}

TEST(FlexibleSyntax, StdMultiMap) {
    std::multimap<std::string, int> t1;
    t1.emplace("one",624);
    t1.emplace("two",-845);
    t1.emplace("one",897);

    auto res = procArchive(t1);
    //same key values is not ordered, and operator == compares each element at same position
    //so we need to compare our selves
    EXPECT_THAT(res.size(), Eq(3));
    for (auto it = t1.begin(); it != t1.end();) {
        const auto lr = t1.equal_range(it->first);
        const auto rr = res.equal_range(it->first);
        EXPECT_TRUE(std::distance(lr.first, lr.second) == std::distance(rr.first, rr.second));
        EXPECT_TRUE(std::is_permutation(lr.first, lr.second, rr.first));
        it = lr.second;
    }
}

TEST(FlexibleSyntax, StdUnorderedSet) {
    std::unordered_set<std::string> t1;
    t1.emplace("one");
    t1.emplace("two");
    t1.emplace("three");

    EXPECT_TRUE(procArchive(t1) == t1);
    EXPECT_TRUE(procArchiveWithMaxSize(t1) == t1);
}

TEST(FlexibleSyntax, StdUnorderedMultiSet) {
    std::unordered_multiset<std::string> t1;
    t1.emplace("one");
    t1.emplace("two");
    t1.emplace("three");
    t1.emplace("one");

    EXPECT_TRUE(procArchive(t1) == t1);
    EXPECT_TRUE(procArchiveWithMaxSize(t1) == t1);
}

TEST(FlexibleSyntax, StdSet) {
    std::set<std::string> t1;
    t1.emplace("one");
    t1.emplace("two");
    t1.emplace("three");

    EXPECT_TRUE(procArchive(t1) == t1);
    EXPECT_TRUE(procArchiveWithMaxSize(t1) == t1);
}

TEST(FlexibleSyntax, StdMultiSet) {
    std::multiset<std::string> t1;
    t1.emplace("one");
    t1.emplace("two");
    t1.emplace("three");
    t1.emplace("one");
    t1.emplace("two");

    EXPECT_TRUE(procArchive(t1) == t1);
    EXPECT_TRUE(procArchiveWithMaxSize(t1) == t1);
}


TEST(FlexibleSyntax, NestedTypes) {
    std::unordered_map<std::string, std::vector<std::string>> t1;
    t1.emplace("my key", std::vector<std::string>{"very", "nice", "string"});
    t1.emplace("other key", std::vector<std::string>{"just a string"});

    EXPECT_THAT(procArchive(t1), Eq(t1));
    EXPECT_THAT(procArchiveWithMaxSize(t1), Eq(t1));
}
