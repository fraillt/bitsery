# The Problem

Deserialization process is the same to serialization in a sense, that all serialization/deserialization operations is in the same order, except that instead of writing to buffer you read from it, so it is very desirable to have the same code express both functionality, but is it really possible? Let's find out!

To achieve this *Deserializer* has exactly the same interface as *Serializer*, EXCEPT that all methods in *Deserializer* accept data as *T&*, but *Serializer* accepts as *const T&*.

So one way to make this happen is to have *Serializer/Deserializer* as template parameter, and actual object accept as *T&* like this.

```cpp
template <typename S>
void serialize(S& s, Player& o) {
  s.value4(o.pos.x);
  s.value4(o.pos.y);
  s.value4(o.pos.z);
  s.text1(o.name);
}
```

You can use this function for serialization and deserialization, but you can`t pass *const T&*, which is huge limitation.

# Bitsery solution

In order to fix this *const T&* issue, all we need to do is use [SFINAE](http://en.cppreference.com/w/cpp/language/sfinae) technique to enable this function if T is *Object* or *const Object*, like this:
```cpp
template <typename S, typename T, typename std::enable_if<std::is_same<T, Player>::value || std::is_same<T, const Player>::value>::type* = nullptr>
void serialize (S& s, T& o) {
...
}
```

Let's modify our [hello world](hello_world.md) example and add deserialization to it.

```cpp
#include <vector>
#include <bitsery/bitsery.h>
#include <cstring>
#include <iostream>

struct Vector3f {
    float x;
    float y;
    float z;
    bool operator == (const Vector3f& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct Player {
    Vector3f pos;
    char name[50];
};

using namespace bitsery;

SERIALIZE(Player) {
    s.value4(o.pos.x);
    s.value4(o.pos.y);
    s.value4(o.pos.z);
    s.text1(o.name);
}

Player createData() {
    Player data;
    data.pos.x = 0.45f;
    data.pos.y = 50.9f;
    data.pos.z = -15687.87f;
    strcpy(data.name,"Yolo");
    return data;
}

int main() {
    const Player data = createData();
    Player res{};

    std::vector<uint8_t> buf;

    BufferWriter bw{buf};
    Serializer<BufferWriter> ser{bw};

    serialize(ser, data);

    bw.flush();

    BufferReader br{bw.getWrittenRange()};
    Deserializer<BufferReader> des{br};

    serialize(des, res);

    std::cout << "deserializer state: " << des.isValid() << std::endl
              << "buffer completed: " << br.isCompleted() << std::endl
              << "pos equals: " << (res.pos == data.pos) << std::endl
              << "name equals: " << (strcmp(res.name, data.name) == 0);
    return 0;
}
```

```bash
deserializer state: 1
buffer completed: 1
pos equals: 1
name equals: 1
```

We created *Deserializer* and modified *serialize* function to accept *Serializer* and *Deserializer*.

Deserialization is very similar as serialization, it also consists of three separate components:
* Buffer - container that we read data from, in our case *vector<uint8_t>*.
* BufferReader - reads bytes and bits from *Buffer*, it also makes sure that it is portable across Little and Big endian systems. 
* Deserializer - same interface as *Serializer* that use *BufferReader* to read bits and bytes, and convert to specific type. Deserializer also checks for errors at runtime, because data might come from untrusted source and can terminate program with buffer-overflow or segmentation fault if we are not careful.

Since deserialization involves error checking there are two additional functions to check if everything is correct after deserialization.
* [BufferReader.isCompleted()](../reference/buf_is_completed.md) - returns true, if whole buffer was read during deserialization.
* [Deserializer.isValid()](../reference/fnc_is_valid.md) - returns true, if there was no errors during deserialization.

One thing to note about BufferReader is that it doesn't have constructor that accepts buffer directly. Instead it only accepts begin/end iterators, because it needs to know precise data buffer length, to correctly use *isComplete* function.

```cpp
BufferReader br{bw.getWrittenRange()};
Deserializer<BufferReader> des{br};
```

To reduce code for *serialize* function using *SFINAE* technique, **bitsery** has macro *SERIALIZE*. Using this macro code looks much cleaner, and now this function can accept both *Player* and *const Player*.
```cpp
SERIALIZE(Player) {
    s.value4(o.pos.x);
    s.value4(o.pos.y);
    s.value4(o.pos.z);
    s.text1(o.name);
}
...
serialize(ser, data); //ser-> Serializer, data-> const Player
...
serialize(des, res); //des-> Deserializer, data-> Player
```

# Summary

You have learned how to write *serialize* function for your type, that works with serialization and deserialization. You also learned that deserialization is very similar to serialization, but has runtime error checking.

In [next chapter](composition.md) you'll learn how to compose complex serialization/deserialization flows efficiently.
