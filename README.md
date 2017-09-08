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
* No code generation required: no IDL or metadata, use your types directly.
* 2-in-1 declarative control flow, same code for serialization and deserialization.
* Provides forward/backward compatibility for your types.
* Allows fine-grained serialization control using advanced serialization techniques like value ranges, entrophy encoding etc...
* Extendable for types that requires different serialization and deserialization logic (e.g. pointers, or geometry compression).
* Configurable endianess support.

## Why to use bitsery
Read more about the "why" in library [motivation](doc/design/README.md) section.

## How to use it
This documentation comprises these parts:
* [Tutorial](doc/tutorial/README.md) - getting started.
* [Reference section](doc/README.md) - all the details.

## Requirements

## Platforms

This library was tested on
* Windows: Visual Studio 2015, MinGW (gcc 5.2)
* Linux: GCC 5.4, GCC 6.2, Clang 3.9
* OS X Mavericks: AppleClang 8

