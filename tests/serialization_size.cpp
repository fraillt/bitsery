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

bool SerializeDeserializeContainerSize(SerializationContext& ctx, const size_t size) {
    std::vector<char> t1(size);
    ctx.createSerializer().container(t1, size+1, []( char& ){});
    t1.clear();
    ctx.createDeserializer().container(t1, size+1, []( char& ){});
    return t1.size() == size;
}

TEST(SerializeSize, WhenLengthLessThan128Then1Byte) {
    SerializationContext ctx1{};
    EXPECT_TRUE(SerializeDeserializeContainerSize(ctx1, 127));
    EXPECT_THAT(ctx1.getBufferSize(), Eq(1));
    SerializationContext ctx2;
    EXPECT_TRUE(SerializeDeserializeContainerSize(ctx2, 128));
    EXPECT_THAT(ctx2.getBufferSize(), testing::Gt(1));
}

TEST(SerializeSize, WhenLengthLessThan16384Then2Bytes) {
    SerializationContext ctx1;
    EXPECT_TRUE(SerializeDeserializeContainerSize(ctx1, 16383));
    EXPECT_THAT(ctx1.getBufferSize(), Eq(2));
    SerializationContext ctx2;
    EXPECT_TRUE(SerializeDeserializeContainerSize(ctx2, 16384));
    EXPECT_THAT(ctx2.getBufferSize(), testing::Gt(2));
}

TEST(SerializeSize, WhenGreaterThan16383Then4Bytes) {
    SerializationContext ctx1;
    EXPECT_TRUE(SerializeDeserializeContainerSize(ctx1, 16384));
    EXPECT_THAT(ctx1.getBufferSize(), Eq(4));
    SerializationContext ctx2;
    EXPECT_TRUE(SerializeDeserializeContainerSize(ctx2, 66384));
    EXPECT_THAT(ctx2.getBufferSize(), Eq(4));
}