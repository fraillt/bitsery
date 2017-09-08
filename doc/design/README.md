## Motivation

Inspiration to create **bitsery** came mainly because there aren't any good alternatives for C++.
Most well-known serialization libraries are *too fat* and tries to solve too many things by supporting multiple data formats (binary, json, xml) and multiple languages (C++, C#, Javascript, etc..) while in the process becomes hard to use, are memory or/and speed inefficient.

The best alternative that I was able to find is [flatbuffers](https://google.github.io/flatbuffers/).
It is fast, memory efficient, and [comparing with other alternatives](https://google.github.io/flatbuffers/flatbuffers_benchmarks.html) looks like *de facto* choice for games.
While Flatbuffers is designed with multiple programming languages support, bitsery is designed specifically for C++.

## A word about JSON

People use C++ because they want speed and memory efficiency, and JSON is not on the list of efficient serialization format.
Although JSON is very readable and very convenient when used together with dynamically typed languages (such as JavaScript).
When serializing data from statically typed languages, however, JSON not only has the obvious drawback of runtime inefficiency, but also forces you to write more code to access data (counterintuitively) due to its dynamic-typing serialization system.
It's also a text format,- human readable, but space inefficient.

Adding optional support for JSON doesn't come for free either.
When there is no multi-language support, we no longer require IDL(interface definition language) to define schemas so we could have consistent interface across multiple languages.
When we no longer have code generation, it becomes imposibble to support JSON *for free* without defining additional metadata, because C++ doesn't have a reflection system (although static reflection was proposed to standard recently).

So, to avoid unnecessary library complexity it is best to forget about JSON, and stick with what machines and C++ is good at,- binary format.

Bitsery is a result of what you can get, when you sacrifice multi-language support and JSON format, but take other *goodies*.

# Bitsery

*todo*