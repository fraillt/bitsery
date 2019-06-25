//MIT License
//
//Copyright (c) 2019 Mindaugas Vinkelis
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


#include <bitsery/adapter/buffer.h>
#include <bitsery/adapter_writer.h>
#include <bitsery/adapter_reader.h>
#include <bitsery/traits/vector.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <gmock/gmock.h>
#include <bitsery/adapter/stream.h>

//some helper types
using Buffer = std::vector<char>;
using OutputAdapter = bitsery::OutputBufferAdapter<Buffer>;
using InputAdapter = bitsery::InputBufferAdapter<Buffer>;
using Writer = bitsery::AdapterWriter<OutputAdapter, bitsery::DefaultConfig>;
using Reader = bitsery::AdapterReader<InputAdapter, bitsery::DefaultConfig>;

using bitsery::ReaderError;

using testing::Eq;
using testing::Ge;

TEST(OutputBuffer, WhenInitialBufferIsEmptyThenResizeInAdapterConstructor) {
    //setup data
    Buffer buf{};
    EXPECT_THAT(buf.size(), Eq(0));
    OutputAdapter adapter{buf};
    EXPECT_THAT(buf.size(), Ge(1));
}

TEST(OutputBuffer, WhenSetWritePositionThenResizeUnderlyingBufferIfRequired) {
    //setup data
    Buffer buf{};
    Writer w{buf};
    const auto initialSize = buf.size();
    EXPECT_THAT(buf.size(), Eq(initialSize));
    EXPECT_THAT(w.currentWritePos(), Eq(0));
    w.currentWritePos(initialSize + 10);
    EXPECT_THAT(w.currentWritePos(), Eq(initialSize + 10));
    EXPECT_THAT(buf.size(), Ge(initialSize + 10));
}

TEST(OutputBuffer, WhenSettingCurrentPositionBeforeBufferEndThenWrittenBytesCountIsNotAffected) {
    //setup data
    Buffer buf{};
    Writer w{buf};
    const auto initialSize = buf.size();
    EXPECT_THAT(buf.size(), Eq(initialSize));
    EXPECT_THAT(w.writtenBytesCount(), Eq(0));
    w.currentWritePos(initialSize + 10);
    w.writeBytes<8>(static_cast<uint64_t>(1));
    EXPECT_THAT(w.writtenBytesCount(), Eq(initialSize + 10 + 8));
    w.currentWritePos(0);
    EXPECT_THAT(w.writtenBytesCount(), Eq(initialSize + 10 + 8));
}

TEST(InputBuffer, CorrectlySetsAndGetsCurrentReadPosition) {

    Buffer buf{};
    buf.resize(100);
    Reader r{{buf.begin(), 10}};
    r.currentReadPos(5);
    EXPECT_THAT(r.currentReadPos(), Eq(5));
    r.currentReadPos(0);
    EXPECT_THAT(r.currentReadPos(), Eq(0));
    uint8_t tmp;
    r.readBytes<1>(tmp);
    EXPECT_THAT(r.currentReadPos(), Eq(1));
}


TEST(InputBuffer, WhenSetReadPositionOutOfRangeThenDataOverflow) {

    Buffer buf{};
    buf.resize(100);
    Reader r{{buf.begin(), 10}};
    r.currentReadPos(10);
    EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
    r.currentReadPos(11);
    EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
}

TEST(InputBuffer, WhenSetReadEndPositionOutOfRangeThenDataOverflow) {
    Buffer buf{};
    buf.resize(100);
    Reader r{{buf.begin(), 10}};
    r.currentReadEndPos(11);
    EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
}

TEST(InputBuffer, WhenReadEndPositionIsNotSetThenReturnZeroAsBufferEndPosition) {
    Buffer buf{};
    buf.resize(100);
    Reader r{{buf.begin(), 10}};
    EXPECT_THAT(r.currentReadEndPos(), Eq(0));
    r.currentReadEndPos(5);
    EXPECT_THAT(r.currentReadEndPos(), Eq(5));
    EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
}


