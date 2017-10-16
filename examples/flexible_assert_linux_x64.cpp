#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/flexible.h>
#include <bitsery/flexible/vector.h>


struct MyStruct {
    int i;
    unsigned short s;
    std::vector<long> vl;
    long long ll;

    template <typename S>
    void serialize(S& s) {
        //now we can use flexible syntax with
        //member function has same name as parameter
        s.archive(this->s, i, vl, ll);
    }

};


using namespace bitsery;

//some helper types
using Buffer = std::vector<uint8_t>;
using OutputAdapter = OutputBufferAdapter<Buffer>;
using InputAdapter = InputBufferAdapter<Buffer>;

int main() {
    //this will only work on linux or mac x64
    bitsery::assertFundamentalTypeSizes<2,4,8,8>();
    //set some random data
    MyStruct data{8941, 3, {15l, -8l, 045l}, 8459845ll};
    MyStruct res{};

    //serialization, deserialization flow is unchanged as in basic usage
    Buffer buffer;
    auto writtenSize = quickSerialization<OutputAdapter>(buffer, data);

    auto state = quickDeserialization<InputAdapter>({buffer.begin(), writtenSize}, res);

    assert(state.first == ReaderError::NoError && state.second);
    assert(data.vl == res.vl && data.s == res.s && data.i == res.i && data.ll == res.ll);
}
