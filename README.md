[![Build Status](https://travis-ci.org/fraillt/bitsery.svg?branch=master)](https://travis-ci.org/fraillt/bitsery)
# Bitsery

[![Join the chat at https://gitter.im/bitsery/Lobby](https://badges.gitter.im/bitsery/Lobby.svg)](https://gitter.im/bitsery/Lobby?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&utm_content=badge)

Header only C++ binary serialization library.
It is designed around the networking requirements for multiplayer real-time fast paced games as first person shooters.
All cross-platform requirements are enforced at compile time, so serialized data do not store any run-time type information and is as small as possible.

## Features

**bitsery** is looking for your feedback.
* Has configurable endianess support.
* Can serialize all common types: arithmetic types, enums, containers and text.
* Has advanced features like value ranges and default values.
* Is extensible, for types that requires different serialization and deserialization logic (e.g. pointers)
* Has error checking on deserialization, and asserts on serialization runtime errors.

## Example
```cpp
#include <bitsery/bitsery.h>

enum class MyEnum:uint16_t { V1,V2,V3 };
struct MyStruct {
    uint32_t i;
    MyEnum e;
    std::vector<float> fs;
};

//define how object should be serialized/deserialized
SERIALIZE(MyStruct) {
    return s.
            value4(o.i).
            value2(o.e).
            container4(o.fs, 10);
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
    assert(data.fs == res.fs && data.i == res.i && data.e == res.e);
}
```

## Todo list

> Support wider range for underlying buffer value type for BufferWriter and BufferReader (currently must be unsigned 1byte, e.g. uint8_t).

> Delta serialization and deserialization is in progress.

## Platforms

This library was tested on
* Windows: Visual Studio 2015, MinGW (gcc 5.2)
* Linux: GCC 5.4, GCC 6.2, Clang 3.9
* OS X Mavericks: AppleClang 8

