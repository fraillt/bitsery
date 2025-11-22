#include <gmock/gmock.h>
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/ext/direct_write_buffer.h>
#include <bitsery/traits/vector.h>
#include "serialization_test_utils.h"
#include <vector>

// Avoid collision with Buffer defined in serialization_test_utils.h
using TestBuffer = std::vector<uint8_t>;
using OutputAdapter = bitsery::OutputAlignedBufferAdapter<TestBuffer, 16>;
using InputAdapter = bitsery::InputAlignedBufferAdapter<TestBuffer, 16>;

TEST(SerializeExtDirectWriteBuffer, WriteAlignedAndVerify) {
    TestBuffer buf{};
    buf.reserve(100);
    
    uint32_t dataSize = 32;
    
    struct MyObject {
    } obj;

    OutputAdapter adapter(buf);
    bitsery::Serializer<OutputAdapter> ser(std::move(adapter));

    ser.ext(obj, bitsery::ext::DirectWriteBuffer<uint32_t, 16>{dataSize},
            [](auto& s, const MyObject& o, uint8_t* dst, uint32_t size) {
                (void)s;
                (void)o;
                EXPECT_TRUE(dst != nullptr);
                EXPECT_EQ(reinterpret_cast<uintptr_t>(dst) % 16, static_cast<uintptr_t>(0));
                
                for(uint32_t i=0; i<size; ++i) {
                    dst[i] = static_cast<uint8_t>(i);
                }
            });
    
    auto writtenSize = ser.adapter().writtenBytesCount();
    
    // Verify by reading back manually
    InputAdapter inputAdapter(buf.begin(), writtenSize);
    bitsery::Deserializer<InputAdapter> des(std::move(inputAdapter));
    
    uint32_t readSize = 0;
    des.ext(obj, bitsery::ext::DirectWriteBuffer<uint32_t, 16>{readSize},
             [](auto& d, MyObject& o, auto& reader) {
                 (void)d;
                 (void)o;
                 
                 size_t curr = reader.currentReadPos();
                 
                 size_t alignment = 16;
                 size_t padding = (alignment - (curr % alignment)) % alignment;
                 
                 EXPECT_EQ(padding, static_cast<size_t>(12));
                 
                 // Read padding
                 std::vector<uint8_t> pad(padding);
                 reader.template readBuffer<1>(pad.data(), padding);
                 
                 // Now we are at data
                 std::vector<uint8_t> data(32);
                 reader.template readBuffer<1>(data.data(), 32);
                 
                 for(uint32_t i=0; i<32; ++i) {
                     EXPECT_EQ(data[i], static_cast<uint8_t>(i));
                 }
             });
             
    EXPECT_EQ(readSize, dataSize);
}

TEST(SerializeExtDirectWriteBuffer, AlignmentAssert) {
    TestBuffer buf;
    OutputAdapter adapter(buf);
}