TEST(InputBuffer, WhenReadEndPositionIsNotZeroThenDataOverflowErrorWillBeIgnored) {
    Buffer buf{};
    buf.resize(100);
    Reader r{{buf.begin(), 1}};
    r.currentReadEndPos(1);
    uint32_t tmp{};
    r.readBytes<4>(tmp);
    EXPECT_THAT(tmp, Eq(0));
    EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
    r.currentReadEndPos(0);
    r.readBytes<4>(tmp);
    EXPECT_THAT(tmp, Eq(0));
    EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
}


TEST(InputBuffer, WhenReadingPastReadEndPositionOrBufferEndThenReadPositionDoesntChange) {
    Buffer buf{};
    buf.resize(10);
    Reader r{{buf.begin(), 3}};
    uint32_t tmp{};
    r.currentReadEndPos(2);
    r.readBytes<4>(tmp);
    EXPECT_THAT(r.currentReadPos(), Eq(0));
    EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
    EXPECT_THAT(tmp, Eq(0));
    r.currentReadEndPos(0);
    r.readBytes<4>(tmp);
    EXPECT_THAT(r.currentReadPos(), Eq(0));
    EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
    EXPECT_THAT(tmp, Eq(0));
}

TEST(InputBuffer, WhenReaderHasErrorsThenSettingReadPosAndReadEndPosIsIgnoredAndGettingAlwaysReturnsZero) {
    Buffer buf{};
    buf.resize(10);
    Reader r{{buf.begin(), 10}};
    uint32_t tmp{};
    r.readBytes<4>(tmp);
    r.currentReadEndPos(5);
    EXPECT_THAT(r.currentReadPos(), Eq(4));
    EXPECT_THAT(r.currentReadEndPos(), Eq(5));
    EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
    r.currentReadEndPos(11);
    EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));
    EXPECT_THAT(r.currentReadPos(), Eq(0));
    EXPECT_THAT(r.currentReadEndPos(), Eq(0));
    r.currentReadPos(1);
    r.currentReadEndPos(1);
    EXPECT_THAT(r.currentReadPos(), Eq(0));
    EXPECT_THAT(r.currentReadEndPos(), Eq(0));
}

TEST(InputBuffer, ConstDataForBufferAllAdapters) {
    //create and write to buffer
    uint16_t data = 7549;
    Buffer bufWrite{};
    Writer bw{bufWrite};
    bw.writeBytes<2>(data);
    bw.flush();
    const Buffer buf{bufWrite};

    //read from buffer
    using Adapter1 = bitsery::InputBufferAdapter<const Buffer>;
    using Adapter2 = bitsery::UnsafeInputBufferAdapter<const Buffer>;

    bitsery::AdapterReader<Adapter1, bitsery::DefaultConfig> r1{Adapter1{buf.begin(), buf.end()}};
    bitsery::AdapterReader<Adapter2, bitsery::DefaultConfig> r2{Adapter2{buf.begin(), buf.end()}};

    uint16_t res1{};
    r1.readBytes<2>(res1);

    uint16_t res2{};
    r2.readBytes<2>(res2);
    EXPECT_THAT(res1, Eq(data));
    EXPECT_THAT(res2, Eq(data));
}


template <template<typename...> class TAdapter>
struct BufferConfig {
    using Data = std::vector<char>;
    using Adapter = TAdapter<Data>;
    using Reader = bitsery::AdapterReader<Adapter, bitsery::DefaultConfig>;

    Data data{};
    Reader createReader(const std::vector<char>& buffer) {
        data = buffer;
        return Reader{Adapter{data.begin(), data.size()}};
    }
};

template <typename TAdapter>
struct StreamConfig {
    using Data = std::stringstream;
    using Adapter = TAdapter;
    using Reader = bitsery::AdapterReader<Adapter, bitsery::DefaultConfig>;

