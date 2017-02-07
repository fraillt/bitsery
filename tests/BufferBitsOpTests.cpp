//
// Created by Mindaugas Vinkelis on 2017-01-10.
//
#include <gmock/gmock.h>
#include "BufferWriter.h"
#include "BufferReader.h"

using testing::Eq;
using testing::ContainerEq;

#include <list>
#include <bitset>


struct IntegralUnsignedTypes {
    uint32_t a;
    uint16_t b;
    uint8_t c;
    uint8_t d;
    uint64_t e;
};

TEST(BufferBitsOperations, WriteAndReadBits) {
    //setup data
    IntegralUnsignedTypes data;
    data.a = 485454;//bits 19
    data.b = 45978;//bits 16
    data.c = 0;//bits 1
    data.d = 36;//bits 6
    data.e = 479845648946;//bits 39

    constexpr size_t aBITS = 21;
    constexpr size_t bBITS = 16;
    constexpr size_t cBITS = 5;
    constexpr size_t dBITS = 7;
    constexpr size_t eBITS = 40;

    //create and write to buffer
    std::vector<uint8_t> buf;
    BufferWriter bw{buf};

    bw.WriteBits<aBITS>(data.a);
    bw.WriteBits<bBITS>(data.b);
    bw.WriteBits<cBITS>(data.c);
    bw.WriteBits<dBITS>(data.d);
    bw.WriteBits<eBITS>(data.e);
    bw.Flush();
    auto bytesCount = ((aBITS + bBITS + cBITS + dBITS + eBITS) / 8) +1 ;
    EXPECT_THAT(std::distance(buf.begin(), buf.end()), Eq(bytesCount));
    //read from buffer
    BufferReader br{buf};
    IntegralUnsignedTypes res;

    br.ReadBits<aBITS>(res.a);
    br.ReadBits<bBITS>(res.b);
    br.ReadBits<cBITS>(res.c);
    br.ReadBits<dBITS>(res.d);
    br.ReadBits<eBITS>(res.e);

    EXPECT_THAT(res.a, Eq(data.a));
    EXPECT_THAT(res.b, Eq(data.b));
    EXPECT_THAT(res.c, Eq(data.c));
    EXPECT_THAT(res.d, Eq(data.d));
    EXPECT_THAT(res.e, Eq(data.e));

}

TEST(BufferBitsOperations, WhenFinishedFlushWriter) {

    std::vector<uint8_t> buf;
    BufferWriter bw{buf};

    bw.WriteBits<2>(0xFFu);
    EXPECT_THAT(std::distance(buf.begin(), buf.end()), Eq(0));
    bw.Flush();
    EXPECT_THAT(std::distance(buf.begin(), buf.end()), Eq(1));

}

TEST(BufferBitsOperations, BufferSizeIsCountedPerByteNotPerBit) {
    //setup data

    //create and write to buffer
    std::vector<uint8_t> buf;
    BufferWriter bw{buf};

    bw.WriteBits<2>(0xFFu);
    bw.Flush();
    EXPECT_THAT(std::distance(buf.begin(), buf.end()), Eq(1));

    //read from buffer
    BufferReader br{buf};
    unsigned tmp;
    EXPECT_THAT(br.ReadBits<4>(tmp), Eq(true));
    EXPECT_THAT(br.ReadBits<2>(tmp), Eq(true));
    EXPECT_THAT(br.ReadBits<2>(tmp), Eq(true));
    EXPECT_THAT(br.ReadBits<2>(tmp), Eq(false));

    //part of next byte
    BufferReader br1{buf};
    EXPECT_THAT(br1.ReadBits<2>(tmp), Eq(true));
    EXPECT_THAT(br1.ReadBits<7>(tmp), Eq(false));

    //bigger than byte
    BufferReader br2{buf};
    EXPECT_THAT(br2.ReadBits<9>(tmp), Eq(false));
}
