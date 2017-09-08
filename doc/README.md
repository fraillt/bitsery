To get the most out of **Bitsery**, start with the [tutorial](tutorial/README.md).
Once you're familiar with the library consider the following reference material.

Library design:
* `valueNb instead of value`
* `fundamental types`
* `serializer/deserializer functions overloads`
* `extending library functionality`
* `errors handling`
* `forward/backward compatibility via growable`

Core Serializer/Deserializer functions (alphabetical order):
* `boolByte`
* `container`
* `object`
* `text`
* `value`

Advanced Serializer/Deserializer functions (alphabetical order):
* `align`
* `boolBit`
* `entropy`
* `extend`
* `growable`
* `range`

BasicBufferWriter/Reader functions:
* `writeBits/readBits`
* `writeBytes/readBytes`
* `writeBuffer/readBuffer`
* `align`
* `beginSession/endSession`
* `flush (writer only)`
* `setError (reader only)`
* `getError (reader only)`
* `isCompletedSuccessfully (reader only)`



Tips and tricks:
* if you're getting static assert "please define 'serialize' function", most likely it is because your SERIALIZE function is not defined in same namespace as object.

Limitations:
* max **text** or **container** size can be 2^(n-2) (where n = sizeof(std::size_t) * 8) for 32-bit systems it is 1073741823 (0x3FFFFFF).
* when using **growable** serialized buffer cannot be greater than 2^(n-2) (where n = sizeof(std::size_t) * 8).

Other:
* [Contributing](../CONTRIBUTING.md)
* [Change log](../CHANGELOG.md)


