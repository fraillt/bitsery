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


Core Serializer/Deserializer functions (alphabetical order):
* `align`
* `boolValue`
* `container`
* `ext`
* `context`
* `object`
* `text`
* `value`

Serializer/Deserializer extensions via `ext` method (alphabetical order):
* `Entropy`
* `Growable`
* `PointerOwner`
* `PointerObserver`
* `ReferencedByPointer`
* `StdMap`
* `StdOptional`
* `StdQueue`
* `StdSet`
* `StdStack`
* `ValueRange`

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
* if you're getting static assert "please define 'serialize' function", most likely it is because your **serialize** function is not defined in same namespace as object.

Limitations:
* max **text** or **container** size can be 2^(n-2) (where n = sizeof(std::size_t) * 8) for 32-bit systems it is 1073741823 (0x3FFFFFF).

Other:
* [Contributing](../CONTRIBUTING.md)
* [Change log](../CHANGELOG.md)
