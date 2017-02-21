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
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Serializer.h"
#include "Deserializer.h"

enum class MyEnum {
    V1,V2,V3
};

struct MyStruct {
    int i;
    MyEnum e;
    std::vector<float> fs;
};

//define how object should be serialized
SERIALIZE(MyStruct) {
    return s.
            value(o.i).
            value(o.e).
            container(o.fs, 100);
}

void print(const char* msg, const MyStruct& v) {
    std::cout << msg << std::endl;
    std::cout << "i:" << v.i << std::endl;
    std::cout << "e:" << (int)v.e << std::endl;
    std::cout << "fs:";
    for (auto p:v.fs)
        std::cout << '\t' << p;
    std::cout << std::endl << std::endl;
}

int main() {
    //set some random data
    MyStruct data{};
    data.e = MyEnum::V2;
    data.i = 48465;
    data.fs.resize(4);
    float tmp = 4253;
    for (auto& v: data.fs) {
        tmp /=2;
        v = tmp;
    }

    //create serializer
    //1) create buffer to store data
    std::vector<uint8_t> buffer;
    //2) create buffer writer that is able to write bytes or bits to buffer
    BufferWriter bw{buffer};
    //3) create serializer
    Serializer<BufferWriter> ser{bw};

    //call serialize function
    serialize(ser, data);

    //flush to buffer
    bw.flush();

    MyStruct result{};

    //create deserializer
    //1) create buffer reader
    BufferReader br{buffer};
    //2) create deserializer
    Deserializer<BufferReader> des{br};

    //call same function with different arguments
    serialize(des, result);

    //print results
    print("initial data", data);
    print("result", result);
}
```

## Platforms

This library was tested on
* Windows: Visual Studio 2015
* Linux: GCC 5.4, GCC 6.2, Clang 3.9

