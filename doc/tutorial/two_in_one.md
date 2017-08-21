# The Problem

Deserialization process is the same to serialization in a sense, that all serialization/deserialization operations is in the same order, except that instead of writing to buffer you read from it, so it is very desirable to have the same code express both functionality, but is it really possible? Let's find out!


To achieve this *Deserializer* has exactly the same interface as *Serializer*, EXCEPT that all values in *Deserializer* has **T&**, but *Serializer* has **const T&**.

So one way to make this happen is to have Serializer/Deserializer as template parameter, and actual object accept as *T&* like this.

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