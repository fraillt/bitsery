# [4.6.0](https://github.com/fraillt/bitsery/compare/v4.5.1...v4.6.0) (2019-03-12)

### Features
* new extensions **StdTuple** and **StdVariant** for `std::tuple` and `std::variant`. These are the first extensions that requires C++17, or higher, standard enabled.
Although `std::tuple` is C++11 type, but from usage perspective it has exactly the same requirements as `std::variant` and relies heavily on having class template argument deduction guides to make it convenient to use.
You can easily use `std::tuple` without any extension at all, so the main motivation was to create convenient interface for **StdVariant** and use the same interface for **StdTuple** as well.
  * instead of providing custom lambda to overload each type in tuple or variant, there was added several helper callable objects.
  **OverloadValue** wrapper around `s.value<N>(o)`, **OverloadExtValue** wrapper around `s.ext<N>(o, Ext{})` and **OverloadExtObject** wrapper around `s.ext(o, Ext{})`.
* new extensions **StdDuration** and **StdTimePoint** for `std::chrono::duration` and `std::chrono::time_point`.

### Improvements
tests now uses `gtest_discover_tests` function, to automatically discover tests, which requires CMake 3.10.

# [4.5.1](https://github.com/fraillt/bitsery/compare/v4.5.0...v4.5.1) (2019-01-16)

