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
#include <list>
#include <bitset>

using testing::Eq;

struct IntegralTypes {
    int64_t a;
    uint32_t b;
    int16_t c;
    uint8_t d;
    int8_t e;
    int8_t f[2];
};

TEST(DataReading, WhenReadingMoreThanAvailableThenEmptyBufferError) {
    //setup data
    uint8_t a = 111;

    //create and write to buffer
    Buffer buf{};
    Writer bw{buf};

    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    bw.flush();
    //read from buffer
    Reader br{InputAdapter{buf.begin(), bw.writtenBytesCount()}};
    int32_t c;
    br.readBytes<4>(c);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::DataOverflow));
}

TEST(DataReading, WhenErrorOccursThenAllOtherOperationsFailsForSameError) {
    //setup data
    uint8_t a = 111;

    //create and write to buffer
    Buffer buf{};
    Writer bw{buf};

    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    bw.flush();
    //read from buffer
    Reader br{InputAdapter{buf.begin(), bw.writtenBytesCount()}};
    int32_t c;
    br.readBytes<4>(c);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::DataOverflow));
    br.readBytes<1>(a);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::DataOverflow));
}


TEST(DataReading, ReadIsCompletedSuccessfullyWhenAllBytesAreReadWithoutErrors) {
    //setup data
    IntegralTypes data;
    data.b = 94545646;
    data.c = -8778;
    data.d = 200;

    //create and write to buffer
    Buffer buf{};
    Writer bw{buf};

    bw.writeBytes<4>(data.b);
    bw.writeBytes<2>(data.c);
    bw.writeBytes<1>(data.d);
    bw.flush();
    //read from buffer
    Reader br{InputAdapter{buf.begin(), bw.writtenBytesCount()}};
    IntegralTypes res;
    br.readBytes<4>(res.b);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
    br.readBytes<2>(res.c);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
    EXPECT_THAT(br.isCompletedSuccessfully(), Eq(false));
    br.readBytes<1>(res.d);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
    EXPECT_THAT(br.isCompletedSuccessfully(), Eq(true));
    br.readBytes<1>(res.d);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::DataOverflow));
    EXPECT_THAT(br.isCompletedSuccessfully(), Eq(false));

    Reader br1{InputAdapter{buf.begin(), bw.writtenBytesCount()}};
    br1.readBytes<4>(res.b);
    EXPECT_THAT(br1.error(), Eq(bitsery::ReaderError::NoError));
    br1.readBytes<2>(res.c);
    EXPECT_THAT(br1.error(), Eq(bitsery::ReaderError::NoError));
    EXPECT_THAT(br1.isCompletedSuccessfully(), Eq(false));
    br1.readBytes<2>(res.c);
    EXPECT_THAT(br1.error(), Eq(bitsery::ReaderError::DataOverflow));
    EXPECT_THAT(br1.isCompletedSuccessfully(), Eq(false));
    br1.readBytes<1>(res.d);
    EXPECT_THAT(br1.error(), Eq(bitsery::ReaderError::DataOverflow));
    EXPECT_THAT(br1.isCompletedSuccessfully(), Eq(false));
}

TEST(DataReading, WhenReaderHasErrorsAllOperationsReadsReturnZero) {
    //setup data
    uint8_t a = 111;

    //create and write to buffer
    Buffer buf{};
    Writer bw{buf};

    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    bw.flush();
    //read from buffer
    Reader br{InputAdapter{buf.begin(), bw.writtenBytesCount()}};
    bitsery::AdapterReaderBitPackingWrapper<Reader> bpr{br};
    int32_t c;
    bpr.readBytes<4>(c);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::DataOverflow));

    int16_t r1= {-645};
    uint32_t r2[2] = {54898,87854};
    uint8_t r3 = 0xFF;

    bpr.readBytes<2>(r1);
    bpr.readBuffer<4>(r2, 2);
    bpr.readBits(r3, 7);
    EXPECT_THAT(r1, Eq(0));
    EXPECT_THAT(r2[0], Eq(0u));
    EXPECT_THAT(r2[1], Eq(0u));
    EXPECT_THAT(r3, Eq(0u));
}
