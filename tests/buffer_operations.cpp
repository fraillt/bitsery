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
#include <bitsery/ext/value_range.h>

using testing::Eq;
using testing::ContainerEq;
using bitsery::BufferWriter;
using bitsery::BufferReader;

using BitPackingWriter = bitsery::BitPackingWriter<bitsery::DefaultConfig>;
using BitPackingReader = bitsery::BitPackingReader<bitsery::DefaultConfig>;
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

TEST(BufferBitsAndBytesOperations, WriteAndReadBitsMaxTypeValues) {
    Buffer buf;
    BufferWriter bw{buf};
    BitPackingWriter bpw{bw};
    bpw.writeBits(std::numeric_limits<uint64_t>::max(), 64);
    bpw.writeBits(std::numeric_limits<uint32_t>::max(), 32);
    bpw.writeBits(std::numeric_limits<uint16_t>::max(), 16);
    bpw.writeBits(std::numeric_limits<uint8_t>::max(), 8);
    bpw.flush();

    BufferReader br{bpw.getWrittenRange()};
    BitPackingReader bpr{br};
    uint64_t v64{};
    uint32_t v32{};
    uint16_t v16{};
    uint8_t v8{};
    bpr.readBits(v64, 64);
    bpr.readBits(v32, 32);
    bpr.readBits(v16, 16);
    bpr.readBits(v8, 8);

    EXPECT_THAT(v64, Eq(std::numeric_limits<uint64_t>::max()));
    EXPECT_THAT(v32, Eq(std::numeric_limits<uint32_t>::max()));
    EXPECT_THAT(v16, Eq(std::numeric_limits<uint16_t>::max()));
    EXPECT_THAT(v8, Eq(std::numeric_limits<uint8_t>::max()));
}

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
    BitPackingWriter bpw{bw};

    bpw.writeBits(data.a, aBITS);
    bpw.writeBits(data.b, bBITS);
    bpw.writeBits(data.c, cBITS);
    bpw.writeBits(data.d, dBITS);
    bpw.writeBits(data.e, eBITS);
    bpw.flush();
    auto range = bpw.getWrittenRange();
    auto bytesCount = ((aBITS + bBITS + cBITS + dBITS + eBITS) / 8) +1 ;
    EXPECT_THAT(std::distance(range.begin(), range.end()), Eq(bytesCount));
    //read from buffer
    BufferReader br{range};
    BitPackingReader bpr{br};

    IntegralUnsignedTypes res{};

    bpr.readBits(res.a, aBITS);
    bpr.readBits(res.b, bBITS);
    bpr.readBits(res.c, cBITS);
    bpr.readBits(res.d, dBITS);
    bpr.readBits(res.e, eBITS);

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
    BitPackingWriter bpw{bw};

    bpw.writeBits(7u,3);
    bpw.flush();
    auto range = bpw.getWrittenRange();
    EXPECT_THAT(std::distance(range.begin(), range.end()), Eq(1));

    //read from buffer
    BufferReader br{range};
    BitPackingReader bpr{br};
    uint16_t tmp;
    bpr.readBits(tmp,4);
    bpr.readBits(tmp,2);
    bpr.readBits(tmp,2);
    EXPECT_THAT(bpr.getError(), Eq(bitsery::BufferReaderError::NO_ERROR));
    bpr.readBits(tmp,2);
    EXPECT_THAT(bpr.getError(), Eq(bitsery::BufferReaderError::BUFFER_OVERFLOW));//false

    //part of next byte
    BufferReader br1{range};
    BitPackingReader bpr1{br1};
    bpr1.readBits(tmp,2);
    EXPECT_THAT(bpr1.getError(), Eq(bitsery::BufferReaderError::NO_ERROR));
    bpr1.readBits(tmp,7);
    EXPECT_THAT(bpr1.getError(), Eq(bitsery::BufferReaderError::BUFFER_OVERFLOW));//false

    //bigger than byte
    BufferReader br2{range};
    BitPackingReader bpr2{br2};
    bpr2.readBits(tmp,9);
    EXPECT_THAT(bpr2.getError(), Eq(bitsery::BufferReaderError::BUFFER_OVERFLOW));//false
}

