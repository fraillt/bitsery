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

#ifndef BITSERY_OFFSET_TABLE_INSPECT_H
#define BITSERY_OFFSET_TABLE_INSPECT_H

#include "details/offset_table.h"
#include "details/offset_table_reader.h"
#include "deserializer.h"
#include "adapter/buffer.h"
#include "traits/core/traits.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <limits>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace bitsery {

namespace ot {

enum class InspectStatus
{
  Ok,
  NoTrailer,
  BadMagic,
  BadVersion,
  OutOfBounds,
  WrongEndianness,
  MissingRoot,
  TooDeep,
  TooManyFields
};

enum class FieldReason
{
  None,
  CopyOnly,
  Unsupported,
  Misaligned,
  OutOfBounds,
  RegistryMismatch,
  SizeMismatch
};

struct InspectConfig
{
  VerifyConfig verifyCfg{};
  size_t maxFieldNodes{ 65536 };
};

template<typename TAdapter>
struct FieldNode;

namespace detail {

struct ByteSpan
{
  const uint8_t* data{ nullptr };
  size_t length{ 0 };

  const uint8_t* begin() const { return data; }
  const uint8_t* end() const { return data + length; }
  size_t size() const { return length; }
};

} // namespace detail

} // namespace ot

namespace traits {

template<>
struct ContainerTraits<bitsery::ot::detail::ByteSpan>
{
  static constexpr bool isResizable = false;
  static constexpr bool isContiguous = true;
  static constexpr bool isDynamicSized = true;
  using TIterator = const uint8_t*;
  using TConstIterator = const uint8_t*;
  using TValue = uint8_t;
  using TSizeType = size_t;
  static TConstIterator begin(const bitsery::ot::detail::ByteSpan& s)
  {
    return s.begin();
  }
  static TConstIterator end(const bitsery::ot::detail::ByteSpan& s)
  {
    return s.end();
  }
  static size_t size(const bitsery::ot::detail::ByteSpan& s)
  {
    return s.size();
  }
};

template<>
struct BufferAdapterTraits<bitsery::ot::detail::ByteSpan>
{
  using TValue = uint8_t;
  using TIterator = const uint8_t*;
  using TConstIterator = const uint8_t*;
};

} // namespace traits

namespace ot {

namespace detail {

inline constexpr size_t InvalidNodeIndex =
  std::numeric_limits<size_t>::max();

template<typename TAdapter>
struct InspectContext;

template<typename TAdapter>
struct InspectAccess;

template<typename TAdapter>
struct ConstructInvoker;

}

template<typename TAdapter>
struct FieldNode
{
  uint16_t fieldId{ 0 };
  details::FieldKind kind{ details::FieldKind::Scalar };
  details::FieldFlags flags{ details::FieldFlags::None };
  FieldReason reason{ FieldReason::None };
  bool viewable{ false };
  uint32_t elemSize{ 0 };
  uint16_t typeVersion{ 0 };
  uint16_t nestedFieldCount{ 0 };
  size_t payloadOffset{ 0 };
  size_t payloadSize{ 0 };
  size_t firstChild{ detail::InvalidNodeIndex };
  size_t childCount{ 0 };
  const details::FieldInfo* registryEntry{ nullptr };

