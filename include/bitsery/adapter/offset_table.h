// MIT License
//
// Copyright (c) 2024 Mindaugas Vinkelis and Victor Stewart
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to do so, subject to the
// following conditions:
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

#ifndef BITSERY_ADAPTER_OFFSET_TABLE_H
#define BITSERY_ADAPTER_OFFSET_TABLE_H

#include "../details/offset_table.h"
#include <cassert>
#include <utility>

namespace bitsery {

namespace adapter {

template<typename TAdapter>
class OffsetTableOutput
{
public:
  using BitPackingEnabled = OffsetTableOutput<typename TAdapter::BitPackingEnabled>;
  using TConfig = typename TAdapter::TConfig;
  using TValue = typename TAdapter::TValue;

  template<typename... TArgs>
  OffsetTableOutput(TArgs&&... args)
    : _adapter{ std::forward<TArgs>(args)... }
  {
  }

  template<typename TInnerAdapter>
  OffsetTableOutput(OffsetTableOutput<TInnerAdapter>& other)
    : _adapter{ other.adapter() }
  {}

  OffsetTableOutput(TAdapter& adapter)
    : _adapter{ adapter }
  {}

  template<size_t SIZE, typename T>
  void writeBytes(const T& v)
  {
    _adapter.template writeBytes<SIZE>(v);
  }

  template<size_t SIZE, typename T>
  void writeBuffer(const T* buf, size_t count)
  {
    _adapter.template writeBuffer<SIZE>(buf, count);
  }

  template<typename T>
  void writeBits(const T& v, size_t count)
  {
    _adapter.template writeBits<T>(v, count);
  }

  void currentWritePos(size_t pos) { _adapter.currentWritePos(pos); }

  size_t currentWritePos() const { return _adapter.currentWritePos(); }

  void align() { _adapter.align(); }

  void flush() { _adapter.flush(); }

  size_t writtenBytesCount() const { return _adapter.writtenBytesCount(); }

  template<size_t ALIGNMENT>
  TValue* allocateForDirectWrite(size_t size)
  {
    return _adapter.template allocateForDirectWrite<ALIGNMENT>(size);
  }

  size_t finalize(details::OffsetTableWriterState& state)
  {
    return details::writeTablesAndTrailer(
      _adapter, state, _adapter.writtenBytesCount());
  }

  TAdapter& adapter() { return _adapter; }
  const TAdapter& adapter() const { return _adapter; }

private:
  TAdapter _adapter;
};

}

}

#endif // BITSERY_ADAPTER_OFFSET_TABLE_H
