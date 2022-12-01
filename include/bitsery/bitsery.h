// MIT License
//
// Copyright (c) 2017 Mindaugas Vinkelis
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef BITSERY_BITSERY_H
#define BITSERY_BITSERY_H

#define BITSERY_MAJOR_VERSION 5
#define BITSERY_MINOR_VERSION 2
#define BITSERY_PATCH_VERSION 2

#define BITSERY_QUOTE_MACRO(name) #name
#define BITSERY_BUILD_VERSION_STR(major, minor, patch)                         \
  BITSERY_QUOTE_MACRO(major)                                                   \
  "." BITSERY_QUOTE_MACRO(minor) "." BITSERY_QUOTE_MACRO(patch)

#define BITSERY_VERSION                                                        \
  BITSERY_BUILD_VERSION_STR(                                                   \
    BITSERY_MAJOR_VERSION, BITSERY_MINOR_VERSION, BITSERY_PATCH_VERSION)

#include "deserializer.h"
#include "serializer.h"

#endif // BITSERY_BITSERY_H