  bool hasChildren() const { return childCount != 0u; }
  const FieldNode* child(size_t idx) const;
  const FieldNode* findChildById(uint16_t id) const;
  bool canViewBytes() const;
  std::pair<const uint8_t*, size_t> bytes() const;
  template<typename U>
  void constructInto(U& value) const;
  template<typename U>
  U construct() const;

private:
  const detail::InspectContext<TAdapter>* _ctx{ nullptr };
  friend struct detail::InspectAccess<TAdapter>;
  friend struct detail::ConstructInvoker<TAdapter>;
};

namespace detail {

template<typename TAdapter>
struct InspectContext
{
  const uint8_t* data{ nullptr };
  size_t totalSize{ 0 };
  const uint8_t* payload{ nullptr };
  size_t payloadSize{ 0 };
  const std::vector<FieldNode<TAdapter>>* nodes{ nullptr };
};

template<typename Adapter, typename = void>
struct AdapterConfig
{
  using type = bitsery::DefaultConfig;
};

template<typename Adapter>
struct AdapterConfig<Adapter, std::void_t<typename Adapter::TConfig>>
{
  using type = typename Adapter::TConfig;
};

template<typename TAdapter>
struct InspectAccess
{
  static void bind(std::vector<FieldNode<TAdapter>>& nodes,
                   InspectContext<TAdapter>& ctx,
                   const uint8_t* data,
                   size_t totalSize,
                   const uint8_t* payload,
                   size_t payloadSize)
  {
    ctx.data = data;
    ctx.totalSize = totalSize;
    ctx.payload = payload;
    ctx.payloadSize = payloadSize;
    ctx.nodes = std::addressof(nodes);
    for (auto& n : nodes) {
      n._ctx = std::addressof(ctx);
    }
  }

  static const std::vector<FieldNode<TAdapter>>* nodes(
    const InspectContext<TAdapter>* ctx)
  {
    return ctx ? ctx->nodes : nullptr;
  }
};

template<typename TAdapter>
struct ConstructInvoker
{
  template<typename U>
  static void construct(const FieldNode<TAdapter>& node, U& value)
  {
    if (node.reason == FieldReason::RegistryMismatch ||
        node.reason == FieldReason::SizeMismatch ||
        node.reason == FieldReason::Misaligned ||
        node.reason == FieldReason::OutOfBounds) {
      return;
    }

    const auto* ctx = node._ctx;
    if (!ctx)
      return;
    if (!ctx->payload || node.payloadSize == 0u)
      return;
    if (node.payloadOffset > ctx->payloadSize)
      return;
    const auto remaining = ctx->payloadSize - node.payloadOffset;
    if (node.payloadSize > remaining)
      return;

    detail::ByteSpan span{};
    span.data = ctx->payload + node.payloadOffset;
    span.length = node.payloadSize;

    using Config = typename detail::AdapterConfig<TAdapter>::type;
    InputBufferAdapter<detail::ByteSpan, Config> adapter{
      span.begin(), span.size()
    };
    setEndIfSupported(adapter, span.size());
    Deserializer<decltype(adapter)> des{ std::move(adapter) };
    readValue(node, des, value);
  }

private:
  template<typename Des, typename U>
  static void readValue(const FieldNode<TAdapter>& node, Des& des, U& value)
  {
    using RawU = typename std::remove_cv<U>::type;
    if constexpr (std::is_enum<RawU>::value || std::is_integral<RawU>::value) {
      if constexpr (sizeof(RawU) == 1) {
        des.value1b(value);
      } else if constexpr (sizeof(RawU) == 2) {
        des.value2b(value);
      } else if constexpr (sizeof(RawU) == 4) {
        des.value4b(value);
      } else if constexpr (sizeof(RawU) == 8) {
        des.value8b(value);
      } else {
        des.object(value);
      }
    } else if constexpr (std::is_floating_point<RawU>::value) {
      if constexpr (sizeof(RawU) == 4) {
        des.value4b(value);
      } else if constexpr (sizeof(RawU) == 8) {
        des.value8b(value);
      } else {
        des.object(value);
      }
    } else if constexpr (IsStdVector<RawU>::value) {
      if (tryCopyVector(node, value))
        return;
      if constexpr (details::HasSerializeFunction<Des, RawU>::value ||
                    details::HasSerializeMethod<Des, RawU>::value)
        des.object(value);
      return;
    } else if constexpr (IsStdBasicString<RawU>::value) {
      if (tryCopyString(node, value))
        return;
      if constexpr (details::HasSerializeFunction<Des, RawU>::value ||
                    details::HasSerializeMethod<Des, RawU>::value)
        des.object(value);
      return;
    } else {
      des.object(value);
    }
  }

