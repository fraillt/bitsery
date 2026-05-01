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

#include <gtest/gtest.h>
#include <cstring>
#include <utility>
#include "bitsery/details/offset_table_reader.h"
#include "bitsery/details/offset_table.h"
#include "bitsery/offset_table_view.h"

using namespace bitsery;
using namespace bitsery::details;
using namespace bitsery::ot;

namespace {

using Buffer = std::vector<uint8_t>;

template<typename T>
void appendObject(Buffer& buf, const T& value)
{
  auto* ptr = reinterpret_cast<const uint8_t*>(&value);
  buf.insert(buf.end(), ptr, ptr + sizeof(T));
}

size_t appendTable(Buffer& buf, uint16_t typeVersion, const std::vector<Entry>& entries)
{
  const auto offset = buf.size();
  TableHdr hdr{};
  hdr.fieldCount = static_cast<uint16_t>(entries.size());
  hdr.typeVersion = typeVersion;
  appendObject(buf, hdr);
  for (const auto& entry : entries) {
    appendObject(buf, entry);
  }
  return offset;
}

void appendTrailer(Buffer& buf,
                   size_t rootTableOff,
                   TrailerFlags flags = TrailerFlags::OffsetsValid |
                                        TrailerFlags::CrossEndianDisallowed,
                   uint8_t version = TRAILER_VERSION,
                   bool validMagic = true)
{
  Trailer trailer{};
  if (validMagic) {
    std::copy(
      std::begin(TRAILER_MAGIC), std::end(TRAILER_MAGIC), trailer.magic.begin());
  }
  trailer.version = version;
  trailer.flags = static_cast<uint8_t>(flags);
  trailer.rootTableOff = static_cast<uint32_t>(rootTableOff);
  appendObject(buf, trailer);
}

Buffer makeBuffer(const std::vector<uint8_t>& payload,
                  const std::vector<std::pair<uint16_t, std::vector<Entry>>>& tables,
                  size_t rootTableIndex = 0u,
                  TrailerFlags flags = TrailerFlags::OffsetsValid |
                                       TrailerFlags::CrossEndianDisallowed,
                  uint8_t version = TRAILER_VERSION,
                  bool validMagic = true)
{
  Buffer buf = payload;
  std::vector<size_t> offsets;
  offsets.reserve(tables.size());
  for (const auto& table : tables) {
    offsets.push_back(appendTable(buf, table.first, table.second));
  }
  appendTrailer(buf, offsets[rootTableIndex], flags, version, validMagic);
  return buf;
}

size_t firstMisalignedOffset(const uint8_t* base, size_t alignment, size_t limit)
{
  for (size_t offset = 0; offset < limit; ++offset) {
    if ((reinterpret_cast<uintptr_t>(base + offset) % alignment) != 0u)
      return offset;
  }
  return limit;
}

} // namespace

TEST(OffsetTableReader, FailsWithoutTrailer)
{
  std::vector<uint8_t> buf(8, 0);
  auto res = verifyOffsetTables(buf.data(), buf.size());
  EXPECT_NE(res.status, VerifyResult::Ok);
}

TEST(OffsetTableReader, DetectsTrailerAndRoot)
{
  // minimal payload + empty table + trailer
  std::vector<uint8_t> buf;
  // payload: 4 bytes
  buf.resize(4, 0x11);
  // table (empty)
  TableHdr hdr{};
  hdr.fieldCount = 0;
  hdr.typeVersion = 0;
  const auto tableOff = buf.size();
  auto hdrPtr = reinterpret_cast<const uint8_t*>(&hdr);
  buf.insert(buf.end(), hdrPtr, hdrPtr + sizeof(hdr));
  // trailer
  Trailer tr{};
  std::copy(std::begin(TRAILER_MAGIC), std::end(TRAILER_MAGIC), tr.magic.begin());
  tr.version = TRAILER_VERSION;
  tr.flags = static_cast<uint8_t>(TrailerFlags::OffsetsValid |
                                  TrailerFlags::CrossEndianDisallowed);
  tr.rootTableOff = static_cast<uint32_t>(tableOff);
  auto trPtr = reinterpret_cast<const uint8_t*>(&tr);
  buf.insert(buf.end(), trPtr, trPtr + sizeof(tr));

  auto res = verifyOffsetTables(buf.data(), buf.size());
  EXPECT_EQ(res.status, VerifyResult::Ok);
  ASSERT_NE(res.rootIndex, InvalidTableIndex);
  auto* root = rootTable(res);
  ASSERT_NE(root, nullptr);
  EXPECT_EQ(root->hdr.fieldCount, 0);
}

