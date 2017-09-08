# [3.0.0](https://github.com/fraillt/bitsery/compare/v2.0.1...v3.0.0) (2017-09-02)

### Features

* new function **growable**, that allows to have forward/backward compatability within this functions serialization flow. It only allows to append new data at the end of to existing flow without breaking old consumers.
  * old consumer: correctly read old interfce and ignore new data.
  * new consumer: get defaults (zero values) for new fields, when reading old data.
* new **SERIALIZE_FRIEND** macro that enables to serialize private object fields.
* friendly static_assert message when serializing **object**, that doesn't have **serialize** function defined.
* added **object** overload, that invokes user function/lambda with object. It is the same as calling user function directly, but makes more consistent API.
* Serializer/Deserializer now have optional *context* parameter, that might be required in some specific serialization cases.
* improved serialization performance: added support for fixed size buffer for best performance.
* improved performance for reading/writing container sizes.
* added new method to BufferReader **getError** which returns on of three values: NO_ERROR, BUFFER_OVERFLOW, INVALID_BUFFER_DATA, also added setError method, that is only used by Deserializer.

### Breaking changes

* container and text sizes representation changed, to allow much faster size reads/writes for small values.
* renamed functions:
  * **ext** to **extend** and changed its interface, to make it more easy to extend.
  * alias functions that write bytes directly no has *b* (meaning bytes) at the end of the name eg. *value4* now is *value4b*.
  * **substitution** changed to **entropy**, to sound more familiar to *entropy encoding* term.
* now all serializer/deserializer functions return void, to avoid undefined behaviour for functions parameters evaluation when using method chaining. There was no benefits apart from *nicer* syntax, but could have undefined behaviour when building complex serialization flows.
* removed **array** and added fixed sizes overloads for **container**.
* changed BufferWriter/Reader behaviour:
  * added *FixedBufferSize* config bool parameter for *BufferWriter* for better serializer performance (more than 50% improvement). Default config is resizable buffer (*std::vector<uint8_t>*).
  * after serialization, call *getWrittenRange* to get valid range written to buffer, because BufferWritter for resizable buffer now always resize to *capacity* to avoid using *back_insert_iterator* for better performance.
  * BufferReader has constructor with iterators (range), and raw value type pointers (begin, end).
* removed **isValid** method from Deserializer, only BufferReader/Writer store states.
* renamed BufferReader **isCompleted** to  **isCompletedSuccessfully**, that returns true only when there is no errors and buffer is fully read.

### Bug fixes

* **readBytes** was reading in aligned mode, when scratch value from previous bit operations was zero.



<a name="2.0.1"></a>
# [2.0.1](https://github.com/fraillt/bitsery/compare/v2.0.0...v2.0.1) (2017-08-12)

### Other notes
* added travis build status

<a name="2.0.0"></a>
# [2.0.0](https://github.com/fraillt/bitsery/compare/v1.1.1...v2.0.0) (2017-07-25)

### Features

* Endianness support, default network configuration is *little endian*
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
