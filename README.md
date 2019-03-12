# Bitsery

[![Build Status](https://travis-ci.org/fraillt/bitsery.svg?branch=master)](https://travis-ci.org/fraillt/bitsery)
[![Join the chat at https://gitter.im/bitsery/Lobby](https://badges.gitter.im/bitsery/Lobby.svg)](https://gitter.im/bitsery/Lobby)

Header only C++ binary serialization library.
It is designed around the networking requirements for real-time data delivery, especially for games.

All cross-platform requirements are enforced at compile time, so serialized data do not store any meta-data information and is as small as possible.

> **bitsery** is looking for your feedback on [gitter](https://gitter.im/bitsery/Lobby)

## Features

* Cross-platform compatible.
* Optimized for speed and space.
* No code generation required: no IDL or metadata, just use your types directly.
* Runtime error checking on deserialization.
* Can read/write from any source: stream (file, network stream. etc... ), or buffer (vector, c-array, etc...).
* Don't pay for what you don't use! - customize your serialization via **extensions**. Some notable *extensions* allow:
  * forward/backward compatibility for your types.
  * smart and raw pointers with customizable runtime polymorphism support.
  * fine-grained bit-level serialization control.
* Easily extendable for any type.
* Allows flexible or/and verbose syntax for better serialization control.
* Configurable endianness support.
* No macros.

## Why to use bitsery

Look at the numbers and features list, and decide yourself.

|                              | binary size | data size | serialize   | deserialize |
|------------------------------|-------------|-----------|-------------|-------------|
| **test_bitsery**             | 64704 B     | **7565 B**| **1229 ms** | **1086 ms** |
| **test_bitsery_compression** | 64880 B     | **4784 B**| **1641 ms** | **2462 ms** |
| test_yas                     | 63864 B     | 11311 B   | 1616 ms     | 1712 ms     |
| test_yas_compression         | 72688 B     | 8523 B    | 2387 ms     | 2890 ms     |
| test_cereal                  | 74848 B     | 11261 B   | 6708 ms     | 6799 ms     |
| test_flatbuffers             | 67032 B     | 16100 B   | 8793 ms     | 3028 ms     |

*benchmarked on Ubuntu with GCC 7.1.0, more details can be found [here](https://github.com/fraillt/cpp_serializers_benchmark.git)*

If still not convinced read more in library [motivation](doc/design/README.md) section.

## Usage example
```cpp
#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>

enum class MyEnum:uint16_t { V1,V2,V3 };
struct MyStruct {
    uint32_t i;
    MyEnum e;
    std::vector<float> fs;
};

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
    MyStruct data{8941, MyEnum::V2, {15.0f, -8.5f, 0.045f}};
    MyStruct res{};

    Buffer buffer;

    auto writtenSize = quickSerialization<OutputAdapter>(buffer, data);

    auto state = quickDeserialization<InputAdapter>({buffer.begin(), writtenSize}, res);

    assert(state.first == ReaderError::NoError && state.second);
    assert(data.fs == res.fs && data.i == res.i && data.e == res.e);
}
```
For more details go directly to [quick start](doc/tutorial/hello_world.md) tutorial.

## How to use it
This documentation comprises these parts:
* [Tutorial](doc/tutorial/README.md) - getting started.
* [Reference section](doc/README.md) - all the details.

*documentation is in progress, most parts are empty, but [contributions](CONTRIBUTING.md) are welcome.*

## Requirements

Works with C++11 compiler, no additional dependencies, include `<bitsery/bitsery.h>` and you're done.

> some **bitsery** extensions might require higher C++ standard (e.g. `StdVariant`)

## Platforms

This library was tested on
* Windows: Visual Studio 2015, MinGW (GCC 5.2)
* Linux: GCC 5.4, GCC 6.2, Clang 3.9
* OS X Mavericks: AppleClang 8

## License

**bitsery** is licensed under the [MIT license](LICENSE).