  template<typename Adapter>
  static auto setEndIfSupported(Adapter& a, size_t end)
    -> decltype(a.currentReadEndPos(end), void())
  {
    a.currentReadEndPos(end);
  }

  static void setEndIfSupported(...) {}

  template<typename RawU>
  struct IsStdVector : std::false_type
  {};

  template<typename T, typename Alloc>
  struct IsStdVector<std::vector<T, Alloc>> : std::true_type
  {};

  template<typename RawU>
  struct IsStdBasicString : std::false_type
  {};

  template<typename Char, typename Traits, typename Alloc>
  struct IsStdBasicString<std::basic_string<Char, Traits, Alloc>>
    : std::true_type
  {};

  static std::pair<const uint8_t*, size_t>
  payloadBytes(const FieldNode<TAdapter>& node)
  {
    const auto* ctx = node._ctx;
    if (!ctx || !ctx->payload)
      return { nullptr, 0u };
    if (node.payloadOffset > ctx->payloadSize)
      return { nullptr, 0u };
    const auto remaining = ctx->payloadSize - node.payloadOffset;
    if (node.payloadSize > remaining)
      return { nullptr, 0u };
    return { ctx->payload + node.payloadOffset, node.payloadSize };
  }

  static bool readSizePrefix(const uint8_t*& data,
                             size_t& remaining,
                             size_t& size)
  {
    if (remaining == 0u)
      return false;
    const uint8_t hb = *data++;
    --remaining;
    if (hb < 0x80u) {
      size = hb;
      return true;
    }
    if (remaining == 0u)
      return false;
    const uint8_t lb = *data++;
    --remaining;
    if (hb & 0x40u) {
      if (remaining < 2u)
        return false;
      const uint16_t lw =
        static_cast<uint16_t>(data[0] | (static_cast<uint16_t>(data[1]) << 8));
      data += 2;
      remaining -= 2;
      size = ((((static_cast<size_t>(hb) & 0x3Fu) << 8) | lb) << 16) | lw;
    } else {
      size = ((static_cast<size_t>(hb) & 0x7Fu) << 8) | lb;
    }
    return true;
  }

  template<typename RawU>
  static bool tryCopyVector(const FieldNode<TAdapter>& node, RawU& value)
  {
    if constexpr (IsStdVector<RawU>::value) {
      using Elem = typename RawU::value_type;
      if constexpr (!std::is_trivially_copyable<Elem>::value)
        return false;

      const auto bytes = payloadBytes(node);
      if (!bytes.first)
        return false;
      size_t elemSize = node.elemSize == 0u ? sizeof(Elem)
                                            : static_cast<size_t>(node.elemSize);
      if (elemSize == 0u)
        return false;
      const uint8_t* data = bytes.first;
      size_t remaining = bytes.second;
      size_t length = 0u;
      if (!readSizePrefix(data, remaining, length))
        return false;
      if (length == 0u) {
        value.clear();
        return true;
      }
      if ((remaining % length) != 0u)
        return false;
      const auto serializedElemSize = remaining / length;
      if (serializedElemSize != elemSize || serializedElemSize != sizeof(Elem))
        return false;
      const auto byteCount = length * serializedElemSize;
      value.resize(length);
      if (byteCount != 0u)
        std::memcpy(value.data(), data, byteCount);
      return true;
    }
    return false;
  }

