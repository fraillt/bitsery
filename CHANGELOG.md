<a name="2.0.0"></a>
# [2.0.0](https://github.com/fraillt/bitsery/compare/v1.1.1...v2.0.0) (2017-07-25)

### Features

* Endianness support, default network configuration is *little endian*
* added user extensible function **ext**, to work with objects that require different serialization/deserialization path (e.g. pointers)
* **optional** extension (for *ext* function), to work with *std::optional* types

### Bug Fixes
* *align* method fixed in *BufferReader*

### Other notes

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
* **value** - primitive types (ints, enums, floats)
* **container** - dynamic size containers
* **array** - fixed size containers
* **text** - for c-array and std::string
* **range** - compresion for primitive types (e.g. int between [255..512] will take up 8bits
* **substitution** - default value from list (e.g. 4d vector, that is most of the time equals to [0,0,0,1] can store only 1bit)
* **boolBit**/**boolByte** - serialize bool, as 1bit or 1byte.