### Improvements
* template specializations, where possible, was changed to avoid using variadics, some Visual Studio compilers has [issues](https://developercommunity.visualstudio.com/content/problem/3437/error-with-c11-variadics.html) with variadic templates.
* reduced compile warnings for VisualStudio:
  * added explicit casts
  * renamed `struct` to `class` where class is used as friend. e.g. `friend class bitsery::Access`, because it is more conventional usage.

# [4.5.0](https://github.com/fraillt/bitsery/compare/v4.4.0...v4.5.0) (2019-01-10)

### Features
* ability to create non default constructible objects, by defining private default constructor and making `friend class bitsery::Access;` to access it.
It is not necessary to enforce class invariant immediately, because internal object representation will be overriden anyway.

### Improvements
* `StdSmartPtr` supports `std::unique_ptr` with custom deleter.
* `*InputBufferAdapter`(all) can also accept const buffer;

### Bug fixes
* fixed deserialization in `bitsery/ext/std_map{set}` when target container is not empty.
* added missing template parameters for specializations on `std` containers in multiple files in `bitsery/ext/*`.

# [4.4.0](https://github.com/fraillt/bitsery/compare/v4.3.0...v4.4.0) (2019-01-08)

### Features
* new extensions **CompactValue** and **CompactValueAsObject**, stores integral values in less space if possible. This is useful when you're working with mostly small values, that in rare cases can be large.
E.g. `int64_t money = 8000;` will only use 2 bytes, instead of 8. **CompactValueAsObject** allows to use `ext()` overload, without specifying size of underlying type and sets BUFFER_OVERFLOW error if value doesn't fit in underlying type during deserialization.

### Improvements
* improved **PolymorphicContext**, allows to extend already registered hierarchy in one translation unit, using different type other than `PolymorphicBaseClass` to avoid symbol collision between translation units or libraries.
`registerBasesList` was modified, so that it could accept user defined type (instead of `PolymorphicBaseClass`) that is used to declare hierarchy, by default it is `PolymorphicBaseClass`.
This introduced breaking change, for those who used this syntax (`registerBasesList<MySerializer, Shape>({})`) during registration.
It is encouraged to define helper type, that could be used for registering hierarchy for serialization and deserialization [example](examples/smart_pointers_with_polymorphism.cpp).
`This is only relevant then you want to use **PolymorphicContext** between different translation units or libraries`.
```cpp
//libA
namespace bitsery {
    namespace ext {
        template<>
        struct PolymorphicBaseClass<Shape> : PolymorphicDerivedClasses<Circle, Rectangle> {};
    }
}
using MyPolymorphicClassesForRegistering = bitsery::ext::PolymorphicClassesList<Shape>;
...
    ctx.registerBasesList<MySerializer>(MyPolymorphicClassesForRegistering{}).

//otherLib
struct MySquare: Shape {...}
//now it must define different type (exactly the same as PolymorphicBaseClass) to declare hierarchy
template<typename TBase>
struct MyHierarchy {
    using Childs = PolymorphicClassesList<>;
};

template <>
struct MyHierarchy<Shape>: bitsery::ext::bitsery::ext::PolymorphicClassesList<MySquare> {};
...
//notice that we pass MyHierarchy as second argument
    ctx.registerBasesList<MySerializer, MyHierarchy>(MyPolymorphicClassesForRegistering{}).
```
* **PolymorphicContext** also get optional method `registerSingleBaseBranch`, that allows manually register hierarchies, this might be more convenient when using you need to register in different translation units (or libraries), but it is error-prone.

# [4.3.0](https://github.com/fraillt/bitsery/compare/v4.2.1...v4.3.0) (2018-08-23)

### Features

* added runtime polymorphism support for pointer like types (raw and smart pointers).
In order to enable polymorphism new **PolymorphicContext** was created. It provides capability to register classes with serializer/deserializer.
  * runtime polymorphism can be customized, by replacing **StandardRTTI** from <bitsery/ext/utils/rtti_utils.h> header.
* added smart pointers support for std::unique_ptr, std::shared_ptr and std::weak_ptr via **StdSmartPtr** extension.
* new **UnsafeInputBufferAdapter** doesn't check for buffer size on deserialization, on some compilers can improve deserialization performance up to ~40%.

### Improvements
* creatly improved interface for extending/implementing support for pointer like types. Now all pointer like types extends from **PointerObjectExtensionBase** and implements/configures required details.
* reimplemented **PointerOwner**, **PointerObserver**, **ReferencedByPointer**.
* reimplemented **PointerLinkingContext** to properly support shared objects and runtime polymorphism, pointer ownership for shared objects now has two states: SharedOwner e.g. std::shared_ptr and SharedObserver std::weak_ptr.

### Other notes
There is one *minor?* issue/limitation for pointer like types that uses virtual inheritance. When several pointers points to same object through different static type. it will not work correctly e.g.:
```cpp
struct Derived: virtual Base {...};
struct MyData {
    std::shared_ptr<Derived> sptr;
    std::weak_ptr<Base> wptddr;
}
```
In this example wptr and sptr have different static type, and *Derived* is virtually inherited from *Base*, so I get different pointer address for different types.

# [4.2.1](https://github.com/fraillt/bitsery/compare/v4.2.0...v4.2.1) (2018-03-09)

### Improvements
* changed CMake structure, to follow **Modern CMake** principles.
  * bitsery now has *install* target and **find_package(Bitsery)** exports **Bitsery::bitsery** target.
  * *GTest* no longer downloads as external application, but tries to find via *find_package*.
  * removed *ext* folder, and instead added *scripts* folder that contains few helper scripts for development, currently tested on Ubuntu.
* fixed/added few tests cases.

### Other notes
* some work was done on polymorphism support, but current solution, although working, but has many design and performance issues, and interfaces for extensibility might change drastically.

# [4.2.0](https://github.com/fraillt/bitsery/compare/v4.1.0...v4.2.0) (2017-11-12)

### Features

* serializer/deserializer can now have **internal context(s)** via configuration.
It is convenient way to pass context, when it doesn't convey useful information outside of serializer/deserializer and is default constructable.
* added **contextOrNull\<T\>()** overload to *BasicSerializer/BasicDeserializer*.
Difference between *contextOrNull\<T\>()* and *context\<T\>()* is, that using *context\<T\>()* code doesn't compile if T doesn't exists at all, while using *contextOrNull\<T\>()* code compiles, but returns *nullptr* at runtime.
* added inheritance support via extensions.
In order to correctly manage virtual inheritance two extensions was created in **<bitsery/ext/inheritance.h>** header:
  * **BaseClass\<TBase\>** - use when inheriting from objects without virtual inheritance.
  * **VirtualBaseClass\<TBase\>** - ensures that only one copy of each virtual base class is serialized.

  To keep track of virtual base classes **InheritanceContext** is required, but it is optional if no virtual bases exists in serialization flow.
  I.e. if context is not defined, code will not compile only if virtual inheritance is used.
  See [inheritance](examples/inheritance.cpp) for usage example.

### Improvements
* added optional ctor parameter for *PointerOwner* and *PointerObserver* - **PointerType**, which specifies if pointer can be null or not.
Default is **Nullable**.

# [4.1.0](https://github.com/fraillt/bitsery/compare/v4.0.1...v4.1.0) (2017-10-27)

### Features
* added raw pointers support via extensions.
In order to correctly manage pointer ownership, three extensions was created in **<bitsery/ext/pointer.h>** header:
  * **PointerOwner** - manages life time of the pointer, creates or destroys if required.
  * **PointerObserver** - doesn't own pointer so it doesn't create or destroy anything.
  * **ReferencedByPointer** - when non-owning pointer (*PointerObserver*) points to reference type, this extension marks this object as a valid target for PointerObserver.

  To validate and update pointers **PointerLinkingContext** have to be passed to serialization/deserialization.
  It ensures that all pointers are valid, that same pointer doesn't have multiple owners, and non-owning pointers doesn't point outside of scope (i.e. non owning pointers points to data that is serialized/deserialized), see [raw_pointers example](examples/raw_pointers.cpp) for usage example.

  *Currently polimorphism and std::shared_ptr,  std::unique_ptr is not supported.*

* added **context\<T\>()** overload to *BasicSerializer/BasicDeserializer* and now they became typesafe.
For better extensions support, added posibility to have multiple types in context with *std::tuple*.
E.g. when using multiple extensions, that requires specific contexts, together with your custom context, you can define your context as *std::tuple\<PointerLinkingContext, MyContext\>* and in serialization function you can correctly get your data via *context\<MyContext\>()*.


### Improvements

* new **OutputBufferedStreamAdapter** use internal buffer instead of directly writing to stream, can get more than 2x performance increase.
  * can use any contiguous container as internal buffer.
  * when using fixed-size, stack allocated container (*std::array*), buffer size via constructor is ignored.
  * default internal buffer is *std::array<char,256>*.
* added *static_assert* when trying to use *BufferAdapter* with non contiguous container.


# [4.0.1](https://github.com/fraillt/bitsery/compare/v4.0.0...v4.0.1) (2017-10-18)

### Improvements

* improved usage with Visual Studio:
  * improved CMake.
  * Visual Studio doesn't properly support expression SFINAE using std::void_t, so it was rewritten.
* refactorings to remove compiler warnings when using *-Wextra -Wno-missing-braces -Wpedantic -Weffc++*
* added assertion when session is empty (sessions is created via *growable* extension).
* stream adapter manually *setstate* to *std::ios_base::eofbit* when unable to read required bytes.

# [4.0.0](https://github.com/fraillt/bitsery/compare/v3.0.0...v4.0.0) (2017-10-13)

I feel that current library public API is complete, and should be stable for long time.
Most changes was made to improve performance or/and make library usage easier.

### Features

* new **flexible** syntax similar to *cereal* library.
This syntax no longer requires to specify explicit fundamental type sizes and container maxsize (container max size can be enforced by special function *maxSize*).
Be careful when using deserializing untrusted data and make sure to enforce fundamental type sizes when using on multiple platforms.
(use helper function *assertFundamentalTypeSizes* to enforce type sizes for multiple platforms)
* added streaming support, by introducing new **adapter** concept. Two adapter implementations is available: stream adapter, or buffer adapter.
* added missing std containers support: forward_list, deque, stack, queue, priority_queue, set, multiset, unordered_set, unordered_multiset.

### Breaking changes

* a lot of classes and files were renamed.
* improved error messages.
* traits reworked:
  * ContainerTraits get **isContiguous**.
  * TextTraits is separate from ContainerTraits, only has **length**, and **addNUL**.
  * buffer traits renamed to **BufferAdapterTraits** and removed difference type.
  * added **TValue** to all trait types, this is used to diagnose better errors.
* BasicBufferReader/Writer is split in two different types: **AdapterReader/Writer** and separate type that enables bit-packing operations  **AdapterBitPacking(Reader/Writer)Wrapper**.
* Serializer/Deserializer reworked
  * No longer copyable, because it stores adapter writer/reader.
  * Removed boolByte, boolBit, and added **boolValue** and it writes bit or byte, depeding on if bit-packing is enabled or not.
  * Bit-packing is enabled by calling **enableBitPacking**, if bitpacking is already enabled, this method will return same instance.
* changed defaults for *DefaultConfig*, BufferSessionsEnabled is false by default, because it doesn't work with input streams.
* serialization config no longer needs typedef *Buffer*.

# [3.0.0](https://github.com/fraillt/bitsery/compare/v2.0.1...v3.0.0) (2017-09-21)

### Features

* refactored interface, now works with C++11 compiler.
* new extension **Growable**, that allows to have forward/backward compatability within this functions serialization flow. It only allows to append new data at the end of to existing flow without breaking old consumers.
  * old consumer: correctly read old interfce and ignore new data.
  * new consumer: get defaults (zero values) for new fields, when reading old data.
* added new extension for associative *map* containers **ContainerMap**.
* friendly static_assert message when serializing **object**, that doesn't have **serialize** function defined.
* added **object** overload, that invokes user function/lambda with object. It is the same as calling user function directly, but makes more consistent API.
* Serializer/Deserializer now have optional *context* parameter, that might be required in some specific serialization cases.
* added traits for custom types specialization, in *details/traits.h*
* improved serialization performance: added support for fixed size buffer for BufferWriter for best performance.
* improved performance for reading/writing container sizes.
* added new method to BufferReader **getError** which returns on of three values: NO_ERROR, BUFFER_OVERFLOW, INVALID_BUFFER_DATA, also added setError method, that is only used by Deserializer.


### Breaking changes

* now all serializer/deserializer functions return void, to avoid undefined behaviour for functions parameters evaluation when using method chaining. There was no benefits apart from *nicer* syntax, but could have undefined behaviour when building complex serialization flows.
* removed **SERIALIZE** macro, and changed interface for all functions that use custom lambdas, to work with C++11. Now lambda function must capture serializer/deserializer.
* to make serializer a little bit *lighter* removed advanced features and made available as extensions via **extend** method
  * removed **range** method, instead added **ValueRange** extension.
  * removed **substitution** method, instead added **Entropy** extension, to sound more familiar to *entropy encoding* term.
* removed **array**, instead added fixed sizes overloads for **container**.
* removed **isValid** method from Deserializer, only BufferReader/Writer store states.
* container and text sizes representation changed, to allow much faster size reads/writes for small values.
* renamed functions:
  * **ext** to **extend** and changed its interface, to make it more easy to extend.
  * alias functions that write bytes directly no has *b* (meaning bytes) at the end of the name eg. *value4* now is *value4b*.
* changed BufferWriter/Reader behaviour:
  * added support for fixed size buffers for better serializer performance (more than 50% improvement). Default config is resizable buffer (*std::vector<uint8_t>*).
  * after serialization, call *getWrittenRange* to get valid range written to buffer, because BufferWritter for resizable buffer now always resize to *capacity* to avoid using *back_insert_iterator* for better performance.
  * BufferReader has constructor with iterators (range), and raw value type pointers (begin, end).
* renamed BufferReader **isCompleted** to  **isCompletedSuccessfully**, that returns true only when there is no errors and buffer is fully read.

### Bug fixes

* **readBytes** was reading in aligned mode, when scratch value from previous bit operations was zero.
* **writeBits** had incorrect assert when writing negative int64_t.

<a name="2.0.1"></a>
# [2.0.1](https://github.com/fraillt/bitsery/compare/v2.0.0...v2.0.1) (2017-08-12)

### Other notes
* added travis build status

<a name="2.0.0"></a>
# [2.0.0](https://github.com/fraillt/bitsery/compare/v1.1.1...v2.0.0) (2017-07-25)

### Features

* endianness support, default network configuration is *little endian*
* added user extensible function **ext**, to work with objects that require different serialization/deserialization path (e.g. pointers)
* **optional** extension (for *ext* function), to work with *std::optional* types

### Bug Fixes
* *align* method fixed in *BufferReader*

### Breaking changes

* file structure changed, added *details* folder.
* no longer support for implicit size converions for all functions (*value*, *array*, *container*), instead added helper functions with specific size, to avoid typing *s.template value<1>...* within serialization function body
* changed parameters order for all functions that use custom function (lambda)
* *BufferReader* and *BufferWriter* is now alias types for real types *BasicBufferReader/Writer&lt;DefaultConfig&gt;* (*DefaultConfig* is defined in *common.h*)


<a name="1.1.1"></a>
# [1.1.1](https://github.com/fraillt/bitsery/compare/v1.0.0...v1.1.1) (2017-02-23)

### Notes
* changed folder structure
* added more BufferReader constructors

# 1.0.0 (2017-02-22)

### Features

Serialization functions:
* **value** - [fundamental types](doc/design/fundamental_types.md)
* **container** - dynamic size containers
* **array** - fixed size containers
* **text** - for c-array and std::string
* **range** - compresion for fundamental types (e.g. int between [255..512] will take up 8bits
* **substitution** - default value from list (e.g. 4d vector, that is most of the time equals to [0,0,0,1] can store only 1bit)
* **boolBit**/**boolByte** - serialize bool, as 1bit or 1byte.
