# Quick Start

This is a quick guide to get **bitsery** up and running in a matter of minutes.
The only prerequisite for running bitsery is a modern C++11 compliant compiler, such as GCC 4.9.4, clang 3.4, MSVC 2015, or newer.
Older versions might work, but it is not tested.

## Get bitsery

bitsery can be directly included in your project or installed anywhere you can access header files.
Grab the latest version, and include directory `bitsery_base_dir/include/` to your project.
There's nothing to build or make - **bitsery** is header only.

## Add serialization methods for your types

**bitsery** needs to know which data members to serialize in your classes.
Let it know by implementing a serialize method for your type:

```cpp
struct MyStruct {
    uint32_t i;
    char str[6];
    std::vector<float> fs;
};

template <typename S>
void serialize(S& s, MyStruct& o) {
    s.value4b(o.i);
    s.text1b(o.str);
    s.container4b(o.fs, 100);
};
```

**bitsery** also can serialize private class members, just move *serialize* function inside structure, and make it *friend* (*fiend void serialize(.....)*).

**bitsery** has verbose syntax, because it is cross-platform compatible by default and has full control over how to serialize data (read more about it in [motivation](../design/README.md))

This example contains core functionality that you'll use all the time, so lets get through it:
* **s.value4b(o.i);** serialize fundamental types (ints, floats, enums) value**4b** means, that data type is 4 bytes. If you use same code on different machines, if it compiles it means it is compatible.
* **s.text1b(o.str);** serialize text (null-terminated) of char type, if you use *wchar* then you would write *text2b*.
* **s.container4b(o.fs, 100);** serializes any container of fundamental types of size 4bytes, **100** is max size of container.
**Bitsery** is designed to be save with untrusted (malicious) data from network, so for dynamic containers you always need to provide max possible size available, to avoid buffer-overflow attacks.
**text** didn't had this max size specified, because it was serializing fixed size container.

External serialization functions should be placed either in the same namespace as the types they serialize or in the **bitsery** namespace so that the compiler can find them properly.

## Serialization and deserialization

### Create serializer
Create a serializer and send the data you want to serialize to it.

```cpp
std::vector<uint8_t> buffer;
BufferWriter bw{buffer};
Serializer ser{bw};
```

Serialization process consists of three independant parts.
* **std::vector<uint8_t> buffer;** core object, that will store the data for serialization and deserialization.
* **BufferWriter bw{buffer};** writer knows how to write bytes to buffer, and how to resize buffer, or how to use fixed-size buffer. It also applies endianess transformations if nesessary.
* **Serializer ser{bw};** serializer is a high level wrapper that knows how to convert object to stream of bytes, and write then to buffer.

Serializer doesn't store any state, it only has reference to buffer, so it is safe to create many of those if nesessary.

BufferWriter also doesn't own buffer, but it stores state about writing position and container size.

One important note that when using bit-level operations, dont forget to flush buffer writer **bw.flush()** otherwise, some data might not be written to buffer.


### Serialize object

```cpp
MyStruct data{8941, "hello", {15.0f, -8.5f, 0.045f}};
ser.object(data); // serializes data
```

**ser.object(data)** is a final core function along with **value, text, container**.

This function is actually equivalent to calling *serialize(ser, data)* directly, but it displays friendly static assert message if it cannot find *serialize* function for your type.

### Deserialize object

```cpp
BufferReader br{bw.getWrittenRange()};
Deserializer des{br};

MyStruct res{};
des.object(res); //deserializes data
```

Deserialization process is equivalent to serialization, except that *BufferReader* reader has getError() method that returns deserialization state.

## Full example code

```cpp
#include <bitsery/bitsery.h>

using namespace bitsery;

struct MyStruct {
    uint32_t i;
    char str[6];
    std::vector<float> fs;
};

template <typename S>
void serialize(S& s, MyStruct& o) {
    s.value4b(o.i);
    s.text1b(o.str);
    s.container4b(o.fs, 100);
};

int main() {
    std::vector<uint8_t> buffer;
    BufferWriter bw{buffer};
    Serializer ser{bw};

    MyStruct data{8941, "hello", {15.0f, -8.5f, 0.045f}};
    ser.object(data); // serializes data

    BufferReader br{bw.getWrittenRange()};
    Deserializer des{br};

    MyStruct res{};
    des.object(res); //deserializes data
}
```

**currently documentation and tutorial is progress, but for more usage examples see examples folder**