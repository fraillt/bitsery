#include <bitsery/bitsery.h>

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

int main() {
    //set some random data
    MyStruct data{8941, MyEnum::V2, {15.0f, -8.5f, 0.045f}};
    MyStruct res{};

    //create serializer
    //1) create buffer to store data
    std::vector<uint8_t> buffer;
    //2) create buffer writer that is able to write bytes or bits to buffer
    BufferWriter bw{buffer};
    //3) create serializer
    Serializer ser{bw};

    //serialize object, can also be invoked like this: serialize(ser, data)
    ser.object(data);

    //flush to buffer, before creating buffer reader
    bw.flush();

    //create deserializer
    //1) create buffer reader
    BufferReader br{bw.getWrittenRange()};
    //2) create deserializer
    Deserializer des{br};

    //deserialize same object, can also be invoked like this: serialize(des, data)
    des.object(res);
    assert(data.fs == res.fs && data.i == res.i && data.e == res.e);
}
