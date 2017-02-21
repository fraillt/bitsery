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
#include "SerializationTestUtils.h"

using testing::Eq;


TEST(SerializeBooleans, BoolAsBit) {
    SerializationContext ctx;
    bool t1{true};
    bool t2{false};
    bool res1;
    bool res2;
    ctx.createSerializer().boolBit(t1).boolBit(t2);
    ctx.createDeserializer().boolBit(res1).boolBit(res2);

    EXPECT_THAT(res1, Eq(t1));
    EXPECT_THAT(res2, Eq(t2));
    EXPECT_THAT(ctx.getBufferSize(), Eq(1));
}

TEST(SerializeBooleans, BoolAsByte) {
    SerializationContext ctx;
    bool t1{true};
    bool t2{false};
    bool res1;
    bool res2;
    ctx.createSerializer().boolByte(t1).boolByte(t2);
    ctx.createDeserializer().boolByte(res1).boolByte(res2);

    EXPECT_THAT(res1, Eq(t1));
    EXPECT_THAT(res2, Eq(t2));
    EXPECT_THAT(ctx.getBufferSize(), Eq(2));
}
