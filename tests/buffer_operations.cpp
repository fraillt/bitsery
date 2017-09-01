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
#include <bitsery/details/serialization_common.h>

using testing::Eq;
using testing::ContainerEq;
using bitsery::BufferWriter;
using bitsery::BufferReader;
using Buffer = bitsery::DefaultConfig::BufferType;

struct IntegralUnsignedTypes {
    uint32_t a;
    uint16_t b;
    uint8_t c;
    uint8_t d;
    uint64_t e;
};

template <typename T>
constexpr size_t getBits(T v) {
    return bitsery::details::calcRequiredBits<T>({}, v);
};

// *** bits operations

TEST(BufferBitsAndBytesOperations, WriteAndReadBits) {
    //setup data
    constexpr IntegralUnsignedTypes data{
        485454,//bits 19
        45978,//bits 16
        0,//bits 1
        36,//bits 6
        479845648946//bits 39
    };

    constexpr size_t aBITS = getBits(data.a) + 2;
    constexpr size_t bBITS = getBits(data.b) + 0;
    constexpr size_t cBITS = getBits(data.c) + 2;
    constexpr size_t dBITS = getBits(data.d) + 1;
    constexpr size_t eBITS = getBits(data.e) + 8;

    //create and write to buffer
    Buffer buf;
    BufferWriter bw{buf};

    bw.writeBits(data.a, aBITS);
    bw.writeBits(data.b, bBITS);
    bw.writeBits(data.c, cBITS);
    bw.writeBits(data.d, dBITS);
    bw.writeBits(data.e, eBITS);
    bw.flush();
    auto range = bw.getWrittenRange();
    auto bytesCount = ((aBITS + bBITS + cBITS + dBITS + eBITS) / 8) +1 ;
    EXPECT_THAT(std::distance(range.begin(), range.end()), Eq(bytesCount));
    //read from buffer
    BufferReader br{range};
    IntegralUnsignedTypes res;

    br.readBits(res.a, aBITS);
    br.readBits(res.b, bBITS);
    br.readBits(res.c, cBITS);
    br.readBits(res.d, dBITS);
    br.readBits(res.e, eBITS);

    EXPECT_THAT(res.a, Eq(data.a));
    EXPECT_THAT(res.b, Eq(data.b));
    EXPECT_THAT(res.c, Eq(data.c));
    EXPECT_THAT(res.d, Eq(data.d));
    EXPECT_THAT(res.e, Eq(data.e));

}

TEST(BufferBitsAndBytesOperations, BufferSizeIsCountedPerByteNotPerBit) {
    //setup data

    //create and write to buffer
    Buffer buf;
    BufferWriter bw{buf};

    bw.writeBits(7u,3);
    bw.flush();
    auto range = bw.getWrittenRange();
    EXPECT_THAT(std::distance(range.begin(), range.end()), Eq(1));

    //read from buffer
    BufferReader br{range};
    uint16_t tmp;
    EXPECT_THAT(br.readBits(tmp,4), Eq(true));
    EXPECT_THAT(br.readBits(tmp,2), Eq(true));
    EXPECT_THAT(br.readBits(tmp,2), Eq(true));
    EXPECT_THAT(br.readBits(tmp,2), Eq(false));

    //part of next byte
    BufferReader br1{range};
    EXPECT_THAT(br1.readBits(tmp,2), Eq(true));
    EXPECT_THAT(br1.readBits(tmp,7), Eq(false));

    //bigger than byte
    BufferReader br2{range};
    EXPECT_THAT(br2.readBits(tmp,9), Eq(false));
}

TEST(BufferBitsAndBytesOperations, ConsecutiveCallsToAlignHasNoEffect) {
    Buffer buf;
    BufferWriter bw{buf};

    bw.writeBits(3u, 2);
    //3 calls to align after 1st data
    bw.align();
    bw.align();
    bw.align();
    bw.writeBits(7u, 3);
    //1 call to align after 2nd data
    bw.align();
    bw.writeBits(15u, 4);
    bw.flush();

    unsigned char tmp;
    BufferReader br{bw.getWrittenRange()};
    EXPECT_THAT(br.readBits(tmp,2), Eq(true));
    EXPECT_THAT(tmp, Eq(3u));
    EXPECT_THAT(br.align(), Eq(true));

    EXPECT_THAT(br.readBits(tmp,3), Eq(true));
    EXPECT_THAT(tmp, Eq(7u));
    EXPECT_THAT(br.align(), Eq(true));
    EXPECT_THAT(br.align(), Eq(true));
    EXPECT_THAT(br.align(), Eq(true));

    EXPECT_THAT(br.readBits(tmp,4), Eq(true));
    EXPECT_THAT(tmp, Eq(15u));
}