TEST(OffsetTableReader, RejectsBadMagic)
{
  auto buf = makeBuffer({ 0x11, 0x22, 0x33, 0x44 }, { { 0u, {} } }, 0u, TrailerFlags::OffsetsValid |
                                                                        TrailerFlags::CrossEndianDisallowed,
                        TRAILER_VERSION,
                        false);

  auto res = verifyOffsetTables(buf.data(), buf.size());
  EXPECT_EQ(res.status, VerifyResult::BadMagic);
}

TEST(OffsetTableReader, RejectsBadVersion)
{
  auto buf = makeBuffer({ 0x11, 0x22, 0x33, 0x44 },
                        { { 0u, {} } },
                        0u,
                        TrailerFlags::OffsetsValid |
                          TrailerFlags::CrossEndianDisallowed,
                        static_cast<uint8_t>(TRAILER_VERSION + 1));

  auto res = verifyOffsetTables(buf.data(), buf.size());
  EXPECT_EQ(res.status, VerifyResult::BadVersion);
}

TEST(OffsetTableReader, RejectsMissingRequiredFlags)
{
  auto buf = makeBuffer(
    { 0x11, 0x22, 0x33, 0x44 }, { { 0u, {} } }, 0u, TrailerFlags::OffsetsValid);

  auto res = verifyOffsetTables(buf.data(), buf.size());
  EXPECT_EQ(res.status, VerifyResult::NoTrailer);
}

TEST(OffsetTableReader, RejectsTruncatedEntryArray)
{
  Buffer buf(4u, 0x55);
  TableHdr hdr{};
  hdr.fieldCount = 1u;
  hdr.typeVersion = 0u;
  appendObject(buf, hdr);
  appendTrailer(buf, 4u);

  auto res = verifyOffsetTables(buf.data(), buf.size());
  EXPECT_EQ(res.status, VerifyResult::OutOfBounds);
}

TEST(OffsetTableReader, RejectsTableThatExceedsConfiguredFieldLimit)
{
  Entry first{};
  first.fieldId = 1u;
  first.kind = FieldKind::Scalar;
  first.payloadOff = 0u;
  first.size = 0u;

  Entry second = first;
  second.fieldId = 2u;

  auto buf =
    makeBuffer({ 0xAB }, { { 0u, { first, second } } });

  VerifyConfig cfg{};
  cfg.maxFieldsPerTable = 1u;

  auto res = verifyOffsetTables(buf.data(), buf.size(), cfg);
  EXPECT_EQ(res.status, VerifyResult::TooManyFields);
}

TEST(OffsetTableReader, RejectsOutOfBoundsNestedTableOffset)
{
  const size_t payloadSize = 1u;
  Entry nested{};
  nested.fieldId = 1u;
  nested.kind = FieldKind::NestedTable;
  nested.payloadOff = 0u;
  nested.size = 0u;
  nested.elemSize = static_cast<uint32_t>(payloadSize + 999u);

  auto buf = makeBuffer({ 0xAA }, { { 0u, { nested } } });

  auto res = verifyOffsetTables(buf.data(), buf.size());
  EXPECT_EQ(res.status, VerifyResult::OutOfBounds);
}

