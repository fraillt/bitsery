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
#include <bitsery/traits/array.h>
#include <bitsery/traits/vector.h>

using testing::Eq;
using testing::ContainerEq;
using bitsery::EndiannessType;
using bitsery::DefaultConfig;
using Buffer = bitsery::DefaultConfig::BufferType;

struct FixedBufferConfig: public DefaultConfig {
    using BufferType = std::array<uint8_t, 100>;
};

struct NonFixedBufferConfig: public DefaultConfig  {
    using BufferType = std::vector<uint8_t>;
};

template <typename Config>
class BufferWriting:public testing::Test {
public:
    using type = Config;

};

using BufferWritingConfigs = ::testing::Types<
        NonFixedBufferConfig,
        FixedBufferConfig>;

TYPED_TEST_CASE(BufferWriting, BufferWritingConfigs);

static constexpr size_t DATA_SIZE = 14u;

template <typename BW>
void writeData(BW& bw) {
    uint16_t tmp1{45}, tmp2{6543}, tmp3{46533};
    uint32_t tmp4{8979445}, tmp5{7987564};
    bw.template writeBytes<2>(tmp1);
    bw.template writeBytes<2>(tmp2);
    bw.template writeBytes<2>(tmp3);
    bw.template writeBytes<4>(tmp4);
    bw.template writeBytes<4>(tmp5);
}

TYPED_TEST(BufferWriting, GetWrittenRangeReturnsBeginEndIterators) {
    using Config = typename TestFixture::type;
    using Buffer = typename Config::BufferType;
    Buffer buf{};
    bitsery::BasicBufferWriter<Config> bw{buf};
    writeData(bw);
    bw.flush();
    auto range = bw.getWrittenRange();
    EXPECT_THAT(std::distance(range.begin(), range.end()), DATA_SIZE);
}

TYPED_TEST(BufferWriting, WhenWritingBitsThenMustFlushWriter) {
    using Config = typename TestFixture::type;
    using Buffer = typename Config::BufferType;
    Buffer buf{};
    bitsery::BasicBufferWriter<Config> bw{buf};
    bw.writeBits(3u, 2);
    auto range1 = bw.getWrittenRange();
    bw.flush();
    auto range2 = bw.getWrittenRange();
    EXPECT_THAT(std::distance(range1.begin(), range1.end()), Eq(0));
    EXPECT_THAT(std::distance(range2.begin(), range2.end()), Eq(1));
}

TYPED_TEST(BufferWriting, WhenDataAlignedThenFlushHasNoEffect) {
    using Config = typename TestFixture::type;
    using Buffer = typename Config::BufferType;
    Buffer buf{};
    bitsery::BasicBufferWriter<Config> bw{buf};

    bw.writeBits(3u, 2);
    bw.align();
    auto range1 = bw.getWrittenRange();
    bw.flush();
    auto range2 = bw.getWrittenRange();
    EXPECT_THAT(std::distance(range1.begin(), range1.end()), Eq(1));
    EXPECT_THAT(std::distance(range2.begin(), range2.end()), Eq(1));
}

TEST(BufferWrittingNonFixedBuffer, BufferIsAlwaysResizedToCapacity) {
    using Buffer = typename NonFixedBufferConfig::BufferType;
    Buffer buf{};
    bitsery::BasicBufferWriter<NonFixedBufferConfig> bw{buf};
    for (auto i = 0; i < 5; ++i) {
        uint32_t tmp{};
        bw.writeBytes<4>(tmp);
        bw.writeBytes<4>(tmp);
        EXPECT_TRUE(buf.size() == buf.capacity());
    }
}
