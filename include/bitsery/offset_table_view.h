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

#ifndef BITSERY_OFFSET_TABLE_VIEW_H
#define BITSERY_OFFSET_TABLE_VIEW_H

#include "details/offset_table.h"
#include "common.h"
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

namespace bitsery {

namespace ot {

enum class VerifyResult
{
  Ok,
  NoTrailer,
  BadMagic,
  BadVersion,
  OutOfBounds,
  Misaligned,
  WrongEndianness,
  MissingRoot,
  TooDeep,
  TooManyFields
};

struct VerifyConfig
{
  size_t maxFieldsPerTable{ 1024 };
  size_t maxDepth{ 8 };
};

struct ViewCtx
{
  const uint8_t* payload{};
  size_t payloadSize{};
  const uint8_t* tables{};
  size_t tablesSize{};
  size_t rootTableOffset{};
};

inline VerifyResult
verifyTrailer(const uint8_t* data, size_t size, ViewCtx& ctx)
{
  if (size < sizeof(details::Trailer))
    return VerifyResult::NoTrailer;

  details::Trailer trailer{};
  auto* trailerPos = data + (size - sizeof(details::Trailer));
  std::memcpy(&trailer, trailerPos, sizeof(details::Trailer));

  if (!std::equal(std::begin(details::TRAILER_MAGIC),
                  std::end(details::TRAILER_MAGIC),
                  trailer.magic.begin()))
    return VerifyResult::BadMagic;
  if (trailer.version != details::TRAILER_VERSION)
    return VerifyResult::BadVersion;

  const auto flags = static_cast<details::TrailerFlags>(trailer.flags);
  if (!details::hasFlag(flags, details::TrailerFlags::OffsetsValid))
    return VerifyResult::NoTrailer;
  if (!details::hasFlag(flags, details::TrailerFlags::CrossEndianDisallowed))
    return VerifyResult::NoTrailer;

  if (trailer.rootTableOff > size - sizeof(details::Trailer))
    return VerifyResult::OutOfBounds;
  const auto payloadSize = static_cast<size_t>(trailer.rootTableOff);
  const auto tablesSize = size - sizeof(details::Trailer) - payloadSize;
  ctx.payload = data;
  ctx.payloadSize = payloadSize;
  ctx.tables = data + payloadSize;
  ctx.tablesSize = tablesSize;
  ctx.rootTableOffset =
    static_cast<size_t>(trailer.rootTableOff - payloadSize);

  if (bitsery::DefaultConfig::Endianness != details::getSystemEndianness())
    return VerifyResult::WrongEndianness;
  return VerifyResult::Ok;
}

inline bool
readTableHdr(const uint8_t* ptr,
             size_t remaining,
             details::TableHdr& hdr,
             VerifyResult& res,
             const VerifyConfig& cfg)
{
  if (remaining < sizeof(details::TableHdr)) {
    res = VerifyResult::OutOfBounds;
    return false;
  }
  std::memcpy(&hdr, ptr, sizeof(details::TableHdr));
  if (hdr.fieldCount > cfg.maxFieldsPerTable) {
    res = VerifyResult::TooManyFields;
    return false;
  }
  return true;
}

struct TableView
{
  details::TableHdr hdr{};
  const details::Entry* entries{};
};

inline const details::Entry*
findEntry(const TableView& table, uint16_t fieldId)
{
  for (uint16_t i = 0; i < table.hdr.fieldCount; ++i) {
    if (table.entries[i].fieldId == fieldId)
      return table.entries + i;
  }
  return nullptr;
}

inline bool
parseTable(const uint8_t* base,
           size_t tablesSize,
           size_t offset,
           const VerifyConfig& cfg,
           TableView& out,
           VerifyResult& res)
{
  if (offset > tablesSize) {
    res = VerifyResult::OutOfBounds;
    return false;
  }
  auto* tablePtr = base + offset;
  auto remaining = tablesSize - offset;
  if (!readTableHdr(tablePtr, remaining, out.hdr, res, cfg))
    return false;
  auto entriesBytes =
    static_cast<size_t>(out.hdr.fieldCount) * sizeof(details::Entry);
  if (remaining < sizeof(details::TableHdr) + entriesBytes) {
    res = VerifyResult::OutOfBounds;
    return false;
  }
  out.entries = reinterpret_cast<const details::Entry*>(tablePtr +
                                                        sizeof(details::TableHdr));
  return true;
}

inline bool
entryInBounds(const details::Entry& e, size_t payloadSize)
{
  const auto end = static_cast<size_t>(e.payloadOff) + e.size;
  return end <= payloadSize;
}

inline bool
isAligned(const uint8_t* base, const details::Entry& e, size_t align)
{
  if (align <= 1)
    return true;
  auto* ptr = base + e.payloadOff;
  return reinterpret_cast<uintptr_t>(ptr) % align == 0;
}

template<typename TValue>
struct FieldView
{
  using RawT = typename std::remove_cv<TValue>::type;
  const RawT* value{};
  bool copyOnly{ false };
};

template<typename TValue>
inline FieldView<TValue>
makeFieldView(const ViewCtx& ctx,
              const details::Entry& e,
              VerifyResult& res,
              size_t depth)
{
  (void)depth;
  using RawT = typename std::remove_cv<TValue>::type;
  static_assert(std::is_trivially_copyable<RawT>::value, "");
  FieldView<TValue> out{};
  if (!entryInBounds(e, ctx.payloadSize)) {
    res = VerifyResult::OutOfBounds;
    return out;
  }
  if (details::hasFlag(e.flags, details::FieldFlags::CopyOnly)) {
    out.copyOnly = true;
    return out;
  }
  if (details::hasFlag(e.flags, details::FieldFlags::Aligned) &&
      !isAligned(ctx.payload, e, alignof(RawT))) {
    res = VerifyResult::Misaligned;
    return out;
  }
  if (e.size < sizeof(RawT)) {
    res = VerifyResult::OutOfBounds;
    return out;
  }
  out.value = reinterpret_cast<const RawT*>(ctx.payload + e.payloadOff);
  return out;
}

inline VerifyResult
walkTables(const ViewCtx& ctx,
           size_t rootOffset,
           const VerifyConfig& cfg,
           size_t depth = 0)
{
  if (depth > cfg.maxDepth)
    return VerifyResult::TooDeep;
  VerifyResult res = VerifyResult::Ok;
  TableView tv{};
  if (!parseTable(ctx.tables, ctx.tablesSize, rootOffset, cfg, tv, res))
    return res;
  for (uint16_t i = 0; i < tv.hdr.fieldCount; ++i) {
    const auto& e = tv.entries[i];
    if (!entryInBounds(e, ctx.payloadSize))
      return VerifyResult::OutOfBounds;
    if (e.kind == details::FieldKind::NestedTable) {
      if (e.elemSize >= ctx.payloadSize + ctx.tablesSize)
        return VerifyResult::OutOfBounds;
      auto nestedOffset = e.elemSize - ctx.payloadSize;
      auto nestedRes =
        walkTables(ctx, nestedOffset, cfg, depth + 1);
      if (nestedRes != VerifyResult::Ok)
        return nestedRes;
    }
  }
  return VerifyResult::Ok;
}

template<typename T>
struct OffsetTableView
{
  ViewCtx ctx{};
  TableView root{};
  VerifyResult status{ VerifyResult::NoTrailer };

