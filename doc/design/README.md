## Motivation

Inspiration to create **bitsery** came mainly because there aren't any good alternatives for C++.

I wanted serializer that is easy to use like [cereal](http://uscilab.github.io/cereal/), is cross-platform compatible, and has support for forward/backward compatibility like [flatbuffers](https://google.github.io/flatbuffers/), is save to use with untrusted (malicious) data, and most importantly is fast and has small binary footprint.

Furthermore I wanted full serialization control and ability to work on bit level, so I can further reduce data size. For example, serializing container of [quaternions](https://en.wikipedia.org/wiki/Quaternion) I can reduce size by large amount. *Size of orientation quaternion can be reduced from 128bits (4floats) down to 29bits using "smallest three" technique and still retaining decent precision*.

Most well-known serialization libraries sacrifice memory and speed efficiency by supporting multiple data formats (binary, json, xml) and multiple languages (C++, C#, Javascript, etc..), these features also adds additional library complexity.

## A word about JSON

Often times people use C++ because they want speed and memory efficiency, and JSON is not on the list of efficient serialization format.
Although JSON is very readable and very convenient when used together with dynamically typed languages such as JavaScript.
When serializing data from statically typed languages, however, JSON not only has the obvious drawback of runtime inefficiency, but also forces you to write more code to access data (counterintuitively) due to its dynamic-typing serialization system.

Adding optional support for JSON doesn't come for free either.
To support JSON, additional metadata is required.
In C++ it can be achieved by two ways:
* with macros, that generate additional types like [cereal](http://uscilab.github.io/cereal/) does.
* with code generation from IDL (interface definition language) like [flatbuffers](https://google.github.io/flatbuffers/) does.

Both solutions adds additional complexity to the library. In the future C++ will get reflection system, currently static reflection was proposed to standard.

So, to avoid unnecessary library complexity it is best to forget about JSON, and stick with what machines and C++ is good at, - binary format.

# Bitsery

Bitsery is designed to be lightweight and simple to use, yet powerful and extendable library.
To ensure it works as intended it is unit tested, and has 100% code coverage.

Now let's review features in more detail.

* **Cross-platform compatible.** if same code compiles on Android, PS3 console, and your PC either x64 or x86 architecture, you are 100% sure it works.
To achieve this, bitsery specifically defines size of underlying data, hence syntax is *value\<2\>* (alias function *value2b*)  instead or *value*, or *container2b* for element type of 16bits, eg int16_t.
Bitsery also applies endianness transformation if necessary.
* **Flexible syntax.** If you don't like like writing code with explicitly specifying underlying type size, like *container2b* or *value8b*, you can use flexible syntax.
Just include <bitsery/flexible.h> and can write like in [cereal](http://uscilab.github.io/cereal/).
But do it on your own risk, and static assert using *assertFundamentalTypeSizes* function if you're planing to use it across multiple platforms.
* **Optimized for speed and space.** library itself doesn't do any allocations (except if you use backward/forward compatibility) so data writing/reading is fast as memcpy to/from your buffer.
It also doesn't serialize any type information, all information needed is written in your code!
* **No code generation required: no IDL or metadata** since it doesn't support any other formats except binary, it doesn't need any metadata.
* **Runtime error checking on deserialization** library designed to be save with untrusted network data, that's why all overloads that work on containers has *maxSize* value, unless container is static size like *std::array*, this way bitsery ensures that no malicious data crash you.
* **Supports forward/backward compatibility for your types** library has optional forward/backward compatibility for types implemented in *AdapterReader/Writer* by allowing to have inner data sessions inside buffer.
This is the only functionality that requires dynamic memory allocation.
*Growable* extension use these sessions to add compatibility support for your types, in most basic form.
You can implement your own extensions if you want to be able to add default values.
* **2-in-1 declarative control flow, same code for serialization and deserialization.** only one function to define, for serialization and deserialization in same manner as *cereal* does.
It might be handy to have separate *load* and *save* functions, but Bitsery explicitly doesn't support it, to avoid any serialization deserialization divergence, because it is very hard to catch an errors if you make a bug in one of these functions.
The only way around this through extensions, write your custom flow once, and reuse where you need them.
* **Allows fine-grained serialization control** this is a feature that no other libraries provides.
Bitsery allows to use bit-level operations and has two extensions that use them:
  * *ValueRange*,- if you have a *int16_t* data type, but you know that your object only stores values in \[0..1000\] range, it will write 10bits, instead of 16bits. ValueRanges also works with floats.
  * *Entropy*,- full term is *entropy encoding*, which means that when you have most common value, or multiple values, it will write just few bits instead of full object.

    Eg.: imagine that you have a struct Person{ int32_t Id; string Profession; }.
    You might know that mostly there are young persons, so the most common value will be equal to: "Student", "Child", "NoProfession", in this case you'll pay 2bits for each record, but write no data if string matches.

  Using these bit-level operations and extensions you can compose your own extensions for vectors, matrices or any other types.
  Further more, all other operations will not align data automatically for you, so data will be compressed as much as possible.

  * One more advanced and dangerous feature, is ability to have serialization context, so you can control your serialization flow at runtime, but make sure that these contexts are in sync between serializer and deserializer.
    One possible use case for serialization context is to pass min/max ranges for *ValueRange* when your information changes at runtime.
* **Easily extendable** library is designed to be easily extendable for any type and flow.
You want to support your custom container, its fine there is *ContainerTraits* for this, only few methods required to implement.
To use same container for buffer writing/reading add specialization to *BufferAdapterTraits*.
You want to customize serialization flow - use extensions, only two methods to define, and *ExtensionTraits* to further customize usage.
* **Configurable endianness support.** default is *Little Endian*, but if your primary target is PowerPC architecture, eg. PlayStation3, just change your configuration to be *Big Endian*.
* **No macros.** Not so much to say, if you are like me, then it's a feature :)
