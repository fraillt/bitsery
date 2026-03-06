// MIT License
//
// Copyright (c) 2024 Mindaugas Vinkelis and Victor Stewart
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

#ifndef BITSERY_EXT_OFFSET_TABLE_H
#define BITSERY_EXT_OFFSET_TABLE_H

#include "../serializer.h"
#include "../details/offset_table.h"
#include "../details/offset_table_serializer.h"
#include <cassert>
#include <utility>

namespace bitsery {

namespace ext {

struct OffsetTableConfig
{
  static constexpr bool Enable = true;
};

template<typename TOutputAdapter>
using OffsetTableSerializer = details::OffsetTableWriteSerializer<TOutputAdapter>;

template<typename TAdapter, typename TContext>
inline details::OffsetTableWriterState*
offsetTableState(Serializer<TAdapter, TContext>& ser)
{
  return ser.template contextOrNull<details::OffsetTableWriterState>();
}

template<typename TAdapter>
inline details::OffsetTableWriterState*
offsetTableState(details::OffsetTableWriteSerializer<TAdapter>& ser)
{
  return std::addressof(ser.state());
}

template<typename TAdapter, typename T>
inline const details::StaticCacheEntry*
cachedStaticOffsetTable(TAdapter& adapter)
{
  if constexpr (!details::FieldRegistry<T>::Enabled ||
                !details::HasWrittenBytesCount<TAdapter>::value) {
    return nullptr;
  } else {
    if (adapter.writtenBytesCount() != 0u) {
      return nullptr;
    }
    return details::cachedStaticRootEntry<T>();
  }
}

template<typename TAdapter, typename T>
inline size_t
serializePayloadWithCachedOffsetTable(TAdapter adapter,
                                      const T& value,
                                      const details::StaticCacheEntry& cached)
{
  bitsery::Serializer<TAdapter> ser{ std::move(adapter) };
  ser.object(value);
  ser.adapter().flush();
  const auto payloadSize = ser.adapter().writtenBytesCount();
  return details::writeCachedTablesAndTrailer(
    ser.adapter(), cached, payloadSize);
}

template<typename TSerializer>
inline details::TableScope
beginOffsetTable(TSerializer& ser, uint16_t typeVersion = 0)
{
  auto* state = offsetTableState(ser);
  assert(state);
  return details::TableScope(state->recorder, typeVersion);
}

inline details::TableScope::TableIndex
endOffsetTable(details::TableScope& scope)
{
  return scope.pop();
}

template<typename TAdapter, typename TContext>
inline details::FieldOffsetScope<TAdapter>
makeFieldScope(Serializer<TAdapter, TContext>& ser,
               uint16_t fieldId,
               details::FieldKind kind,
               details::FieldFlags flags,
               uint32_t elemSize,
               typename details::OffsetTableRecorder::TableIndex nestedIdx =
                 details::InvalidTableIndex)
{
  auto* state = offsetTableState(ser);
  assert(state);
  return details::FieldOffsetScope<TAdapter>(
    state->recorder, ser.adapter(), fieldId, kind, flags, elemSize, nestedIdx);
}

template<typename TAdapter>
inline details::FieldOffsetScope<TAdapter>
makeFieldScope(details::OffsetTableWriteSerializer<TAdapter>& ser,
               uint16_t fieldId,
               details::FieldKind kind,
               details::FieldFlags flags,
               uint32_t elemSize,
               typename details::OffsetTableRecorder::TableIndex nestedIdx =
                 details::InvalidTableIndex)
{
  auto* state = offsetTableState(ser);
  assert(state);
  return details::FieldOffsetScope<TAdapter>(
    state->recorder, ser.adapter(), fieldId, kind, flags, elemSize, nestedIdx);
}

template<typename TSerializer>
inline size_t
finalizeOffsetTable(TSerializer& ser)
{
  auto* state = offsetTableState(ser);
  assert(state);
  ser.adapter().flush();
  const auto payloadSize = ser.adapter().writtenBytesCount();
  return details::writeTablesAndTrailer(
    ser.adapter(), *state, payloadSize);
}

template<typename TAdapter, typename T>
inline size_t
serializeWithOffsetTable(details::OffsetTableWriterState& state,
                         TAdapter adapter,
                         const T& value)
{
  if (!details::FieldRegistry<T>::Enabled)
    return bitsery::quickSerialization(std::move(adapter), value);
  if constexpr (details::IsStreamAdapter<TAdapter>::value) {
    state.clear();
    return bitsery::quickSerialization(std::move(adapter), value);
  }
  if (const auto* cached = cachedStaticOffsetTable<TAdapter, T>(adapter)) {
    state.clear();
    return serializePayloadWithCachedOffsetTable(
      std::move(adapter), value, *cached);
  }
  details::OffsetTableWriteSerializer<TAdapter> ser{ state, std::move(adapter) };
  ser.object(value);
  ser.adapter().flush();
  return ser.finalize();
}

template<typename TAdapter, typename T>
inline size_t
serializeWithOffsetTable(TAdapter adapter, const T& value)
{
  if (!details::FieldRegistry<T>::Enabled)
    return bitsery::quickSerialization(std::move(adapter), value);
  if constexpr (details::IsStreamAdapter<TAdapter>::value)
    return bitsery::quickSerialization(std::move(adapter), value);
  if (const auto* cached = cachedStaticOffsetTable<TAdapter, T>(adapter))
    return serializePayloadWithCachedOffsetTable(
      std::move(adapter), value, *cached);
  auto state = details::acquireOffsetTableWriterState();
  return serializeWithOffsetTable(state.get(), std::move(adapter), value);
}

}

}

#endif // BITSERY_EXT_OFFSET_TABLE_H
