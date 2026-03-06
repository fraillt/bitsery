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

#ifndef BITSERY_DETAILS_OFFSET_TABLE_READER_H
#define BITSERY_DETAILS_OFFSET_TABLE_READER_H

#include "offset_table.h"
#include "../offset_table_view.h"
#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace bitsery {

namespace details {

struct TableRecord
{
  size_t offset{};
  ot::TableView view{};
};

struct VerifiedOffsetTables
{
  ot::VerifyResult status{ ot::VerifyResult::NoTrailer };
  ot::ViewCtx ctx{};
  std::vector<TableRecord> tables{};
  std::unordered_map<size_t, size_t> tableIndexByOffset{};
  size_t rootIndex{ InvalidTableIndex };
};

inline bool
hasTable(const VerifiedOffsetTables& tables, size_t offset)
{
  return tables.tableIndexByOffset.find(offset) !=
         tables.tableIndexByOffset.end();
}

inline ot::VerifyResult
loadTablesRecursive(const ot::ViewCtx& ctx,
                    size_t offset,
                    const ot::VerifyConfig& cfg,
                    size_t depth,
                    VerifiedOffsetTables& out,
                    size_t& rootIndex)
{
  if (depth > cfg.maxDepth)
    return ot::VerifyResult::TooDeep;
  if (hasTable(out, offset))
    return ot::VerifyResult::Ok;

  ot::TableView tv{};
  ot::VerifyResult res{};
  if (!ot::parseTable(ctx.tables, ctx.tablesSize, offset, cfg, tv, res))
    return res;

  const auto idx = out.tables.size();
  out.tables.push_back(TableRecord{ offset, tv });
  out.tableIndexByOffset.emplace(offset, idx);
  if (depth == 0)
    rootIndex = idx;

  for (uint16_t i = 0; i < tv.hdr.fieldCount; ++i) {
    const auto& e = tv.entries[i];
    if (!ot::entryInBounds(e, ctx.payloadSize))
      return ot::VerifyResult::OutOfBounds;
    if (e.kind == FieldKind::NestedTable) {
      if (e.elemSize <= ctx.payloadSize)
        return ot::VerifyResult::OutOfBounds;
      const auto nestedOffset = static_cast<size_t>(e.elemSize - ctx.payloadSize);
      res =
        loadTablesRecursive(ctx, nestedOffset, cfg, depth + 1, out, rootIndex);
      if (res != ot::VerifyResult::Ok)
        return res;
    }
  }
  return ot::VerifyResult::Ok;
}

inline VerifiedOffsetTables
verifyOffsetTables(const uint8_t* data,
                   size_t size,
                   const ot::VerifyConfig& cfg = {})
{
  VerifiedOffsetTables result{};
  auto res = ot::verifyTrailer(data, size, result.ctx);
  if (res != ot::VerifyResult::Ok) {
    result.status = res;
    return result;
  }

  if (result.ctx.rootTableOffset > result.ctx.tablesSize) {
    result.status = ot::VerifyResult::OutOfBounds;
    return result;
  }

  size_t rootIndex = InvalidTableIndex;
  res = loadTablesRecursive(
    result.ctx, result.ctx.rootTableOffset, cfg, 0u, result, rootIndex);

  result.status = res;
  result.rootIndex = rootIndex;
  if (result.status == ot::VerifyResult::Ok &&
      result.rootIndex == InvalidTableIndex) {
    result.status = ot::VerifyResult::MissingRoot;
  }
  return result;
}

inline const TableRecord*
findTable(const VerifiedOffsetTables& v, size_t offset)
{
  const auto it = v.tableIndexByOffset.find(offset);
  if (it == v.tableIndexByOffset.end() || it->second >= v.tables.size())
    return nullptr;
  return &v.tables[it->second];
}

inline const ot::TableView*
rootTable(const VerifiedOffsetTables& v)
{
  if (v.rootIndex == InvalidTableIndex || v.rootIndex >= v.tables.size())
    return nullptr;
  return &v.tables[v.rootIndex].view;
}

}

}

#endif // BITSERY_DETAILS_OFFSET_TABLE_READER_H