  template<typename RawU>
  static bool tryCopyString(const FieldNode<TAdapter>& node, RawU& value)
  {
    if constexpr (IsStdBasicString<RawU>::value &&
                  std::is_same<typename RawU::value_type, char>::value) {
      const auto bytes = payloadBytes(node);
      if (!bytes.first)
        return false;
      const uint8_t* data = bytes.first;
      size_t remaining = bytes.second;
      size_t length = 0u;
      if (!readSizePrefix(data, remaining, length))
        return false;
      if (length > remaining)
        return false;
      value.assign(reinterpret_cast<const char*>(data), length);
      return true;
    }
    return false;
  }
};

}

template<typename TAdapter>
inline const FieldNode<TAdapter>*
FieldNode<TAdapter>::child(size_t idx) const
{
  if (!_ctx || childCount == 0 || idx >= childCount)
    return nullptr;
  const auto pos = firstChild + idx;
  if (pos == detail::InvalidNodeIndex)
    return nullptr;
  auto* nodes = detail::InspectAccess<TAdapter>::nodes(_ctx);
  if (!nodes || pos >= nodes->size())
    return nullptr;
  return std::addressof((*nodes)[pos]);
}

template<typename TAdapter>
inline const FieldNode<TAdapter>*
FieldNode<TAdapter>::findChildById(uint16_t id) const
{
  for (size_t i = 0; i < childCount; ++i) {
    auto* c = child(i);
    if (c && c->fieldId == id)
      return c;
  }
  return nullptr;
}

template<typename TAdapter>
inline bool
FieldNode<TAdapter>::canViewBytes() const
{
  return viewable && reason == FieldReason::None && payloadSize != 0u && _ctx &&
         _ctx->payload;
}

template<typename TAdapter>
inline std::pair<const uint8_t*, size_t>
FieldNode<TAdapter>::bytes() const
{
  if (!canViewBytes())
    return { nullptr, 0u };
  if (payloadOffset > _ctx->payloadSize)
    return { nullptr, 0u };
  const auto remaining = _ctx->payloadSize - payloadOffset;
  if (payloadSize > remaining)
    return { nullptr, 0u };
  return { _ctx->payload + payloadOffset, payloadSize };
}

template<typename TAdapter>
template<typename U>
inline void
FieldNode<TAdapter>::constructInto(U& value) const
{
  detail::ConstructInvoker<TAdapter>::construct(*this, value);
}

template<typename TAdapter>
template<typename U>
inline U
FieldNode<TAdapter>::construct() const
{
  U value{};
  constructInto(value);
  return value;
}

template<typename TRoot, typename TAdapter>
struct InspectResult
{
  using Adapter = typename std::decay<TAdapter>::type;

  InspectStatus status{ InspectStatus::NoTrailer };
  InspectConfig config{};
  std::vector<FieldNode<Adapter>> nodes{};
  const FieldNode<Adapter>* root{ nullptr };

  InspectResult() = default;

  InspectResult(const InspectResult& other)
    : status{ other.status }
    , config{ other.config }
    , nodes{ other.nodes }
    , root{ nullptr }
  {
    bindContext(other._ctx);
  }

  InspectResult(InspectResult&& other) noexcept
    : status{ other.status }
    , config{ other.config }
    , nodes{ std::move(other.nodes) }
    , root{ nullptr }
  {
    bindContext(other._ctx);
    other.root = nullptr;
    other.status = InspectStatus::NoTrailer;
    other.nodes.clear();
    other._ctx = detail::InspectContext<Adapter>{};
  }

  InspectResult& operator=(const InspectResult& other)
  {
    if (this != std::addressof(other)) {
      status = other.status;
      config = other.config;
      nodes = other.nodes;
      bindContext(other._ctx);
    }
    return *this;
  }

  InspectResult& operator=(InspectResult&& other) noexcept
  {
    if (this != std::addressof(other)) {
      status = other.status;
      config = other.config;
      nodes = std::move(other.nodes);
      bindContext(other._ctx);
      other.root = nullptr;
      other.status = InspectStatus::NoTrailer;
      other.nodes.clear();
      other._ctx = detail::InspectContext<Adapter>{};
    }
    return *this;
  }

  bool ok() const { return status == InspectStatus::Ok && root != nullptr; }

  static InspectResult error(InspectStatus s,
                             InspectConfig cfg,
                             const uint8_t* data,
                             size_t totalSize)
  {
    return InspectResult{
      s, std::move(cfg), {}, data, totalSize, nullptr, 0u };
  }

private:
  detail::InspectContext<Adapter> _ctx{};

