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
#include <bitsery/buffer_writer.h>
#include <bitsery/buffer_reader.h>
#include <list>
#include <bitset>

using testing::Eq;
using testing::ContainerEq;
using bitsery::BufferWriter;
using bitsery::BufferReader;
using Buffer = bitsery::DefaultConfig::BufferType;

struct IntegralTypes {
    int64_t a;
    uint32_t b;
    int16_t c;
    uint8_t d;
    int8_t e;
    int8_t f[2];
};

TEST(BufferReading, ReadReturnsFalseIfNotEnoughBufferSize) {
    //setup data
    uint8_t a = 111;

    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};

    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    bw.flush();
    //read from buffer
    BufferReader br{bw.getWrittenRange()};
    int16_t b;
    int32_t c;
    EXPECT_THAT(br.readBytes<4>(c), Eq(false));
    EXPECT_THAT(br.readBytes<2>(b), Eq(true));
    EXPECT_THAT(br.readBytes<2>(b), Eq(false));
    EXPECT_THAT(br.readBytes<1>(a), Eq(true));
    EXPECT_THAT(br.readBytes<1>(a), Eq(false));
    EXPECT_THAT(br.readBytes<2>(b), Eq(false));
    EXPECT_THAT(br.readBytes<4>(c), Eq(false));

}

TEST(BufferReading, ReadIsCompletedWhenAllBytesAreRead) {
    //setup data
    IntegralTypes data;
    data.b = 94545646;
    data.c = -8778;
    data.d = 200;

    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};

    bw.writeBytes<4>(data.b);
    bw.writeBytes<2>(data.c);
    bw.writeBytes<1>(data.d);
    bw.flush();
    //read from buffer
    BufferReader br{bw.getWrittenRange()};
    IntegralTypes res;
    EXPECT_THAT(br.readBytes<4>(res.b), Eq(true));
    EXPECT_THAT(br.readBytes<2>(res.c), Eq(true));
    EXPECT_THAT(br.isCompleted(), Eq(false));
    EXPECT_THAT(br.readBytes<1>(res.d), Eq(true));
    EXPECT_THAT(br.isCompleted(), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.d), Eq(false));
    EXPECT_THAT(br.isCompleted(), Eq(true));

    BufferReader br1{bw.getWrittenRange()};
    EXPECT_THAT(br1.readBytes<4>(res.b), Eq(true));
    EXPECT_THAT(br1.readBytes<2>(res.c), Eq(true));
    EXPECT_THAT(br1.isCompleted(), Eq(false));
    EXPECT_THAT(br1.readBytes<2>(res.c), Eq(false));
    EXPECT_THAT(br1.isCompleted(), Eq(false));
    EXPECT_THAT(br1.readBytes<1>(res.d), Eq(true));
    EXPECT_THAT(br1.isCompleted(), Eq(true));

}

