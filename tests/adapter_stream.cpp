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
#include <bitsery/adapter/stream.h>
#include <bitsery/adapter_writer.h>
#include <bitsery/adapter_reader.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <sstream>

//some helper types
using Stream = std::stringstream;
using OutputAdapter = bitsery::OutputStreamAdapter;
using InputAdapter = bitsery::InputStreamAdapter ;
using Writer = bitsery::AdapterWriter<bitsery::OutputStreamAdapter, bitsery::DefaultConfig>;
using Reader = bitsery::AdapterReader<bitsery::InputStreamAdapter, bitsery::DefaultConfig>;

static constexpr size_t InternalBufferSize = 128;
using BufferedAdapterInternalBuffer = std::array<char, InternalBufferSize>;
using OutputBufferedAdapter = bitsery::BasicBufferedOutputStreamAdapter<char, std::char_traits<char>, BufferedAdapterInternalBuffer>;
using BufferedWriter = bitsery::AdapterWriter<OutputBufferedAdapter, bitsery::DefaultConfig>;

using testing::Eq;

TEST(AdapterIOStream, CorrectlyReturnsIsCompletedSuccessfully) {
    //setup data
    uint8_t t1 = 111;

    Stream buf{};
    Writer w{{buf}};
    w.writeBytes<1>(t1);
    w.flush();

    Reader r{{buf}};

    uint8_t r1{};
    EXPECT_THAT(r.isCompletedSuccessfully(), Eq(false));
    r.readBytes<1>(r1);
    EXPECT_THAT(r.isCompletedSuccessfully(), Eq(true));
    EXPECT_THAT(r1, Eq(t1));
}

TEST(AdapterIOStream, ReadingMoreThanAvailableReturnsZero) {
    //setup data
    uint8_t t1 = 111;

    Stream buf{};
    Writer w{{buf}};
    w.writeBytes<1>(t1);
    w.flush();

    Reader r{{buf}};

    uint8_t r1{};
    r.readBytes<1>(r1);
    r.readBytes<1>(r1);
    EXPECT_THAT(r1, Eq(0));
}

//this is strange, but probably stringstream doesnt use any of the base methods that sets io_base::iostate flags
TEST(AdapterIOStream, WhenReadingMoreThanAvailableThenDataOverflow) {
    //setup data
    uint8_t t1 = 111;

    Stream buf{};
    Writer w{{buf}};
    w.writeBytes<1>(t1);
    w.flush();

    Reader r{{buf}};

    uint8_t r1{};
    EXPECT_THAT(r.isCompletedSuccessfully(), Eq(false));
    EXPECT_THAT(r.error(), Eq(bitsery::ReaderError::NoError));
    r.readBytes<1>(r1);
    EXPECT_THAT(r.isCompletedSuccessfully(), Eq(true));
    EXPECT_THAT(r.error(), Eq(bitsery::ReaderError::NoError));
    EXPECT_THAT(r1, Eq(t1));
    r.readBytes<1>(r1);
    r.readBytes<1>(r1);
    EXPECT_THAT(r1, Eq(0));
    EXPECT_THAT(r.isCompletedSuccessfully(), Eq(false));
    EXPECT_THAT(r.error(), Eq(bitsery::ReaderError::DataOverflow));

}


template<typename T>
class AdapterBufferedOutputStream : public testing::Test {
public:
    using Buffer = T;
    using Adapter = bitsery::BasicBufferedOutputStreamAdapter<char, std::char_traits<char>, Buffer>;
    using Writer = bitsery::AdapterWriter<Adapter, bitsery::DefaultConfig>;

    static constexpr size_t InternalBufferSize = 128;

    Stream stream{};

    Writer writer{{stream, 128}};
};

using BufferedAdapterInternalBufferTypes = ::testing::Types<
        std::vector<char>,
        std::array<char, 128>,
        std::string
>;

TYPED_TEST_CASE(AdapterBufferedOutputStream, BufferedAdapterInternalBufferTypes);

TYPED_TEST(AdapterBufferedOutputStream, WhenBufferOverflowThenWriteBufferAndRemainingDataToStream) {
    uint8_t x{};
    for (auto i = 0u; i < TestFixture::InternalBufferSize; ++i)
        this->writer.template writeBytes<1>(x);
    EXPECT_TRUE(this->stream.str().empty());
    this->writer.template writeBytes<1>(x);
    EXPECT_THAT(this->stream.str().size(), Eq(TestFixture::InternalBufferSize + 1));
}

TYPED_TEST(AdapterBufferedOutputStream, WhenFlushThenWriteImmediately) {
    uint8_t x{};
    this->writer.template writeBytes<1>(x);
    EXPECT_THAT(this->stream.str().size(), Eq(0));
    this->writer.flush();
    EXPECT_THAT(this->stream.str().size(), Eq(1));
    this->writer.flush();
    EXPECT_THAT(this->stream.str().size(), Eq(1));
}

TYPED_TEST(AdapterBufferedOutputStream, WhenBufferIsStackAllocatedThenBufferSizeViaCtorHasNoEffect) {

    //create writer with half the internal buffer size
    //for std::vector it should overflow, and for std::array it should have no effect
    typename TestFixture::Writer w{{this->stream, TestFixture::InternalBufferSize / 2}};

    uint8_t x{};
    for (auto i = 0u; i < TestFixture::InternalBufferSize; ++i)
        w.template writeBytes<1>(x);
    static constexpr bool ShouldWriteToStream = bitsery::traits::ContainerTraits<typename TestFixture::Buffer>::isResizable;
    EXPECT_THAT(this->stream.str().empty(), ::testing::Ne(ShouldWriteToStream));
}