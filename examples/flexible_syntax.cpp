#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
//include flexible header, to use flexible syntax
#include <bitsery/flexible.h>
//we also need additional traits to work with container types,
//instead of including <bitsery/traits/vector.h> for vector traits, now we also need traits to work with flexible types.
//so include everything from <bitsery/flexible/...> instead of <bitsery/traits/...>
//otherwise we'll get static assert error, saying to define serialize function.
#include <bitsery/flexible/vector.h>

enum class MyEnum:uint16_t { V1,V2,V3 };
struct MyStruct {
    uint32_t i;
    MyEnum e;
    std::vector<float> fs;

    //define serialize function as usual
    template <typename S>
    void serialize(S& s) {
        //now we can use flexible syntax with
        s.archive(i, e, fs);
    }

};


using namespace bitsery;

//some helper types
using Buffer = std::vector<uint8_t>;
using OutputAdapter = OutputBufferAdapter<Buffer>;
using InputAdapter = InputBufferAdapter<Buffer>;

int main() {
    //set some random data
    MyStruct data{8941, MyEnum::V2, {15.0f, -8.5f, 0.045f}};
    MyStruct res{};

    //serialization, deserialization flow is unchanged as in basic usage
    Buffer buffer;
    auto writtenSize = quickSerialization<OutputAdapter>(buffer, data);

    auto state = quickDeserialization<InputAdapter>({buffer.begin(), writtenSize}, res);

    assert(state.first == ReaderError::NoError && state.second);
    assert(data.fs == res.fs && data.i == res.i && data.e == res.e);
}
