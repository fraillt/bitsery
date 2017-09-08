# The problem

You want to serialize *Player* structure efficiently into buffer.

```cpp
struct Vector3f {
  float x;
  float y;
  float z;
};
struct Player {
  Vector3f pos;
  char name[50];
};

```

# Poor man's implementation

Since you don't want to waste any space using any text serialization library like json or xml, the one of the most easiest and obvious solution is to simply write memory representation of the structure directly to buffer.


Possible implementation could look like this.
```cpp
#include <iostream>
#include <vector>
#include <cassert>
#include <cstdint>
#include <algorithm>

struct Vector3f {
  float x;
  float y;
  float z;
};
struct Player {
  Vector3f pos;
  char name[50];
};

void serialize(std::vector<uint8_t>& buf, const Player& data) {
  auto ptr = reinterpret_cast<const uint8_t*>(&data);
  std::copy_n(ptr, sizeof(data), std::back_inserter(buf));
}

int main() {
  Player data;
  std::vector<uint8_t> buf;
  serialize(buf, data);
  assert(buf.size() == sizeof(data));
  std::cout << "size: " << buf.size() << std::endl;
  return 0;
}
```

```bash
size: 64
```

Although it is simple and fast (it could be faster if we reserve buffer before writing) it has a lot of limitations.
* char[50] always writes to atleast 50 bytes in buffer, even if player name is *Yolo*.
* you can't replace char[50] with std::string.
* you can't use this solution if you need to support different endianness systems, and you should be extra careful if different systems has different size for fundamental types like int, long int, etc...
* you pay for structure field alignment hence size is equal to 64, not 62(3*4+50).

You can improve your name serialization in various ways, but then your serialization and deserialization code gets compllicated and error prone. We can do better than this.

# Bitsery solution

Let's solve the same problem with the library.
```cpp
#include <vector>
#include <bitsery/bitsery.h>
#include <cstring>
#include <iostream>

struct Vector3f {
  float x;
  float y;
  float z;
};

struct Player {
  Vector3f pos;
  char name[50];
};

using namespace bitsery;

void serialize(Serializer<BufferWriter>& s, const Player& data) {
  s.value4b(data.pos.x);
  s.value4b(data.pos.y);
  s.value4b(data.pos.z);
  s.text1b(data.name);
}

int main() {
  Player data;
  strcpy(data.name,"Yolo");

  std::vector<uint8_t> buf;
  BufferWriter bw{buf};
  Serializer<BufferWriter> ser{bw};

  serialize(ser, data);

  bw.flush();
  auto range = bw.getWrittenRange();
  std::cout << "size: " << std::distance(range.begin(), range.end()) << std::endl;
  return 0;
}
```

```bash
size: 17
```

First of all, buffer size dropped from 64 down to 17bytes: 12 bytes (3*4) for floats and only 5bytes for the name "Yolo".
In the process you also lost all limitations that had naive solution. You even gain some features for free:
* endianess support.
* more readable serialization code.


Let's look at the code, how we did this.


There are three distinct parts that participate in serialization process.
* Buffer - container that we store our serialized data, in our case vector<uint8_t>.
* BufferWriter - resposible for writing bytes and bits to *Buffer*, it also makes sure that it is portable across Little and Big endian systems.
* Serializer - extendable interface that converts any type to bytes or bits, and use *BufferWriter* to write them. Serializer object does not store any state, it only forwards all calls to BufferWritter, further more it ensures that code is portable at compile time. This means, that if your serialization code compiles on other platform, it will be 100% correct.
```cpp
  std::vector<uint8_t> buf;
  BufferWriter bw{buf};
  Serializer<BufferWriter> ser{bw};
```

Serialization function is very readable, and explicitly express intent what and how to serialize:
* *value4b* serialize [fundamental type](../design/fundamental_types.md) (ints, floats, chars, enums) of 4 bytes.
* *text1b* effectively serialize text, and underlying text type is 1byte per letter.

```cpp
  s.value4b(data.pos.x);
  s.value4b(data.pos.y);
  s.value4b(data.pos.z);
  s.text1b(data.name);
```

> learn more about why you need to write [value4b instead of value](../design/function_n.md).


Finally, before getting serialized data you must *flush* BufferWriter, it writes any remaining bits to buffer and additional data for types that require forward/backward compatibility. In our case it is not required, because we only worked with whole bytes, but it is good practice to always use it after finishing serialization.

To actually get written data you must call *getWrittenRange*, it return begin/end iterators to our buffer (*std::vector<uint8_t> buf*), for performance reasons BufferWritter always resizes underlying buffer to *capacity* so it could use containers iterator to update data, instead of back_insert_iterator to insert data.

```cpp
  bw.flush();
  auto range = bw.getWrittenRange();
```

# Summary

You have learned how to serialize simple structure to buffer that occupies no unnecessary bytes, and is portable across any system. You also learned that serialization process consist of three independant parts: buffer, buffer writer, and serializer. You used serializer to explicitly declare what and how to serialize and learned that you should always call BufferWriter.flush() before using buffer data.

In [next chapter](two_in_one.md) you'll learn how to use this expressive, declarative serialization function and use it to deserialize buffer to object, and in the process gain runtime error checking for free!



