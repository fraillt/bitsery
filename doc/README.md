To get the most out of **Bitsery**, start with the [tutorial](tutorial/README.md).
Once you're familiar with the library consider the following reference material.

Library design:
* `fundamental types`
* `valueNb instead of value`
* `flexible syntax`
* `serializer/deserializer functions overloads`
* `extending library functionality`
* `errors handling`
* `forward/backward compatibility via Growable extension`
* `pointers`
* `inheritance`
* `polymorphism`


Core Serializer/Deserializer functions (alphabetical order):
* `align` (1.0.0)
* `boolValue` (4.0.0)
* `container` (1.0.0)
* `ext` (2.0.0)
* `context` (3.0.0)
* `context<T>` (4.1.0)
* `contextOrNull<T>` (4.2.0)
* `object` (1.0.0)
* `text` (1.0.0)
* `value` (1.0.0)

Serializer/Deserializer extensions via `ext` method (alphabetical order):
* `BaseClass` (4.2.0)
* `CompactValue` (4.4.0)
* `CompactValueAsObject` (4.4.0)
* `Entropy` (3.0.0)
* `Growable` (3.0.0)
* `PointerOwner` (4.1.0)
* `PointerObserver` (4.1.0)
* `ReferencedByPointer` (4.1.0)
* `StdDuration` (4.6.0)
* `StdMap` (3.0.0)
* `StdOptional` (2.0.0)
* `StdQueue` (4.0.0)
* `StdSet` (4.0.0)
* `StdSmartPrt` (4.3.0)
* `StdStack` (4.0.0)
* `StdTimePoint` (4.6.0)
* `StdTuple` (4.6.0) (requires c++17)
* `StdVariant` (4.6.0) (requires c++17)
* `ValueRange` (3.0.0)
* `VirtualBaseClass` (4.2.0)

AdapterWriter/Reader functions:
* `writeBits/readBits`
* `writeBytes/readBytes`
* `writeBuffer/readBuffer`
* `align`
* `beginSession/endSession`
* `flush (writer only)`
* `writtenBytesCount (writer only)`
* `setError (reader only)`
* `getError (reader only)`
* `isCompletedSuccessfully (reader only)`

Input adapters (buffer and stream) functions:
* `read`
* `error`
* `setError`
* `isCompletedSuccessfully`

Output adapters (buffer and stream) functions:
* `write`
* `flush`
* `writtenBytesCount` (buffer adapter only)


Tips and tricks:
* if you're getting static assert "please define 'serialize' function", please define **serialize** function in same namespace as object, or in **bitsery** namespace, for more info [ADL](https://en.cppreference.com/w/cpp/language/adl).

Other:
* [Contributing](../CONTRIBUTING.md)
* [Change log](../CHANGELOG.md)
