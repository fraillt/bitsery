#include <bitsery/bitsery.h>
//in order to work with streams include stream adapter
#include <bitsery/adapter/stream.h>
#include <fstream>
#include <iostream>

enum class MyEnum:uint16_t { V1,V2,V3 };
struct MyStruct {
    uint32_t i;
    MyEnum e;
    double f;
};

//define how object should be serialized/deserialized
template <typename S>
void serialize(S& s, MyStruct& o) {
    s.value4b(o.i);
    s.value2b(o.e);
    s.value8b(o.f);
}

using namespace bitsery;

//some helper types
using Stream = std::fstream;
using IOAdapter = IOStreamAdapter;

int main() {
    //set some random data
    MyStruct data{8941, MyEnum::V2, 0.045};
    MyStruct res{};

    //open file stream for writing and reading
    auto fileName = "test_file.bin";
    Stream s{fileName, s.binary | s.trunc | s.out};
    if (!s.is_open()) {
        std::cout << "cannot open " << fileName << " for writing\n";
        return 0;
    }
    //use same quick serialization function
    //streams do not return written size
    quickSerialization<IOAdapter>(s, data);

    s.close();
    //reopen for reading

    s.open(fileName, s.binary | s.in);
    if (!s.is_open()) {
        std::cout << "cannot open " << fileName << " for reading\n";
        return 0;
    }

    //same as serialization, but returns deserialization state as a pair
    //first = error code, second = is buffer was successfully read from begin to the end.
    auto state = quickDeserialization<IOAdapter>(s, res);

    assert(state.first == ReaderError::NoError && state.second);
    assert(data.f == res.f && data.i == res.i && data.e == res.e);
}