    Data data{};
    Reader createReader(const std::vector<char>& buffer) {
        std::string str(buffer.begin(), buffer.end());
        data = std::stringstream{str};
        return Reader{Adapter{data}};
    }
};

template<typename TAdapterWithData>
class AdapterConfig : public testing::Test {
public:

    TAdapterWithData config{};
};

using AdapterInputTypes = ::testing::Types<
    BufferConfig<bitsery::InputBufferAdapter>,
    BufferConfig<bitsery::UnsafeInputBufferAdapter>,
    StreamConfig<bitsery::InputStreamAdapter>
>;

template <typename TConfig>
class InputAll: public AdapterConfig<TConfig> {
};

TYPED_TEST_CASE(InputAll, AdapterInputTypes);


using AdapterInputSafeOnlyTypes = ::testing::Types<
    BufferConfig<bitsery::InputBufferAdapter>,
    StreamConfig<bitsery::InputStreamAdapter>
>;

template <typename TConfig>
class InputSafeOnly: public AdapterConfig<TConfig> {
};

TYPED_TEST_CASE(InputSafeOnly, AdapterInputSafeOnlyTypes);


TYPED_TEST(InputAll, SettingMultipleErrorsAlwaysReturnsFirstError) {
    auto r = this->config.createReader({0,0,0,0});
    EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
    r.error(ReaderError::InvalidPointer);
    EXPECT_THAT(r.error(), Eq(ReaderError::InvalidPointer));
    r.error(ReaderError::DataOverflow);
    EXPECT_THAT(r.error(), Eq(ReaderError::InvalidPointer));
    r.error(ReaderError::NoError);
    EXPECT_THAT(r.error(), Eq(ReaderError::InvalidPointer));
}


TYPED_TEST(InputAll, WhenAlignHasNonZerosThenInvalidDataError) {

    auto r = this->config.createReader({0x7F});
    bitsery::AdapterReaderBitPackingWrapper<decltype(r)> bpr{r};

    uint8_t tmp{0xFF};
    bpr.readBits(tmp,3);
    bpr.align();
    EXPECT_THAT(bpr.error(), Eq(ReaderError::InvalidData));
}


TYPED_TEST(InputAll, WhenAllBytesAreReadWithoutErrorsThenIsCompletedSuccessfully) {
    //setup data

    uint32_t tb = 94545646;
    int16_t tc = -8778;
    uint8_t td = 200;

    //create and write to buffer
    Buffer buf{};
    Writer bw{buf};

    bw.writeBytes<4>(tb);
    bw.writeBytes<2>(tc);
    bw.writeBytes<1>(td);
    bw.flush();
    buf.resize(bw.writtenBytesCount());

    auto br = this->config.createReader(buf);

    uint32_t rb = 94545646;
    int16_t rc = -8778;
    uint8_t rd = 200;

    br.template readBytes<4>(rb);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
    br.template readBytes<2>(rc);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
    EXPECT_THAT(br.isCompletedSuccessfully(), Eq(false));
    br.template readBytes<1>(rd);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
    EXPECT_THAT(br.isCompletedSuccessfully(), Eq(true));

    EXPECT_THAT(rb, Eq(tb));
    EXPECT_THAT(rc, Eq(tc));
    EXPECT_THAT(rd, Eq(td));
}