  OffsetTableView() = default;

  OffsetTableView(const uint8_t* data, size_t size, const VerifyConfig& cfg)
  {
    status = verifyTrailer(data, size, ctx);
    if (status != VerifyResult::Ok)
      return;
    status = walkTables(ctx, ctx.rootTableOffset, cfg);
    if (status != VerifyResult::Ok)
      return;
    VerifyResult res{};
    if (!parseTable(
          ctx.tables, ctx.tablesSize, ctx.rootTableOffset, cfg, root, res))
      status = res;
  }

  bool valid() const { return status == VerifyResult::Ok; }

  const details::Entry* find(uint16_t fieldId) const
  {
    if (!valid())
      return nullptr;
    return findEntry(root, fieldId);
  }

  template<typename TValue>
  FieldView<TValue> field(uint16_t fieldId, VerifyResult& res) const
  {
    const auto* entry = find(fieldId);
    if (!entry) {
      res = VerifyResult::OutOfBounds;
      return {};
    }
    return makeFieldView<TValue>(ctx, *entry, res, 0u);
  }
};

template<typename T>
inline OffsetTableView<T>
makeOffsetTableView(const uint8_t* data,
                    size_t size,
                    const VerifyConfig& cfg = {})
{
  return OffsetTableView<T>(data, size, cfg);
}

}

}

#endif // BITSERY_OFFSET_TABLE_VIEW_H