TEST(OffsetTableReader, RejectsTablesDeeperThanConfiguredLimit)
{
  const size_t payloadSize = 1u;
  const auto childOffset = sizeof(TableHdr) + sizeof(Entry);

  Entry nested{};
  nested.fieldId = 1u;
  nested.kind = FieldKind::NestedTable;
  nested.payloadOff = 0u;
  nested.size = 0u;
  nested.elemSize = static_cast<uint32_t>(payloadSize + childOffset);

  auto buf = makeBuffer({ 0xAA }, { { 0u, { nested } }, { 0u, {} } });

  VerifyConfig cfg{};
  cfg.maxDepth = 0u;

  auto res = verifyOffsetTables(buf.data(), buf.size(), cfg);
  EXPECT_EQ(res.status, VerifyResult::TooDeep);
}

TEST(OffsetTableView, BuildsTypedZeroCopyViewForScalarField)
{
  uint32_t value = 0xAABBCCDDu;
  std::vector<uint8_t> payload(sizeof(value), 0u);
  std::memcpy(payload.data(), &value, sizeof(value));

  Entry entry{};
  entry.fieldId = 1u;
  entry.kind = FieldKind::Scalar;
  entry.payloadOff = 0u;
  entry.size = sizeof(value);
  entry.elemSize = sizeof(value);

  auto buf = makeBuffer(payload, { { 0u, { entry } } });
  auto view = makeOffsetTableView<uint32_t>(buf.data(), buf.size());

  ASSERT_TRUE(view.valid());
  ASSERT_EQ(view.root.hdr.fieldCount, 1u);

  VerifyResult res = VerifyResult::Ok;
  auto fv = makeFieldView<uint32_t>(view.ctx, view.root.entries[0], res, 0u);
  ASSERT_EQ(res, VerifyResult::Ok);
  ASSERT_NE(fv.value, nullptr);
  EXPECT_FALSE(fv.copyOnly);
  EXPECT_EQ(*fv.value, value);

  res = VerifyResult::Ok;
  auto byId = view.field<uint32_t>(1u, res);
  ASSERT_EQ(res, VerifyResult::Ok);
  ASSERT_NE(byId.value, nullptr);
  EXPECT_EQ(*byId.value, value);
  EXPECT_EQ(view.find(2u), nullptr);
}

TEST(OffsetTableView, MarksCopyOnlyFieldWithoutExposingTypedPointer)
{
  uint32_t value = 0xAABBCCDDu;
  std::vector<uint8_t> payload(sizeof(value), 0u);
  std::memcpy(payload.data(), &value, sizeof(value));

  Entry entry{};
  entry.fieldId = 1u;
  entry.kind = FieldKind::Scalar;
  entry.flags = FieldFlags::CopyOnly;
  entry.payloadOff = 0u;
  entry.size = sizeof(value);
  entry.elemSize = sizeof(value);

  ViewCtx ctx{};
  ctx.payload = payload.data();
  ctx.payloadSize = payload.size();

  VerifyResult res = VerifyResult::Ok;
  auto fv = makeFieldView<uint32_t>(ctx, entry, res, 0u);
  EXPECT_EQ(res, VerifyResult::Ok);
  EXPECT_TRUE(fv.copyOnly);
  EXPECT_EQ(fv.value, nullptr);
}

TEST(OffsetTableView, RejectsMisalignedAlignedFieldView)
{
  std::vector<uint8_t> payload(16u, 0u);
  const auto offset =
    firstMisalignedOffset(payload.data(), alignof(uint32_t), payload.size() - 4u);
  ASSERT_LT(offset, payload.size() - 4u);

  uint32_t value = 0x01020304u;
  std::memcpy(payload.data() + offset, &value, sizeof(value));

  Entry entry{};
  entry.fieldId = 1u;
  entry.kind = FieldKind::Scalar;
  entry.flags = FieldFlags::Aligned;
  entry.payloadOff = static_cast<uint32_t>(offset);
  entry.size = sizeof(value);
  entry.elemSize = sizeof(value);

  ViewCtx ctx{};
  ctx.payload = payload.data();
  ctx.payloadSize = payload.size();

  VerifyResult res = VerifyResult::Ok;
  auto fv = makeFieldView<uint32_t>(ctx, entry, res, 0u);
  EXPECT_EQ(res, VerifyResult::Misaligned);
  EXPECT_FALSE(fv.copyOnly);
  EXPECT_EQ(fv.value, nullptr);
}
