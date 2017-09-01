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
using bitsery::EndiannessType;
using bitsery::DefaultConfig;
using Buffer = bitsery::DefaultConfig::BufferType;

constexpr EndiannessType getInverseEndianness(EndiannessType e) {
    return e == EndiannessType::LittleEndian
           ? EndiannessType::BigEndian
           : EndiannessType::LittleEndian;
}

struct InverseEndiannessConfig {
    static constexpr bitsery::EndiannessType NetworkEndianness = getInverseEndianness(DefaultConfig::NetworkEndianness);
    static constexpr bool FixedBufferSize = DefaultConfig::FixedBufferSize;
    using BufferType = DefaultConfig::BufferType;
};

struct IntegralTypes {
    int64_t a;
    uint32_t b;
    int16_t c;
    uint8_t d;
    int8_t e;
};


TEST(BufferEndianness, WhenWriteBytesThenBytesAreSwapped) {
    //fill initial values
    IntegralTypes src{};
    src.a = 0x1122334455667788;
    src.b = 0xBBCCDDEE;
    src.c = 0xCCDD;
    src.d = 0xDD;
    src.e = 0xEE;

    //fill expected result after swap
    IntegralTypes resInv{};
    resInv.a = 0x8877665544332211;
    resInv.b = 0xEEDDCCBB;
    resInv.c = 0xDDCC;
    resInv.d = 0xDD;
    resInv.e = 0xEE;

    //create and write to buffer
    Buffer buf{};
    bitsery::BasicBufferWriter<DefaultConfig> bw{buf};
    bw.writeBytes<8>(src.a);
    bw.writeBytes<4>(src.b);
    bw.writeBytes<2>(src.c);
    bw.writeBytes<1>(src.d);
    bw.writeBytes<1>(src.e);
    bw.flush();
    //read from buffer using inverse endianness config
    bitsery::BasicBufferReader<InverseEndiannessConfig> br{bw.getWrittenRange()};
    IntegralTypes res{};
    br.readBytes<8>(res.a);
    br.readBytes<4>(res.b);
    br.readBytes<2>(res.c);
    br.readBytes<1>(res.d);
    br.readBytes<1>(res.e);
    //check results
    EXPECT_THAT(res.a, Eq(resInv.a));
    EXPECT_THAT(res.b, Eq(resInv.b));
    EXPECT_THAT(res.c, Eq(resInv.c));
    EXPECT_THAT(res.d, Eq(resInv.d));
    EXPECT_THAT(res.e, Eq(resInv.e));
}

TEST(BufferEndianness, WhenWriteBuffer1ByteValuesThenEndiannessIsIgnored) {
    //fill initial values
    constexpr size_t SIZE = 4;
    uint8_t src[SIZE] = {0xAA, 0xBB, 0xCC, 0xDD};
    uint8_t res[SIZE] = {};
    //create and write to buffer
    Buffer buf{};
    bitsery::BasicBufferWriter<DefaultConfig> bw{buf};
    bw.writeBuffer<1>(src, SIZE);
    bw.flush();
    //read from buffer using inverse endianness config
    bitsery::BasicBufferReader<InverseEndiannessConfig> br{bw.getWrittenRange()};
    br.readBuffer<1>(res, SIZE);
    //result is identical, because we write separate values, of size 1byte, that requires no swapping
    //check results
    EXPECT_THAT(res, ContainerEq(src));
}

TEST(BufferEndianness, WhenWriteBufferMoreThan1ByteValuesThenValuesAreSwapped) {
    //fill initial values
    constexpr size_t SIZE = 4;
    uint16_t src[SIZE] = {0xAA00, 0xBB11, 0xCC22, 0xDD33};
    uint16_t resInv[SIZE] = {0x00AA, 0x11BB, 0x22CC, 0x33DD};
    uint16_t res[SIZE] = {};
    //create and write to buffer
    Buffer buf{};
    bitsery::BasicBufferWriter<DefaultConfig> bw{buf};
    bw.writeBuffer<2>(src, SIZE);
    bw.flush();
    //read from buffer using inverse endianness config
    bitsery::BasicBufferReader<InverseEndiannessConfig> br{bw.getWrittenRange()};
    br.readBuffer<2>(res, SIZE);
    //result is identical, because we write separate values, of size 1byte, that requires no swapping
    //check results
    EXPECT_THAT(res, ContainerEq(resInv));
}


template <typename T>
constexpr size_t getBits(T v) {
    return bitsery::details::calcRequiredBits<T>({}, v);
};

struct IntegralUnsignedTypes {
    uint64_t a;
    uint32_t b;
    uint16_t c;
    uint8_t d;
};

TEST(BufferEndianness, WhenBufferValueTypeIs1ByteThenBitOperationsIsNotAffectedByEndianness) {
    //fill initial values
    static_assert(sizeof(DefaultConfig::BufferType::value_type) == 1, "currently only 1 byte size, value size is supported");
    //fill initial values
    constexpr IntegralUnsignedTypes src {
            0x0000334455667788,//bits 19
            0x00CCDDEE,//bits 16
            0x00DD,//bits 1
            0x0F,//bits 6
    };

    constexpr size_t aBITS = getBits(src.a) + 8;
    constexpr size_t bBITS = getBits(src.b) + 0;
    constexpr size_t cBITS = getBits(src.c) + 5;
    constexpr size_t dBITS = getBits(src.d) + 2;
    //create and write to buffer
    Buffer buf{};
    bitsery::BasicBufferWriter<DefaultConfig> bw{buf};
    bw.writeBits(src.a, aBITS);
    bw.writeBits(src.b, bBITS);
    bw.writeBits(src.c, cBITS);
    bw.writeBits(src.d, dBITS);
    bw.flush();
    //read from buffer using inverse endianness config
    bitsery::BasicBufferReader<InverseEndiannessConfig> br{bw.getWrittenRange()};
    IntegralUnsignedTypes res{};
    br.readBits(res.a, aBITS);
    br.readBits(res.b, bBITS);
    br.readBits(res.c, cBITS);
    br.readBits(res.d, dBITS);
    //check results
    EXPECT_THAT(res.a, Eq(src.a));
    EXPECT_THAT(res.b, Eq(src.b));
    EXPECT_THAT(res.c, Eq(src.c));
    EXPECT_THAT(res.d, Eq(src.d));
}
