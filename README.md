# Bitsery

[![Build Status](https://travis-ci.org/fraillt/bitsery.svg?branch=master)](https://travis-ci.org/fraillt/bitsery)
[![Join the chat at https://gitter.im/bitsery/Lobby](https://badges.gitter.im/bitsery/Lobby.svg)](https://gitter.im/bitsery/Lobby)

Header only C++ binary serialization library.
It is designed around the networking requirements for real-time data delivery, especially for games.

All cross-platform requirements are enforced at compile time, so serialized data do not store any meta-data information and is as small as possible.

> **bitsery** is looking for your feedback on [gitter](https://gitter.im/bitsery/Lobby)

## Features

* Cross-platform compatible.
* Optimized for speed and space.
* No code generation required: no IDL or metadata, just use your types directly.
* Runtime error checking on deserialization,- designed to be save with untrusted network data.
* Provides forward/backward compatibility for your types.
* 2-in-1 declarative control flow, same code for serialization and deserialization.
* Allows fine-grained serialization control using advanced serialization techniques like value ranges, entrophy encoding etc...
* Extendable for flows that requires different serialization and deserialization logic (e.g. pointers serialization, geometry compression, or versioning support).
* Configurable endianess support.
* No macros.

## Why to use bitsery

Look at the numbers, and features list, and deside for your self. *(benchmarked on Ubuntu with GCC 7.1)*

|                                                           | serialize | deserialize | data size   | executable size |
|-----------------------------------------------------------|-----------|-------------|-------------|-----------------|
| flatbuffers                                               | 1852 ms.  | 777 ms.     | 27252 bytes | 74544 bytes     |
| cereal                                                    | 1069 ms.  | 1385 ms.    | 20208 bytes | 72336 bytes     |
| bitsery                                                   | 808 ms.   | 737 ms.     | 14803 bytes | 69784 bytes     |
| bitsery fixed-size buffer                                 | 297 ms.   | 738 ms.     | 14803 bytes | 69928 bytes     |
| bitsery optimized  serialization  flow                    | 686 ms.   | 997 ms.     | 6601 bytes  | 69320 bytes     |
| bitsery optimized serialization flow  + fixed-size buffer | 446 ms.   | 996 ms.     | 6601 bytes  | 69464 bytes     |

If still not convinced read more in library [motivation](doc/design/README.md) section.

## How to use it
This documentation comprises these parts:
* [Tutorial](doc/tutorial/README.md) - getting started.
* [Reference section](doc/README.md) - all the details.

## Requirements

Works with C++11 compiler, no additional dependencies, include `<bitsery/bitsery.h>` and you're done.

## Platforms

This library was tested on
* Windows: Visual Studio 2015, MinGW (gcc 5.2)
* Linux: GCC 5.4, GCC 6.2, Clang 3.9
* OS X Mavericks: AppleClang 8

