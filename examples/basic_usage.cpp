#include <bitsery/bitsery.h>
#include <bitsery/adapters/buffer_adapters.h>
#include <bitsery/traits/vector.h>

enum class MyEnum:uint16_t { V1,V2,V3 };
struct MyStruct {
    uint32_t i;
    MyEnum e;
    std::vector<float> fs;
};

//define how object should be serialized/deserialized
template <typename S>
void serialize(S& s, MyStruct& o) {
    s.value4b(o.i);
    s.value2b(o.e);
    s.container4b(o.fs, 10);
};

using namespace bitsery;

using Buffer = std::vector<uint8_t>;
using OutputAdapter = OutputBufferAdapter<Buffer>;
using InputAdapter = InputBufferAdapter<Buffer>;

int main() {
    //set some random data
    MyStruct data{8941, MyEnum::V2, {15.0f, -8.5f, 0.045f}};
    MyStruct res{};

    //create buffer to store data
    std::vector<uint8_t> buffer;

    auto writtenSize = startSerialization<OutputAdapter>(buffer, data);

    auto state = startDeserialization<InputAdapter>(InputAdapter{buffer.begin(), writtenSize}, res);

    assert(state.first == ReaderError::NO_ERROR && state.second);
    assert(data.fs == res.fs && data.i == res.i && data.e == res.e);
}
