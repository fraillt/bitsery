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

IntegralTypes getInitializedIntegralTypes() {
    IntegralTypes data;
    data.a = -4894541654564;
    data.b = 94545646;
    data.c = -8778;
    data.d = 200;
    data.e = -98;
    data.f[0] = 43;
    data.f[1] = -45;
    return data;
}

void writeIntegralTypesToBuffer(BufferWriter& bw, const IntegralTypes& data) {
    bw.writeBytes<4>(data.b);
    bw.writeBytes<1>(data.f[0]);
    bw.writeBytes<2>(data.c);
    bw.writeBytes<1>(data.d);
    bw.writeBytes<8>(data.a);
    bw.writeBytes<1>(data.e);
    bw.writeBytes<1>(data.f[1]);
}


TEST(BufferBytesOperations, WriteAndReadBytes) {
    //setup data
    auto data =getInitializedIntegralTypes();
    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};
    writeIntegralTypesToBuffer(bw, data);

    EXPECT_THAT(std::distance(buf.begin(), buf.end()), Eq(18));
    //read from buffer
    BufferReader br{buf};
    IntegralTypes res{};
    EXPECT_THAT(br.readBytes<4>(res.b), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.f[0]), Eq(true));
    EXPECT_THAT(br.readBytes<2>(res.c), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.d), Eq(true));
    EXPECT_THAT(br.readBytes<8>(res.a), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.e), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.f[1]), Eq(true));
    //assert results

    EXPECT_THAT(data.a, Eq(res.a));
    EXPECT_THAT(data.b, Eq(res.b));
    EXPECT_THAT(data.c, Eq(res.c));
    EXPECT_THAT(data.d, Eq(res.d));
    EXPECT_THAT(data.e, Eq(res.e));
    EXPECT_THAT(data.f, ContainerEq(res.f));

}

TEST(BufferBytesOperations, BufferReaderUsingDataPlusSizeCtor) {
    //setup data
    auto data =getInitializedIntegralTypes();
    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};
    writeIntegralTypesToBuffer(bw, data);

    EXPECT_THAT(std::distance(buf.begin(), buf.end()), Eq(18));
    //read from buffer
    BufferReader br{buf.data(), buf.size()};
    IntegralTypes res{};
    EXPECT_THAT(br.readBytes<4>(res.b), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.f[0]), Eq(true));
    EXPECT_THAT(br.readBytes<2>(res.c), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.d), Eq(true));
    EXPECT_THAT(br.readBytes<8>(res.a), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.e), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.f[1]), Eq(true));
    //assert results

    EXPECT_THAT(data.a, Eq(res.a));
    EXPECT_THAT(data.b, Eq(res.b));
    EXPECT_THAT(data.c, Eq(res.c));
    EXPECT_THAT(data.d, Eq(res.d));
    EXPECT_THAT(data.e, Eq(res.e));
    EXPECT_THAT(data.f, ContainerEq(res.f));
}

TEST(BufferBytesOperations, BufferReaderUsingIteratorsCtor) {
    //setup data
    auto data =getInitializedIntegralTypes();
    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};
    writeIntegralTypesToBuffer(bw, data);

    ASSERT_THAT(std::distance(buf.begin(), buf.end()), Eq(18));
    //read from buffer
    BufferReader br{buf.begin(), buf.end()};
    IntegralTypes res{};
    EXPECT_THAT(br.readBytes<4>(res.b), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.f[0]), Eq(true));
    EXPECT_THAT(br.readBytes<2>(res.c), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.d), Eq(true));
    EXPECT_THAT(br.readBytes<8>(res.a), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.e), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.f[1]), Eq(true));
    //assert results

    EXPECT_THAT(data.a, Eq(res.a));
    EXPECT_THAT(data.b, Eq(res.b));
    EXPECT_THAT(data.c, Eq(res.c));
    EXPECT_THAT(data.d, Eq(res.d));
    EXPECT_THAT(data.e, Eq(res.e));
    EXPECT_THAT(data.f, ContainerEq(res.f));
}



TEST(BufferBytesOperations, ReadReturnsFalseIfNotEnoughBufferSize) {
    //setup data
    uint8_t a = 111;

    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};

    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    bw.writeBytes<1>(a);
    //read from buffer
    BufferReader br{buf};
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


TEST(BufferBytesOperations, ReadIsCompletedWhenAllBytesAreRead) {
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
    //read from buffer
    BufferReader br{buf};
    IntegralTypes res;
    EXPECT_THAT(br.readBytes<4>(res.b), Eq(true));
    EXPECT_THAT(br.readBytes<2>(res.c), Eq(true));
    EXPECT_THAT(br.isCompleted(), Eq(false));
    EXPECT_THAT(br.readBytes<1>(res.d), Eq(true));
    EXPECT_THAT(br.isCompleted(), Eq(true));
    EXPECT_THAT(br.readBytes<1>(res.d), Eq(false));
    EXPECT_THAT(br.isCompleted(), Eq(true));

    BufferReader br1{buf};
    EXPECT_THAT(br1.readBytes<4>(res.b), Eq(true));
    EXPECT_THAT(br1.readBytes<2>(res.c), Eq(true));
    EXPECT_THAT(br1.isCompleted(), Eq(false));
    EXPECT_THAT(br1.readBytes<2>(res.c), Eq(false));
    EXPECT_THAT(br1.isCompleted(), Eq(false));
    EXPECT_THAT(br1.readBytes<1>(res.d), Eq(true));
    EXPECT_THAT(br1.isCompleted(), Eq(true));

}

TEST(BufferBytesOperations, ReadWriteBufferFncCanAcceptSignedData) {
    //setup data
    constexpr size_t DATA_SIZE = 3;
    int16_t src[DATA_SIZE] {54,-4877,30067};
    //create and write to buffer
    Buffer buf{};
    BufferWriter bw{buf};
    bw.writeBuffer<2>(src, DATA_SIZE);
    bw.flush();
    //read from buffer
    BufferReader br1{buf};
    int16_t dst[DATA_SIZE]{};
    EXPECT_THAT(br1.readBuffer<2>(dst, DATA_SIZE), Eq(true));
    EXPECT_THAT(dst, ContainerEq(src));

    //read more than available
    BufferReader br2{buf};
    int16_t dstMore[DATA_SIZE+1]{};
    EXPECT_THAT(br2.readBuffer<2>(dstMore, DATA_SIZE+1), Eq(false));
}

TEST(BufferBytesOperations, ReadWriteBufferCanWorkOnUnalignedData) {
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
    EXPECT_THAT(buf.size(), Eq(sizeof(src) + 1));

    //read from buffer
    BufferReader br1{buf};
    int16_t dst[DATA_SIZE]{};
    uint8_t tmp{};
    br1.readBits(tmp, 4);
    EXPECT_THAT(tmp, Eq(15));
    EXPECT_THAT(br1.readBuffer<2>(dst, DATA_SIZE), Eq(true));
    EXPECT_THAT(dst, ContainerEq(src));
    br1.readBits(tmp, 4);
    EXPECT_THAT(tmp, Eq(12));

    //read more than available
    BufferReader br2{buf};
    br2.readBits(tmp, 4);
    int16_t dstMore[DATA_SIZE+1]{};
    EXPECT_THAT(tmp, Eq(15));
    EXPECT_THAT(br2.readBuffer<2>(dstMore, DATA_SIZE+1), Eq(false));
}