  InspectResult(InspectStatus s,
                InspectConfig cfg,
                std::vector<FieldNode<Adapter>> ns,
                const uint8_t* data,
                size_t totalSize,
                const uint8_t* payload,
                size_t payloadSize)
    : status{ s }
    , config{ std::move(cfg) }
    , nodes{ std::move(ns) }
    , root{ nullptr }
  {
    detail::InspectAccess<Adapter>::bind(
      nodes, _ctx, data, totalSize, payload, payloadSize);
    root = nodes.empty() ? nullptr : nodes.data();
  }

  void bindContext(const detail::InspectContext<Adapter>& otherCtx)
  {
    detail::InspectAccess<Adapter>::bind(
      nodes, _ctx, otherCtx.data, otherCtx.totalSize, otherCtx.payload, otherCtx.payloadSize);
    root = nodes.empty() ? nullptr : nodes.data();
  }

  template<typename T, typename A>
  friend InspectResult<T, typename std::decay<A>::type> inspectOffsetTable(
    const uint8_t*,
    size_t,
    A&&,
    const InspectConfig&);
};

namespace detail {

inline InspectStatus
mapVerify(ot::VerifyResult res)
{
  switch (res) {
    case ot::VerifyResult::Ok:
      return InspectStatus::Ok;
    case ot::VerifyResult::BadMagic:
      return InspectStatus::BadMagic;
    case ot::VerifyResult::BadVersion:
      return InspectStatus::BadVersion;
    case ot::VerifyResult::OutOfBounds:
      return InspectStatus::OutOfBounds;
    case ot::VerifyResult::WrongEndianness:
      return InspectStatus::WrongEndianness;
    case ot::VerifyResult::MissingRoot:
      return InspectStatus::MissingRoot;
    case ot::VerifyResult::TooDeep:
      return InspectStatus::TooDeep;
    case ot::VerifyResult::TooManyFields:
      return InspectStatus::TooManyFields;
    case ot::VerifyResult::NoTrailer:
    default:
      return InspectStatus::NoTrailer;
  }
}

template<typename Adapter>
inline void
setReason(FieldNode<Adapter>& node, FieldReason reason)
{
  if (node.reason == FieldReason::None)
    node.reason = reason;
  node.viewable = false;
}

template<typename Adapter>
inline bool
checkAlignment(const details::Entry& e,
               const ot::ViewCtx& ctx,
               const details::FieldInfo& expected)
{
  const auto align = expected.align ? expected.align : 1u;
  if (align <= 1u)
    return true;
  return ot::isAligned(ctx.payload, e, align);
}

template<typename Adapter>
inline void
populateNodeFromEntry(FieldNode<Adapter>& node,
                      const details::Entry& e,
                      const ot::ViewCtx& ctx,
                      const details::FieldInfo* expected,
                      size_t payloadBaseOffset,
                      bool skipOffsetCheck)
{
  node.fieldId = e.fieldId;
  node.kind = e.kind;
  node.flags = e.flags;
  node.elemSize = e.elemSize;
  node.payloadOffset = static_cast<size_t>(e.payloadOff);
  node.payloadSize = static_cast<size_t>(e.size);
  node.viewable = true;
  node.reason = FieldReason::None;
  node.registryEntry = expected;
  if (node.payloadOffset < payloadBaseOffset) {
    setReason(node, FieldReason::OutOfBounds);
    return;
  }
  const auto relativePayloadOff = node.payloadOffset - payloadBaseOffset;

  if (!ot::entryInBounds(e, ctx.payloadSize)) {
    setReason(node, FieldReason::OutOfBounds);
    return;
  }

  if (details::hasFlag(e.flags, details::FieldFlags::CopyOnly) ||
      (expected &&
       details::hasFlag(expected->flags, details::FieldFlags::CopyOnly))) {
    setReason(node, FieldReason::CopyOnly);
    return;
  }

  if (!expected) {
    setReason(node, FieldReason::RegistryMismatch);
    return;
  }

  if (!skipOffsetCheck && relativePayloadOff != expected->offset) {
    setReason(node, FieldReason::RegistryMismatch);
    return;
  }

  if (e.kind == details::FieldKind::Span) {
    setReason(node, FieldReason::Unsupported);
    return;
  }

  if (e.fieldId != expected->id || e.kind != expected->kind) {
    setReason(node, FieldReason::RegistryMismatch);
    return;
  }

  const auto optionalFlag = details::FieldFlags::Optional;
  const bool optionalMatch =
    details::hasFlag(e.flags, optionalFlag) ==
    details::hasFlag(expected->flags, optionalFlag);
  if (!optionalMatch) {
    setReason(node, FieldReason::RegistryMismatch);
    return;
  }

  const bool alignedFlagMatch =
    details::hasFlag(e.flags, details::FieldFlags::Aligned) ==
    details::hasFlag(expected->flags, details::FieldFlags::Aligned);
  if (!alignedFlagMatch) {
    setReason(node, FieldReason::RegistryMismatch);
    return;
  }

  if (expected->size != 0u && node.payloadSize != expected->size) {
    setReason(node, FieldReason::SizeMismatch);
    return;
  }

  const bool needsAlign = details::hasFlag(e.flags, details::FieldFlags::Aligned) ||
                          details::hasFlag(expected->flags, details::FieldFlags::Aligned) ||
                          (expected->align > 1u);
  if (needsAlign && !checkAlignment<Adapter>(e, ctx, *expected)) {
    setReason(node, FieldReason::Misaligned);
    return;
  }

  if (node.kind == details::FieldKind::NestedTable) {
    node.viewable = false;
  }
}

template<typename Adapter>
inline size_t
buildTableNodes(const details::VerifiedOffsetTables& verified,
                const ot::TableView& table,
                const details::FieldInfo* registry,
                size_t registryCount,
                uint16_t registryTypeVersion,
                size_t payloadBaseOffset,
                bool skipOffsetCheck,
                std::vector<FieldNode<Adapter>>& nodes,
                size_t maxNodes,
                InspectStatus& status)
{
  if (status != InspectStatus::Ok)
    return InvalidNodeIndex;

  const auto needed = nodes.size() + static_cast<size_t>(table.hdr.fieldCount);
  if (maxNodes != 0 && needed > maxNodes) {
    status = InspectStatus::TooManyFields;
    return InvalidNodeIndex;
  }

  const auto startIdx = nodes.size();
  nodes.resize(startIdx + table.hdr.fieldCount);

  const bool typeMismatch =
    registryTypeVersion != 0 && table.hdr.typeVersion != registryTypeVersion;

  for (uint16_t i = 0; i < table.hdr.fieldCount; ++i) {
    auto& node = nodes[startIdx + i];
    const details::FieldInfo* expected =
      (registry && i < registryCount) ? std::addressof(registry[i]) : nullptr;
    populateNodeFromEntry(
      node,
      table.entries[i],
      verified.ctx,
      expected,
      payloadBaseOffset,
      skipOffsetCheck);
    node.typeVersion = table.hdr.typeVersion;
    if (typeMismatch) {
      setReason(node, FieldReason::RegistryMismatch);
    }
    if (expected) {
      node.nestedFieldCount = expected->nestedFieldCount;
    }

    const bool sizeDiffers =
      expected && expected->size != 0u && node.payloadSize != expected->size;
    if (sizeDiffers)
      skipOffsetCheck = true;
  }

  for (uint16_t i = 0; i < table.hdr.fieldCount; ++i) {
    auto& node = nodes[startIdx + i];
    if (node.kind != details::FieldKind::NestedTable)
      continue;

    const auto& e = table.entries[i];
    if (e.elemSize <= verified.ctx.payloadSize) {
      setReason(node, FieldReason::OutOfBounds);
      continue;
    }

    const auto nestedOffset =
      static_cast<size_t>(e.elemSize - verified.ctx.payloadSize);
    auto* nested = details::findTable(verified, nestedOffset);
    if (!nested) {
      setReason(node, FieldReason::OutOfBounds);
      continue;
    }

    const details::FieldInfo* childRegistry = nullptr;
    size_t childCount = 0;
    uint16_t childTypeVer = 0;
    if (node.registryEntry) {
      childRegistry = node.registryEntry->nestedEntries;
      childCount = node.registryEntry->nestedFieldCount;
      childTypeVer = node.registryEntry->nestedTypeVersion;
      if (!childRegistry) {
        setReason(node, FieldReason::RegistryMismatch);
      }
      if (node.registryEntry->nestedFieldCount != nested->view.hdr.fieldCount) {
        setReason(node, FieldReason::RegistryMismatch);
      }
      if (childTypeVer != 0 &&
          nested->view.hdr.typeVersion != childTypeVer) {
        setReason(node, FieldReason::RegistryMismatch);
      }
    } else {
      setReason(node, FieldReason::RegistryMismatch);
    }

    bool hasViewableChild = false;
    if (childRegistry) {
      for (size_t idx = 0; idx < childCount; ++idx) {
        if (!details::hasFlag(
              childRegistry[idx].flags, details::FieldFlags::CopyOnly)) {
          hasViewableChild = true;
          break;
        }
      }
    }
    if (node.reason == FieldReason::None && childRegistry && !hasViewableChild) {
      setReason(node, FieldReason::CopyOnly);
      continue;
    }

    const auto childStart = buildTableNodes<Adapter>(
      verified,
      nested->view,
      childRegistry,
      childCount,
      childTypeVer,
      node.payloadOffset,
      false,
      nodes,
      maxNodes,
      status);
    if (status != InspectStatus::Ok)
      continue;
    node.firstChild = childStart;
    node.childCount = nested->view.hdr.fieldCount;
    node.nestedFieldCount = nested->view.hdr.fieldCount;
    node.typeVersion = nested->view.hdr.typeVersion;
  }

  return startIdx;
}

} // namespace detail

template<typename TRoot, typename TAdapter>
inline InspectResult<TRoot, typename std::decay<TAdapter>::type>
inspectOffsetTable(const uint8_t* data,
                   size_t size,
                   TAdapter&& adapter,
                   const InspectConfig& cfg = {})
{
  using Adapter = typename std::decay<TAdapter>::type;
  (void)adapter;

  auto verified = details::verifyOffsetTables(data, size, cfg.verifyCfg);
  auto status = detail::mapVerify(verified.status);
  if (status != InspectStatus::Ok) {
    return InspectResult<TRoot, Adapter>::error(status, cfg, data, size);
  }

  auto* rootTable = details::rootTable(verified);
  if (!rootTable) {
    return InspectResult<TRoot, Adapter>::error(
      InspectStatus::MissingRoot, cfg, data, size);
  }

  constexpr bool hasRegistry = details::FieldRegistry<TRoot>::Enabled;
  const auto* registryRoot = hasRegistry ? details::FieldRegistry<TRoot>::entries() : nullptr;
  const size_t registryCount =
    hasRegistry ? details::FieldRegistry<TRoot>::FieldCount : 0u;
  const uint16_t registryTypeVer =
    hasRegistry ? details::FieldRegistry<TRoot>::TypeVersion : 0u;

  size_t totalFields = 0u;
  for (const auto& rec : verified.tables) {
    totalFields += rec.view.hdr.fieldCount;
  }

  if (cfg.maxFieldNodes != 0 && (1u + totalFields) > cfg.maxFieldNodes) {
    return InspectResult<TRoot, Adapter>::error(
      InspectStatus::TooManyFields, cfg, data, size);
  }

  std::vector<FieldNode<Adapter>> nodes;
  nodes.reserve(1u + totalFields);

  FieldNode<Adapter> root{};
  root.fieldId = 0;
  root.kind = details::FieldKind::NestedTable;
  root.viewable = false;
  root.reason = FieldReason::None;
  root.typeVersion = rootTable->hdr.typeVersion;
  root.childCount = rootTable->hdr.fieldCount;
  root.nestedFieldCount = static_cast<uint16_t>(registryCount);
  if (!hasRegistry) {
    root.reason = FieldReason::RegistryMismatch;
  }
  if ((registryTypeVer != 0 && registryTypeVer != root.typeVersion) ||
      root.childCount != registryCount) {
    root.reason = FieldReason::RegistryMismatch;
  }
  nodes.push_back(root);

  auto buildStatus = InspectStatus::Ok;
  if (root.childCount != 0) {
    const auto childStart = detail::buildTableNodes<Adapter>(
      verified,
      *rootTable,
      registryRoot,
      registryCount,
      registryTypeVer,
      0u,
      false,
      nodes,
      cfg.maxFieldNodes,
      buildStatus);
    if (buildStatus != InspectStatus::Ok) {
      return InspectResult<TRoot, Adapter>::error(
        buildStatus, cfg, data, size);
    }
    nodes[0].firstChild = childStart;
  }

  return InspectResult<TRoot, Adapter>{ InspectStatus::Ok,
                                        cfg,
                                        std::move(nodes),
                                        data,
                                        size,
                                        verified.ctx.payload,
                                        verified.ctx.payloadSize };
}

template<typename TRoot, typename TAdapter, typename Visitor>
inline InspectStatus
inspectOffsetTable(const uint8_t* data,
                   size_t size,
                   TAdapter&& adapter,
                   Visitor&& visitor,
                   const InspectConfig& cfg = {})
{
  using Adapter = typename std::decay<TAdapter>::type;
  auto result = inspectOffsetTable<TRoot>(
    data, size, std::forward<TAdapter>(adapter), cfg);
  if (!result.root)
    return result.status;

  struct Frame
  {
    size_t idx{};
    std::vector<uint16_t> path{};
  };

  auto childPaths = [&](const Frame& parent, const FieldNode<Adapter>& node) {
    std::vector<std::vector<uint16_t>> out;
    out.reserve(node.childCount);
    for (size_t i = 0; i < node.childCount; ++i) {
      std::vector<uint16_t> p = parent.path;
      p.push_back(result.nodes[node.firstChild + i].fieldId);
      out.push_back(std::move(p));
    }
    return out;
  };

  auto callVisitor =
    [&](const FieldNode<Adapter>& node, const std::vector<uint16_t>& path) {
      if constexpr (std::is_invocable_v<Visitor,
                                        const FieldNode<Adapter>&,
                                        const std::vector<uint16_t>&>) {
        visitor(node, path);
      } else {
        visitor(node);
      }
    };

  const auto* base = result.nodes.data();
  const auto count = result.nodes.size();
  std::vector<Frame> stack;
  stack.reserve(count);
  stack.push_back(Frame{ static_cast<size_t>(result.root - base), {} });

  while (!stack.empty()) {
    auto frame = std::move(stack.back());
    stack.pop_back();
    const auto idx = frame.idx;
    if (idx >= count)
      continue;
    const auto& node = result.nodes[idx];
    if (!frame.path.empty())
      callVisitor(node, frame.path);
    if (node.childCount == 0 || node.firstChild == detail::InvalidNodeIndex)
      continue;
    const auto begin = node.firstChild;
    for (size_t i = 0; i < node.childCount; ++i) {
      const auto childIdx = begin + (node.childCount - 1u - i);
      std::vector<uint16_t> childPath = frame.path;
      childPath.push_back(result.nodes[childIdx].fieldId);
      stack.push_back(Frame{ childIdx, std::move(childPath) });
    }
  }

  return result.status;
}

}

}

#endif // BITSERY_OFFSET_TABLE_INSPECT_H