TEST(BufferBitsAndBytesOperations, AlignWritesZerosBits) {
    //setup data

    //create and write to buffer
    Buffer buf;
    BufferWriter bw{buf};

    //write 2 bits and align
    bw.writeBits(3u, 2);
    bw.align();
    bw.flush();

    unsigned char tmp;
    BufferReader br1{bw.getWrittenRange()};
    br1.readBits(tmp,2);
    //read aligned bits
    EXPECT_THAT(br1.readBits(tmp,6), Eq(true));
    EXPECT_THAT(tmp, Eq(0));

    BufferReader br2{bw.getWrittenRange()};
    //read 2 bits
    br2.readBits(tmp,2);
    EXPECT_THAT(br2.align(), Eq(true));
}


// *** bytes operations

struct IntegralTypes {
    int64_t a;
    uint32_t b;
    int16_t c;
    uint8_t d;
    int8_t e;
    int8_t f[2];
};

TEST(BufferBitsAndBytesOperations, WriteAndReadBytes) {
    //setup data
    IntegralTypes data;
    data.a = -4894541654564;
    data.b = 94545646;
    data.c = -8778;
    data.d = 200;
    data.e = -98;
    data.f[0] = 43;
    data.f[1] = -45;

    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};
    bw.writeBytes<4>(data.b);
    bw.writeBytes<2>(data.c);
    bw.writeBytes<1>(data.d);
    bw.writeBytes<8>(data.a);
    bw.writeBytes<1>(data.e);
    bw.writeBuffer<1>(data.f, 2);
    bw.flush();
    auto range = bw.getWrittenRange();

    EXPECT_THAT(std::distance(range.begin(), range.end()), Eq(18));
    //read from buffer
    BufferReader br{range};
    IntegralTypes res{};
    EXPECT_THAT(br.readBytes<4>(res.b), Eq(true));
    EXPECT_THAT(br.readBytes<2>(res.c), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.d), Eq(true));
    EXPECT_THAT(br.readBytes<8>(res.a), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.e), Eq(true));
    EXPECT_THAT(br.readBuffer<1>(res.f, 2), Eq(true));
    //assert results

    EXPECT_THAT(data.a, Eq(res.a));
    EXPECT_THAT(data.b, Eq(res.b));
    EXPECT_THAT(data.c, Eq(res.c));
    EXPECT_THAT(data.d, Eq(res.d));
    EXPECT_THAT(data.e, Eq(res.e));
    EXPECT_THAT(data.f, ContainerEq(res.f));

}

TEST(BufferBitsAndBytesOperations, ReadWriteBufferFncCanAcceptSignedData) {
    //setup data
    constexpr size_t DATA_SIZE = 3;
    int16_t src[DATA_SIZE] {54,-4877,30067};
    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};
    bw.writeBuffer<2>(src, DATA_SIZE);
    bw.flush();
    //read from buffer
    BufferReader br1{bw.getWrittenRange()};
    int16_t dst[DATA_SIZE]{};
    EXPECT_THAT(br1.readBuffer<2>(dst, DATA_SIZE), Eq(true));
    EXPECT_THAT(dst, ContainerEq(src));

    //read more than available
    BufferReader br2{bw.getWrittenRange()};
    int16_t dstMore[DATA_SIZE+1]{};
    EXPECT_THAT(br2.readBuffer<2>(dstMore, DATA_SIZE+1), Eq(false));
}

TEST(BufferBitsAndBytesOperations, ReadWriteBufferCanWorkOnUnalignedData) {
    //setup data
    constexpr size_t DATA_SIZE = 3;
    int16_t src[DATA_SIZE] {54,-4877,30067};
    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};
    bw.writeBits(15u, 4);
    bw.writeBuffer<2>(src, DATA_SIZE);
    bw.writeBits(12u, 4);
    bw.flush();
    auto range = bw.getWrittenRange();
    EXPECT_THAT(std::distance(range.begin(), range.end()), Eq(sizeof(src) + 1));

    //read from buffer
    BufferReader br1{range};
    int16_t dst[DATA_SIZE]{};
    uint8_t tmp{};
    br1.readBits(tmp, 4);
    EXPECT_THAT(tmp, Eq(15));
    EXPECT_THAT(br1.readBuffer<2>(dst, DATA_SIZE), Eq(true));
    EXPECT_THAT(dst, ContainerEq(src));
    br1.readBits(tmp, 4);
    EXPECT_THAT(tmp, Eq(12));

    //read more than available
    BufferReader br2{range};
    br2.readBits(tmp, 4);
    int16_t dstMore[DATA_SIZE+1]{};
    EXPECT_THAT(tmp, Eq(15));
    EXPECT_THAT(br2.readBuffer<2>(dstMore, DATA_SIZE+1), Eq(false));
}

