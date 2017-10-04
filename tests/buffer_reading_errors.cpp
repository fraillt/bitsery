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
#include <bitsery/traits/string.h>

using testing::Eq;
using bitsery::BufferWriter;
using bitsery::BufferReader;
using Buffer = bitsery::DefaultConfig::BufferType;

TEST(BufferReadingErrors, WhenContainerOrTextSizeIsMoreThanMaxThenInvalidBufferDataError) {
    SerializationContext ctx;
    std::string tmp = "larger text then allowed";
    ctx.createSerializer().text1b(tmp,100);
    ctx.createDeserializer().text1b(tmp, 10);
    EXPECT_THAT(ctx.br->getError(), Eq(bitsery::BufferReaderError::INVALID_BUFFER_DATA));
}

TEST(BufferReadingErrors, WhenReadingBoolByteReadsMoreThanOneThenInvalidBufferDataErrorAndResultIsFalse) {
    SerializationContext ctx;
    auto ser = ctx.createSerializer();
    ser.value1b(uint8_t{1});
    ser.value1b(uint8_t{2});
    bool res{};
    auto des = ctx.createDeserializer();
    des.boolValue(res);
    EXPECT_THAT(res, Eq(true));
    des.boolValue(res);
    EXPECT_THAT(res, Eq(false));
    EXPECT_THAT(ctx.br->getError(), Eq(bitsery::BufferReaderError::INVALID_BUFFER_DATA));
}

TEST(BufferReadingErrors, WhenReadingAlignHasNonZerosThenInvalidBufferDataError) {
    Buffer buf{};
    BufferWriter bw{buf};
    uint8_t tmp{0xFF};
    bw.writeBytes<1>(tmp);
    bw.flush();

    BufferReader br{bw.getWrittenRange()};
    bitsery::BitPackingReader<bitsery::DefaultConfig> bpr{br};

    bpr.readBits(tmp,3);
    bpr.align();
    EXPECT_THAT(bpr.getError(), Eq(bitsery::BufferReaderError::INVALID_BUFFER_DATA));
}

TEST(BufferReadingErrors, WhenReadingNewSessionInMiddleOfOldDataThenInvalidBufferError) {
    uint8_t tmp{0xFF};
    Buffer buf{};
    BufferWriter bw{buf};
    for (auto i = 0; i < 2; ++i) {
        bw.beginSession();
        bw.writeBytes<1>(tmp);
        bw.writeBytes<1>(tmp);
        bw.endSession();
    }
    bw.flush();
    BufferReader br{bw.getWrittenRange()};
    for (auto i = 0; i < 2; ++i) {
        br.beginSession();
        br.readBytes<1>(tmp);
        br.beginSession();
        br.readBytes<1>(tmp);
        br.endSession();
        br.endSession();
    }
    EXPECT_THAT(br.getError(), Eq(bitsery::BufferReaderError::INVALID_BUFFER_DATA));
}


TEST(BufferReadingErrors, WhenInitializingSessionsWhenNotEnoughDataThenInvalidBufferData) {
    uint8_t tmp1{0xFF};
    Buffer buf1{};
    BufferWriter bw1{buf1};
    bw1.writeBytes<1>(tmp1);
    bw1.flush();
    BufferReader br1{bw1.getWrittenRange()};
    br1.beginSession();
    EXPECT_THAT(br1.getError(), Eq(bitsery::BufferReaderError::INVALID_BUFFER_DATA));

    Buffer buf2{};
    BufferWriter bw2{buf2};
    uint16_t tmp2{0x8000};
    bw2.writeBytes<2>(tmp2);
    bw2.flush();
    BufferReader br2{bw2.getWrittenRange()};
    br2.beginSession();
    EXPECT_THAT(br2.getError(), Eq(bitsery::BufferReaderError::INVALID_BUFFER_DATA));
}

TEST(BufferReadingErrors, WhenInitializingSessionsWhereSessionsDataOffsetIsCorruptedThenInvalidBufferData) {
    Buffer buf{};
    BufferWriter bw{buf};
    bw.writeBytes<1>(uint8_t{1});
    bw.writeBytes<1>(uint8_t{1});
    bw.writeBytes<2>(uint16_t{10});
    BufferReader br{bw.getWrittenRange()};
    br.beginSession();
    EXPECT_THAT(br.getError(), Eq(bitsery::BufferReaderError::INVALID_BUFFER_DATA));
}