TYPED_TEST(InputSafeOnly, WhenAllBytesAreReadWithoutErrorsThenIsCompletedSuccessfully) {
    //setup data

    uint32_t tb = 94545646;
    int16_t tc = -8778;
    uint8_t td = 200;

    //create and write to buffer
    Buffer buf{};
    Writer bw{buf};

    bw.writeBytes<4>(tb);
    bw.writeBytes<2>(tc);
    bw.writeBytes<1>(td);
    bw.flush();
    buf.resize(bw.writtenBytesCount());

    auto br = this->config.createReader(buf);

    uint32_t rb = 94545646;
    int16_t rc = -8778;
    uint8_t rd = 200;

    br.template readBytes<4>(rb);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
    br.template readBytes<2>(rc);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
    EXPECT_THAT(br.isCompletedSuccessfully(), Eq(false));
    br.template readBytes<1>(rd);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::NoError));
    EXPECT_THAT(br.isCompletedSuccessfully(), Eq(true));
    br.template readBytes<1>(rd);
    EXPECT_THAT(br.error(), Eq(bitsery::ReaderError::DataOverflow));
    EXPECT_THAT(br.isCompletedSuccessfully(), Eq(false));

    Reader br1{InputAdapter{buf.begin(), bw.writtenBytesCount()}};
    br1.template readBytes<4>(rb);
    EXPECT_THAT(br1.error(), Eq(bitsery::ReaderError::NoError));
    br1.template readBytes<2>(rc);
    EXPECT_THAT(br1.error(), Eq(bitsery::ReaderError::NoError));
    EXPECT_THAT(br1.isCompletedSuccessfully(), Eq(false));
    br1.template readBytes<2>(rc);
    EXPECT_THAT(br1.error(), Eq(bitsery::ReaderError::DataOverflow));
    EXPECT_THAT(br1.isCompletedSuccessfully(), Eq(false));
    br1.template readBytes<1>(rd);
    EXPECT_THAT(br1.error(), Eq(bitsery::ReaderError::DataOverflow));
    EXPECT_THAT(br1.isCompletedSuccessfully(), Eq(false));
}


TYPED_TEST(InputSafeOnly, WhenReadingMoreThanAvailableThenDataOverflow) {
    //setup data
    uint8_t t1 = 111;

    Buffer buf{};
    Writer w{buf};
    w.writeBytes<1>(t1);
    w.flush();
    buf.resize(w.writtenBytesCount());

    auto r = this->config.createReader(buf);

    uint8_t r1{};
    EXPECT_THAT(r.isCompletedSuccessfully(), Eq(false));
    EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
    r.template readBytes<1>(r1);
    EXPECT_THAT(r.isCompletedSuccessfully(), Eq(true));
    EXPECT_THAT(r.error(), Eq(ReaderError::NoError));
    EXPECT_THAT(r1, Eq(t1));
    r.template readBytes<1>(r1);
    r.template readBytes<1>(r1);
    EXPECT_THAT(r1, Eq(0));
    EXPECT_THAT(r.isCompletedSuccessfully(), Eq(false));
    EXPECT_THAT(r.error(), Eq(ReaderError::DataOverflow));

}

TYPED_TEST(InputSafeOnly, WhenReaderHasErrorsAllThenReadsReturnZero) {
    //setup data
    uint8_t t1 = 111;

    Buffer buf{};
    Writer w{buf};
    w.writeBytes<1>(t1);
    w.writeBytes<1>(t1);
    w.flush();
    buf.resize(w.writtenBytesCount());

    auto r = this->config.createReader(buf);

    uint8_t r1{};
    r.template readBytes<1>(r1);
    EXPECT_THAT(r1, Eq(t1));
    r.error(ReaderError::InvalidPointer);
    r.template readBytes<1>(r1);
    EXPECT_THAT(r1, Eq(0));
}



template<typename T>
class OutputStreamBuffered : public testing::Test {
public:
    using Buffer = T;
    using Adapter = bitsery::BasicBufferedOutputStreamAdapter<char, std::char_traits<char>, Buffer>;
    using Writer = bitsery::AdapterWriter<Adapter, bitsery::DefaultConfig>;

    static constexpr size_t InternalBufferSize = 128;

    std::stringstream stream{};

    Writer writer{{stream, 128}};
};

