# Bitsery

Header only C++ binary serialization library.
It is designed around the networking requirements for multiplayer real-time fast paced games as first person shooters.
All cross-platform requirements are enforced at compile time, so serialized data do not store any run-time type information and is as small as possible.

## Status

**bitsery** is in pre-release state and is looking for your feedback. 
It has basic features, serialize arithmetic types, enums, containers and text, and few advanced features like value ranges and default values (substitution), but is still missing functions for delta changes and geometry compression.
> Current version do not handle Big/Little Endianness.
> Error handling on deserialization is not tested.

## Example
```cpp
#include <iostream>
#include <vector>
#include <Bitsery.h>


enum class MyEnum { V1,V2,V3 };
struct MyStruct {
    int i;
    MyEnum e;
    std::vector<float> fs;
};

//define how object should be serialized/deserialized
SERIALIZE(MyStruct) {
    return s.
            value(o.i).
            value(o.e).
            container(o.fs, 100);
}

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
    Serializer<BufferWriter> ser{bw};

    //serialize object, can also be invoked like this: serialize(ser, data)
    ser.object(data);

    //flush to buffer, before creating buffer reader
    bw.flush();

    //create deserializer
    //1) create buffer reader
    BufferReader br{buffer};
    //2) create deserializer
    Deserializer<BufferReader> des{br};

    //deserialize same object, can also be invoked like this: serialize(des, data)
    des.object(res);

    //check is equal
    std::cout << "is equal: " << (data.fs == res.fs && data.i == res.i && data.e == res.e) << std::endl;
}
```

## Platforms

This library was tested on
* Windows: Visual Studio 2015, MinGW (gcc 5.2)
* Linux: GCC 5.4, GCC 6.2, Clang 3.9