TEST(BufferBitsAndBytesOperations, ConsecutiveCallsToAlignHasNoEffect) {
    Buffer buf;
    BufferWriter bw{buf};
    BitPackingWriter bpw{bw};

    bpw.writeBits(3u, 2);
    //3 calls to align after 1st data
    bpw.align();
    bpw.align();
    bpw.align();
    bpw.writeBits(7u, 3);
    //1 call to align after 2nd data
    bpw.align();
    bpw.writeBits(15u, 4);
    bpw.flush();

    unsigned char tmp;
    BufferReader br{bpw.getWrittenRange()};
    BitPackingReader bpr{br};
    bpr.readBits(tmp,2);
    EXPECT_THAT(tmp, Eq(3u));
    bpr.align();
    EXPECT_THAT(bpr.getError(), Eq(bitsery::BufferReaderError::NO_ERROR));
    bpr.readBits(tmp,3);
    bpr.align();
    bpr.align();
    bpr.align();
    EXPECT_THAT(tmp, Eq(7u));
    EXPECT_THAT(bpr.getError(), Eq(bitsery::BufferReaderError::NO_ERROR));

    bpr.readBits(tmp,4);
    EXPECT_THAT(tmp, Eq(15u));
    EXPECT_THAT(bpr.getError(), Eq(bitsery::BufferReaderError::NO_ERROR));
}

TEST(BufferBitsAndBytesOperations, AlignWritesZerosBits) {
    //setup data

    //create and write to buffer
    Buffer buf;
    BufferWriter bw{buf};
    BitPackingWriter bpw{bw};

    //write 2 bits and align
    bpw.writeBits(3u, 2);
    bpw.align();
    bpw.flush();
    auto range = bpw.getWrittenRange();
    EXPECT_THAT(std::distance(range.begin(), range.end()), Eq(1));
    unsigned char tmp;
    BufferReader br1{range};
    BitPackingReader bpr1{br1};
    bpr1.readBits(tmp,2);
    //read aligned bits
    bpr1.readBits(tmp,6);
    EXPECT_THAT(tmp, Eq(0));

    BufferReader br2{range};
    BitPackingReader bpr2{br2};
    //read 2 bits
    bpr2.readBits(tmp,2);
    bpr2.align();
    EXPECT_THAT(bpr2.getError(), Eq(bitsery::BufferReaderError::NO_ERROR));
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
    br.readBytes<4>(res.b);
    br.readBytes<2>(res.c);
    br.readBytes<1>(res.d);
    br.readBytes<8>(res.a);
    br.readBytes<1>(res.e);
    br.readBuffer<1>(res.f, 2);
    EXPECT_THAT(br.getError(), Eq(bitsery::BufferReaderError::NO_ERROR));
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
    br1.readBuffer<2>(dst, DATA_SIZE);
    EXPECT_THAT(br1.getError(), Eq(bitsery::BufferReaderError::NO_ERROR));
    EXPECT_THAT(dst, ContainerEq(src));
}

TEST(BufferBitsAndBytesOperations, ReadWriteBufferCanWorkOnUnalignedData) {
    //setup data
    constexpr size_t DATA_SIZE = 3;
    int16_t src[DATA_SIZE] {54,-4877,30067};
    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};
    BitPackingWriter bpw{bw};
    bpw.writeBits(15u, 4);
    bpw.writeBuffer<2>(src, DATA_SIZE);
    bpw.writeBits(12u, 4);
    bpw.flush();
    auto range = bpw.getWrittenRange();
    EXPECT_THAT(std::distance(range.begin(), range.end()), Eq(sizeof(src) + 1));

    //read from buffer
    BufferReader br1{range};
    BitPackingReader bpr1{br1};
    int16_t dst[DATA_SIZE]{};
    uint8_t tmp{};
    bpr1.readBits(tmp, 4);
    EXPECT_THAT(tmp, Eq(15));
    bpr1.readBuffer<2>(dst, DATA_SIZE);
    EXPECT_THAT(bpr1.getError(), Eq(bitsery::BufferReaderError::NO_ERROR));
    EXPECT_THAT(dst, ContainerEq(src));
    bpr1.readBits(tmp, 4);
    EXPECT_THAT(tmp, Eq(12));
}

TEST(BufferBitsAndBytesOperations, RegressionTestReadBytesAfterReadBitsWithLotsOfZeroBits) {
    //setup data
    int16_t data[2]{0x0000, 0x7FFF};
    int16_t res[2]{};
    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};
    BitPackingWriter bpw{bw};
    bpw.writeBits(2u, 2);
    bpw.writeBytes<2>(data[0]);
    bpw.writeBytes<2>(data[1]);
    bpw.align();
    bpw.flush();
    auto range = bpw.getWrittenRange();

    //read from buffer
    BufferReader br{range};
    BitPackingReader bpr{br};
    uint8_t tmp{};
    bpr.readBits(tmp, 2);
    EXPECT_THAT(tmp, Eq(2));
    bpr.readBytes<2>(res[0]);
    bpr.readBytes<2>(res[1]);
    bpr.align();
    EXPECT_THAT(res[0], Eq(data[0]));
    EXPECT_THAT(res[1], Eq(data[1]));
}