using BufferedAdapterInternalBufferTypes = ::testing::Types<
    std::vector<char>,
    std::array<char, 128>,
    std::string
>;

TYPED_TEST_CASE(OutputStreamBuffered, BufferedAdapterInternalBufferTypes);

TYPED_TEST(OutputStreamBuffered, WhenBufferOverflowThenWriteBufferAndRemainingDataToStream) {
    uint8_t x{};
    for (auto i = 0u; i < TestFixture::InternalBufferSize; ++i)
        this->writer.template writeBytes<1>(x);
    EXPECT_TRUE(this->stream.str().empty());
    this->writer.template writeBytes<1>(x);
    EXPECT_THAT(this->stream.str().size(), Eq(TestFixture::InternalBufferSize + 1));
}

TYPED_TEST(OutputStreamBuffered, WhenFlushThenWriteImmediately) {
    uint8_t x{};
    this->writer.template writeBytes<1>(x);
    EXPECT_THAT(this->stream.str().size(), Eq(0));
    this->writer.flush();
    EXPECT_THAT(this->stream.str().size(), Eq(1));
    this->writer.flush();
    EXPECT_THAT(this->stream.str().size(), Eq(1));
}

TYPED_TEST(OutputStreamBuffered, WhenBufferIsStackAllocatedThenBufferSizeViaCtorHasNoEffect) {

    //create writer with half the internal buffer size
    //for std::vector it should overflow, and for std::array it should have no effect
    typename TestFixture::Writer w{{this->stream, TestFixture::InternalBufferSize / 2}};

    uint8_t x{};
    for (auto i = 0u; i < TestFixture::InternalBufferSize; ++i)
        w.template writeBytes<1>(x);
    static constexpr bool ShouldWriteToStream = bitsery::traits::ContainerTraits<typename TestFixture::Buffer>::isResizable;
    EXPECT_THAT(this->stream.str().empty(), ::testing::Ne(ShouldWriteToStream));
}

TEST(AdapterWriterMeasureSize, CorrectlyMeasuresWrittenBytesCountForSerialization) {
    bitsery::MeasureSize w{};
    EXPECT_THAT(w.writtenBytesCount(), Eq(0));
    w.writeBytes<8>(uint64_t{0});
    EXPECT_THAT(w.writtenBytesCount(), Eq(8));
    w.writeBuffer<8, uint64_t>(nullptr, 9);
    EXPECT_THAT(w.writtenBytesCount(), Eq(80));
    w.currentWritePos(10);
    w.writeBytes<4>(uint32_t{0});
    EXPECT_THAT(w.writtenBytesCount(), Eq(80));
    EXPECT_THAT(w.currentWritePos(), Eq(14));
    w.currentWritePos(80);
    EXPECT_THAT(w.writtenBytesCount(), Eq(80));
    w.writeBits(uint32_t{0}, 7u);
    EXPECT_THAT(w.writtenBytesCount(), Eq(80));
    w.align();
    EXPECT_THAT(w.writtenBytesCount(), Eq(81));
    w.writeBits(uint32_t{0}, 7u);
    w.flush();
    EXPECT_THAT(w.writtenBytesCount(), Eq(82));
    // doesn't compile on older compilers if I write bitsery::MeasureSize::BitPackingEnabled directly in EXPECT_THAT macro.
    constexpr bool bpEnabled = bitsery::MeasureSize::BitPackingEnabled;
    EXPECT_THAT(bpEnabled, Eq(true));
}


struct CustomInternalContextConfig: bitsery::DefaultConfig {
    using InternalContext = std::tuple<int, float>;
};

TEST(AdapterWriterMeasureSize, SupportsInternalAndExternalContexts) {
    char extCtx{'A'};
    bitsery::BasicMeasureSize<CustomInternalContextConfig, char> w{extCtx};
    EXPECT_THAT(w.externalContext(), Eq('A'));
    std::tuple<int, float>& tmp = w.internalContext();
}
