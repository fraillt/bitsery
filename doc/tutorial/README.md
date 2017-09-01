The grand plan for this tutorial is to learn how to serialize/deserialize any object efficiently in time and space, so you could focus on other, more interesting things.


This tutorial will cover these main topics:
* [Hello World](hello_world.md) serialize a simple struct.
* [2 in 1](two_in_one.md) write one control flow for both: serialization and deserialization.
* [Composer](composition.md) efficiently compose complex serialization flows.
* [Squeeze Me!](compression.md) compress your data when you know what it stores.
* [Anything is Possible](extensions.md) extend library for custom container, compress geometry and more.
* [Little or Big](endianness.md) change endianness if you want best performance on PowerPC.

In order to successfully use the library you need c++14 compatible compiler. In theory you could also use c++11 compatible compiler, but c++14 generic lambdas really change the way you can work with this library, so all tutorial sections will asume that you use c++14 compatible compiler.

So without further ado lets start with [hello world](hello_world.md).
