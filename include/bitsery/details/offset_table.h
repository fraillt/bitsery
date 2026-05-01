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

#ifndef BITSERY_DETAILS_OFFSET_TABLE_H
#define BITSERY_DETAILS_OFFSET_TABLE_H

#include <algorithm>
#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <deque>
#include <iterator>
#include <limits>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>
#include "adapter_common.h"

#if defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202506L
#include "serialization_common.h"
#include <meta>
#define BITSERY_HAS_CPP26_REFLECTION 1
#else
#define BITSERY_HAS_CPP26_REFLECTION 0
#endif

namespace bitsery {

namespace details {

constexpr char TRAILER_MAGIC[8]{ 'B', 'T', 'S', 'Y', 'O', 'T', '0', '1' };
constexpr uint8_t TRAILER_VERSION = 1;

enum class TrailerFlags : uint8_t
{
  None = 0,
  OffsetsValid = 1 << 0,
  HasNestedTables = 1 << 1,
  CrossEndianDisallowed = 1 << 2
};

constexpr TrailerFlags
operator|(TrailerFlags lhs, TrailerFlags rhs)
{
  return static_cast<TrailerFlags>(static_cast<uint8_t>(lhs) |
                                   static_cast<uint8_t>(rhs));
}

inline TrailerFlags&
operator|=(TrailerFlags& lhs, TrailerFlags rhs)
{
  lhs = lhs | rhs;
  return lhs;
}

constexpr bool
hasFlag(TrailerFlags value, TrailerFlags flag)
{
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0u;
}

#pragma pack(push, 1)
struct Trailer
{
  std::array<char, 8> magic;
  uint8_t version;
  uint8_t flags;
  uint16_t reserved;
  uint32_t rootTableOff;
};
#pragma pack(pop)

static_assert(sizeof(Trailer) == 16, "Invalid Trailer size");

enum class FieldKind : uint8_t
{
  Scalar,
  Array,
  Span,
  NestedStruct,
  NestedTable
};

enum class FieldFlags : uint8_t
{
  None = 0,
  Optional = 1 << 0,
  CopyOnly = 1 << 1,
  Aligned = 1 << 2
};

constexpr FieldFlags
operator|(FieldFlags lhs, FieldFlags rhs)
{
  return static_cast<FieldFlags>(static_cast<uint8_t>(lhs) |
                                 static_cast<uint8_t>(rhs));
}

constexpr FieldFlags
operator&(FieldFlags lhs, FieldFlags rhs)
{
  return static_cast<FieldFlags>(static_cast<uint8_t>(lhs) &
                                 static_cast<uint8_t>(rhs));
}

constexpr FieldFlags&
operator|=(FieldFlags& lhs, FieldFlags rhs)
{
  lhs = lhs | rhs;
  return lhs;
}

constexpr bool
hasFlag(FieldFlags value, FieldFlags flag)
{
  return (static_cast<uint8_t>(value) & static_cast<uint8_t>(flag)) != 0u;
}

#pragma pack(push, 1)
struct TableHdr
{
  uint16_t fieldCount;
  uint16_t typeVersion;
};
#pragma pack(pop)

static_assert(sizeof(TableHdr) == 4, "Invalid TableHdr size");

#pragma pack(push, 1)
struct Entry
{
  uint16_t fieldId;
  FieldKind kind;
  FieldFlags flags;
  uint32_t payloadOff;
  uint32_t size;
  uint32_t elemSize;
};
#pragma pack(pop)

static_assert(sizeof(Entry) == 16, "Invalid Entry size");

constexpr size_t InvalidTableIndex = std::numeric_limits<size_t>::max();
constexpr uint32_t InvalidRecordedTableIndex =
  std::numeric_limits<uint32_t>::max();

struct RecordedEntry
{
  uint32_t payloadOff{};
  uint32_t size{};
  uint32_t elemSize{};
  uint32_t nestedTableIdx{ InvalidRecordedTableIndex };
  uint16_t fieldId{};
  FieldKind kind{ FieldKind::Scalar };
  FieldFlags flags{ FieldFlags::None };
};

static_assert(sizeof(RecordedEntry) == 20, "Unexpected RecordedEntry size");

struct GeneratedRuntimeEntry
{
  uint32_t payloadOff{};
  uint32_t size{};
};

static_assert(sizeof(GeneratedRuntimeEntry) == 8,
              "Unexpected GeneratedRuntimeEntry size");

inline bool
sameRecordedEntry(const RecordedEntry& lhs, const RecordedEntry& rhs)
{
  return lhs.payloadOff == rhs.payloadOff && lhs.size == rhs.size &&
         lhs.elemSize == rhs.elemSize &&
         lhs.nestedTableIdx == rhs.nestedTableIdx &&
         lhs.fieldId == rhs.fieldId && lhs.kind == rhs.kind &&
         lhs.flags == rhs.flags;
}

struct RecordedTable
{
  uint16_t typeVersion{ 0 };
  std::vector<RecordedEntry> entries;

  RecordedTable()
    : typeVersion{ 0 }
    , entries{}
  {}
  RecordedTable(const RecordedTable&) = default;
  RecordedTable& operator=(const RecordedTable&) = default;
  RecordedTable(RecordedTable&&) = default;
  RecordedTable& operator=(RecordedTable&&) = default;
};

inline bool
sameRecordedTable(const RecordedTable& lhs, const RecordedTable& rhs)
{
  if (lhs.typeVersion != rhs.typeVersion ||
      lhs.entries.size() != rhs.entries.size()) {
    return false;
  }
  for (size_t i = 0; i < lhs.entries.size(); ++i) {
    if (!sameRecordedEntry(lhs.entries[i], rhs.entries[i]))
      return false;
  }
  return true;
}

inline bool
sameRecordedTables(const std::vector<RecordedTable>& lhs,
                   size_t lhsCount,
                   const std::vector<RecordedTable>& rhs,
                   size_t rhsCount)
{
  if (lhsCount != rhsCount)
    return false;
  for (size_t i = 0; i < lhsCount; ++i) {
    if (!sameRecordedTable(lhs[i], rhs[i]))
      return false;
  }
  return true;
}

struct FieldInfo
{
  uint16_t id;
  FieldKind kind;
  FieldFlags flags;
  uint32_t elemSize;
  size_t offset;
  size_t size;
  size_t align;
  uint16_t nestedFieldCount;
  uint16_t nestedTypeVersion;
  const FieldInfo* nestedEntries;
};

inline constexpr FieldInfo
makeField(uint16_t id,
          FieldKind kind,
          FieldFlags flags = FieldFlags::None,
          size_t offset = 0u,
          size_t size = 0u,
          size_t elemSize = 0u,
          size_t align = 1u,
          uint16_t nestedFieldCount = 0u,
          uint16_t nestedTypeVersion = 0u,
          const FieldInfo* nestedEntries = nullptr)
{
  return FieldInfo{ id,
                    kind,
                    flags,
                    static_cast<uint32_t>(elemSize),
                    offset,
                    size,
                    align,
                    nestedFieldCount,
                    nestedTypeVersion,
                    nestedEntries };
}

#if BITSERY_HAS_CPP26_REFLECTION
template<typename T, bool IsContainer = IsContainerTraitsDefined<T>::value>
struct DefaultFieldElementSize
  : std::integral_constant<size_t,
                           (std::is_fundamental<T>::value ||
                            std::is_enum<T>::value)
                             ? sizeof(T)
                             : 0u>
{};

template<typename T>
struct DefaultFieldElementSize<T, true>
  : std::integral_constant<
      size_t,
      sizeof(typename traits::ContainerTraits<T>::TValue)>
{};

template<typename T>
constexpr size_t
defaultFieldElementSize()
{
  return DefaultFieldElementSize<T>::value;
}
#endif

template<typename T>
constexpr FieldFlags
defaultFieldFlags()
{
  FieldFlags flags = FieldFlags::None;
  const auto copyOnly = !std::is_trivially_copyable<T>::value ||
                        std::is_pointer<T>::value ||
                        std::is_member_pointer<T>::value;
  if (copyOnly)
    flags |= FieldFlags::CopyOnly;
  if (!copyOnly && alignof(T) > 1)
    flags |= FieldFlags::Aligned;
  return flags;
}

template<typename T>
struct FieldRegistry;

template<typename T>
struct EnableReflectedFieldRegistry
#if BITSERY_HAS_CPP26_REFLECTION
  : std::integral_constant<bool,
                           std::is_aggregate<T>::value &&
                             std::is_standard_layout<T>::value &&
                             !std::is_fundamental<T>::value &&
                             !std::is_enum<T>::value &&
                             !IsContainerTraitsDefined<T>::value &&
                             !IsTextTraitsDefined<T>::value>
#else
  : std::false_type
#endif
{};

template<typename T,
         bool ReflectionEnabled = EnableReflectedFieldRegistry<T>::value>
struct ReflectedFieldRegistrySelector
{
  static constexpr bool Enabled = false;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 0;
  static constexpr const FieldInfo* entries() { return nullptr; }
};

template<typename T>
struct ReflectedFieldRegistry : ReflectedFieldRegistrySelector<T>
{};

#if BITSERY_HAS_CPP26_REFLECTION

template<typename T, std::meta::info Member>
struct ReflectedFieldOverride
{
  static constexpr uint16_t Id = 0;
  static constexpr FieldFlags Flags = FieldFlags::None;
};

template<typename T>
struct ReflectedTypeVersion
{
  static constexpr uint16_t Value = 0;
};

template<typename T>
consteval bool
isReflectedRegistryEnabled()
{
  return EnableReflectedFieldRegistry<T>::value;
}

template<typename T>
consteval FieldKind
reflectedDefaultFieldKind()
{
  if constexpr (IsContainerTraitsDefined<T>::value) {
    return FieldKind::Array;
  } else if constexpr (std::is_fundamental<T>::value ||
                       std::is_enum<T>::value) {
    return FieldKind::Scalar;
  } else if constexpr (isReflectedRegistryEnabled<T>()) {
    return FieldKind::NestedTable;
  } else {
    return FieldKind::NestedStruct;
  }
}

template<typename T, std::meta::info Member>
consteval uint16_t
reflectedFieldId(size_t index)
{
  constexpr auto overrideId = ReflectedFieldOverride<T, Member>::Id;
  if constexpr (overrideId != 0u) {
    return overrideId;
  } else {
    if (index >= std::numeric_limits<uint16_t>::max())
      return std::numeric_limits<uint16_t>::max();
    return static_cast<uint16_t>(index + 1u);
  }
}

template<typename T, std::meta::info Member>
consteval FieldInfo
reflectedFieldInfo(size_t index)
{
  using MemberT = typename[:std::meta::type_of(Member):];
  constexpr auto kind = reflectedDefaultFieldKind<MemberT>();
  auto flags =
    defaultFieldFlags<MemberT>() | ReflectedFieldOverride<T, Member>::Flags;
  constexpr auto elemSize = defaultFieldElementSize<MemberT>();
  uint16_t nestedCount = 0;
  uint16_t nestedVersion = 0;
  const FieldInfo* nestedEntries = nullptr;
  if constexpr (kind == FieldKind::NestedTable) {
    if constexpr (FieldRegistry<MemberT>::Enabled) {
      nestedEntries = FieldRegistry<MemberT>::entries();
      nestedCount = static_cast<uint16_t>(FieldRegistry<MemberT>::FieldCount);
      nestedVersion = FieldRegistry<MemberT>::TypeVersion;
    } else {
      flags |= FieldFlags::CopyOnly;
    }
  }
  constexpr auto offset = std::meta::offset_of(Member);
  static_assert(offset.bits == 0,
                "Bit-field reflection is not supported by zero-copy views.");
  return makeField(reflectedFieldId<T, Member>(index),
                   kind,
                   flags,
                   static_cast<size_t>(offset.bytes),
                   std::meta::size_of(Member),
                   elemSize,
                   std::meta::alignment_of(Member),
                   nestedCount,
                   nestedVersion,
                   nestedEntries);
}

template<typename T>
consteval auto
reflectedFields()
{
  constexpr auto ctx = std::meta::access_context::unchecked();
  static constexpr auto members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^T, ctx));
  std::array<FieldInfo, members.size()> fields{};
  size_t index = 0;
  template for (constexpr auto member : members)
  {
    fields[index] = reflectedFieldInfo<T, member>(index);
    ++index;
  }
  return fields;
}

template<typename T>
consteval bool
reflectedPayloadCanCopyObject()
{
  if constexpr (!std::is_trivially_copyable<T>::value) {
    return false;
  } else {
    constexpr auto fields = reflectedFields<T>();
    size_t expectedOffset = 0;
    for (const auto& field : fields) {
      if (field.offset != expectedOffset)
        return false;
      if (field.kind != FieldKind::Scalar && field.kind != FieldKind::Array)
        return false;
      if (hasFlag(field.flags, FieldFlags::CopyOnly))
        return false;
      expectedOffset += field.size;
    }
    return expectedOffset == sizeof(T);
  }
}

template<typename T>
struct ReflectedPayloadTraits
{
  static constexpr bool ContiguousObject = reflectedPayloadCanCopyObject<T>();
};

template<typename TAdapter, typename T>
inline void
writeReflectedPayload(TAdapter& adapter, const T& obj);

template<typename TAdapter, typename T>
inline void
writeReflectedPayloadRange(TAdapter& adapter, const T& obj, size_t size)
{
  auto first = std::begin(obj);
  using ValueT = typename std::decay<decltype(*first)>::type;
  constexpr auto valueSize = sizeof(ValueT);
  if constexpr (traits::ContainerTraits<T>::isContiguous &&
                IsFundamentalType<ValueT>::value) {
    if (size > 0u) {
      using IntegralT = typename IntegralFromFundamental<ValueT>::TValue;
      adapter.template writeBuffer<valueSize>(
        reinterpret_cast<const IntegralT*>(&(*first)),
        size);
    }
  } else {
    using diff_t =
      typename std::iterator_traits<decltype(first)>::difference_type;
    auto last = std::next(first, static_cast<diff_t>(size));
    for (; first != last; ++first) {
      writeReflectedPayload(adapter, *first);
    }
  }
}

template<typename TAdapter, typename T>
inline void
writeReflectedPayloadField(TAdapter& adapter, const T& field)
{
  using RawT =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  if constexpr (IsTextTraitsDefined<RawT>::value) {
    const size_t length = traits::TextTraits<RawT>::length(field);
    writeSize(adapter, length);
    writeReflectedPayloadRange(adapter, field, length);
  } else if constexpr (IsContainerTraitsDefined<RawT>::value) {
    if constexpr (traits::ContainerTraits<RawT>::isResizable) {
      const auto size = traits::ContainerTraits<RawT>::size(field);
      writeSize(adapter, size);
      writeReflectedPayloadRange(adapter, field, size);
    } else {
      writeReflectedPayloadRange(
        adapter, field, traits::ContainerTraits<RawT>::size(field));
    }
  } else if constexpr (IsFundamentalType<RawT>::value) {
    using ValueT = typename IntegralFromFundamental<RawT>::TValue;
    adapter.template writeBytes<sizeof(RawT)>(
      reinterpret_cast<const ValueT&>(field));
  } else {
    writeReflectedPayload(adapter, field);
  }
}

template<typename TAdapter, typename T>
inline void
writeReflectedPayload(TAdapter& adapter, const T& obj)
{
  using RawT =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  if constexpr (IsFundamentalType<RawT>::value ||
                IsContainerTraitsDefined<RawT>::value ||
                IsTextTraitsDefined<RawT>::value) {
    writeReflectedPayloadField(adapter, obj);
  } else if constexpr (ReflectedPayloadTraits<RawT>::ContiguousObject &&
                       TAdapter::TConfig::Endianness == getSystemEndianness()) {
    adapter.template writeBuffer<1>(
      reinterpret_cast<const uint8_t*>(std::addressof(obj)), sizeof(RawT));
  } else {
    constexpr auto ctx = std::meta::access_context::unchecked();
    static constexpr auto members = std::define_static_array(
      std::meta::nonstatic_data_members_of(^^RawT, ctx));
    template for (constexpr auto member : members) {
      writeReflectedPayloadField(adapter, obj.[:member:]);
    }
  }
}

template<typename T>
struct ReflectedFieldRegistrySelector<T, true>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = ReflectedTypeVersion<T>::Value;
  static constexpr auto Fields = reflectedFields<T>();
  static constexpr size_t FieldCount = Fields.size();
  static constexpr const FieldInfo* entries() { return Fields.data(); }
};

#endif

template<typename T>
struct FieldRegistry : ReflectedFieldRegistry<T>
{};

// Forward declarations for stream adapters to specialize HasWrittenBytesCount.
} // namespace details
} // namespace bitsery

namespace bitsery {
template<typename TChar, typename Config, typename CharTraits>
class BasicOutputStreamAdapter;
template<typename TChar, typename Config, typename CharTraits, typename TBuffer>
class BasicBufferedOutputStreamAdapter;
}

namespace bitsery { namespace details {

template<typename Adapter, typename = void>
struct HasWrittenBytesCount : std::false_type
{};

template<typename Adapter, typename = void>
struct HasCurrentWritePos : std::false_type
{};

template<typename Adapter>
struct HasWrittenBytesCount<
  Adapter,
  std::void_t<decltype(std::declval<const Adapter&>().writtenBytesCount())>>
  : std::true_type
{};

template<typename C, typename Conf, typename Traits>
struct HasWrittenBytesCount<
  ::bitsery::BasicOutputStreamAdapter<C, Conf, Traits>,
  void> : std::false_type
{};

template<typename C, typename Conf, typename Traits, typename Buf>
struct HasWrittenBytesCount<
  ::bitsery::BasicBufferedOutputStreamAdapter<C, Conf, Traits, Buf>,
  void> : std::false_type
{};

template<typename Adapter>
struct HasCurrentWritePos<
  Adapter,
  std::void_t<decltype(std::declval<const Adapter&>().currentWritePos())>>
  : std::true_type
{};

template<typename C, typename Conf, typename Traits>
struct HasCurrentWritePos<
  ::bitsery::BasicOutputStreamAdapter<C, Conf, Traits>,
  void> : std::false_type
{};

template<typename C, typename Conf, typename Traits, typename Buf>
struct HasCurrentWritePos<
  ::bitsery::BasicBufferedOutputStreamAdapter<C, Conf, Traits, Buf>,
  void> : std::false_type
{};

class OffsetTableRecorder
{
public:
  using TableIndex = size_t;
  static constexpr uint64_t SignatureSeed = 1469598103934665603ull;

  OffsetTableRecorder();

  void pushTable(uint16_t typeVersion = 0, size_t expectedFieldCount = 0);
  TableIndex popTable();

  RecordedTable& currentTable();
  const RecordedTable& table(TableIndex idx) const;
  const RecordedTable* rootTable() const;
  TableIndex rootTableIndex() const { return _rootIndex; }
  void clear();

  void recordField(uint16_t fieldId,
                   FieldKind kind,
                   FieldFlags flags,
                   size_t payloadBegin,
                   size_t payloadEnd,
                   uint32_t elemSize,
                   TableIndex nestedTableIdx = InvalidTableIndex);

  const std::vector<RecordedTable>& completedTables() const
  {
    return _completed;
  }

  size_t signature() const { return static_cast<size_t>(_signature); }
  bool hasNestedTables() const { return _hasNestedTables; }
  bool hasOpenTable() const { return !_stack.empty(); }

private:
  void mixSignature(uint64_t v)
  {
    _signature ^= v + 0x9e3779b97f4a7c15ull + (_signature << 6) +
                  (_signature >> 2);
  }

  std::vector<RecordedTable> _stack;
  std::vector<RecordedTable> _completed;
  TableIndex _rootIndex{ InvalidTableIndex };
  bool _hasNestedTables{ false };
  uint64_t _signature{ SignatureSeed };
};

struct StaticCacheEntry;

inline OffsetTableRecorder::OffsetTableRecorder()
  : _stack{}
  , _completed{}
  , _rootIndex{ InvalidTableIndex }
  , _hasNestedTables{ false }
  , _signature{ SignatureSeed }
{}

inline void
OffsetTableRecorder::pushTable(uint16_t typeVersion, size_t expectedFieldCount)
{
  mixSignature(0x6f2c3e6d5a7b9184ull);
  mixSignature(static_cast<uint64_t>(typeVersion));
  _stack.emplace_back();
  _stack.back().typeVersion = typeVersion;
  if (expectedFieldCount != 0u)
    _stack.back().entries.reserve(expectedFieldCount);
  if (_stack.size() > 1u)
    _hasNestedTables = true;
}

inline OffsetTableRecorder::TableIndex
OffsetTableRecorder::popTable()
{
  assert(!_stack.empty());
  mixSignature(0x9b9d4f4f5d7aee31ull);
  mixSignature(static_cast<uint64_t>(_stack.back().entries.size()));
  const auto idx = _completed.size();
  _completed.emplace_back(std::move(_stack.back()));
  _stack.pop_back();
  if (_stack.empty())
    _rootIndex = idx;
  return idx;
}

inline RecordedTable&
OffsetTableRecorder::currentTable()
{
  assert(!_stack.empty());
  return _stack.back();
}

inline const RecordedTable&
OffsetTableRecorder::table(TableIndex idx) const
{
  assert(idx < _completed.size());
  return _completed[idx];
}

inline const RecordedTable*
OffsetTableRecorder::rootTable() const
{
  return _rootIndex == InvalidTableIndex ? nullptr : &_completed[_rootIndex];
}

inline void
OffsetTableRecorder::clear()
{
  _stack.clear();
  _completed.clear();
  _rootIndex = InvalidTableIndex;
  _hasNestedTables = false;
  _signature = SignatureSeed;
}

inline void
OffsetTableRecorder::recordField(uint16_t fieldId,
                                 FieldKind kind,
                                 FieldFlags flags,
                                 size_t payloadBegin,
                                 size_t payloadEnd,
                                 uint32_t elemSize,
                                 TableIndex nestedTableIdx)
{
  assert(!_stack.empty());
  assert(payloadEnd >= payloadBegin);
  assert(payloadBegin <= std::numeric_limits<uint32_t>::max());
  const auto size = payloadEnd - payloadBegin;
  assert(size <= std::numeric_limits<uint32_t>::max());
  assert(nestedTableIdx == InvalidTableIndex ||
         nestedTableIdx <= std::numeric_limits<uint32_t>::max());
  const auto payloadOff32 = static_cast<uint32_t>(payloadBegin);
  const auto size32 = static_cast<uint32_t>(size);
  const auto nestedTableIdx32 =
    nestedTableIdx == InvalidTableIndex
      ? InvalidRecordedTableIndex
      : static_cast<uint32_t>(nestedTableIdx);
  mixSignature(static_cast<uint64_t>(fieldId));
  mixSignature(static_cast<uint64_t>(kind));
  mixSignature(static_cast<uint64_t>(flags));
  mixSignature(static_cast<uint64_t>(payloadOff32));
  mixSignature(static_cast<uint64_t>(size32));
  mixSignature(static_cast<uint64_t>(elemSize));
  mixSignature(static_cast<uint64_t>(nestedTableIdx32));
  auto& entries = _stack.back().entries;
  assert(entries.size() < std::numeric_limits<uint16_t>::max());
  auto& entry = entries.emplace_back();
  entry.payloadOff = payloadOff32;
  entry.size = size32;
  entry.elemSize = elemSize;
  entry.nestedTableIdx = nestedTableIdx32;
  entry.fieldId = fieldId;
  entry.kind = kind;
  entry.flags = flags;
}

class TableScope
{
public:
  using TableIndex = OffsetTableRecorder::TableIndex;

  TableScope()
    : _recorder{ nullptr }
    , _popped{ true }
  {
  }

  TableScope(OffsetTableRecorder& recorder,
             uint16_t typeVersion = 0,
             size_t expectedFieldCount = 0)
    : _recorder{ std::addressof(recorder) }
    , _popped{ false }
  {
    _recorder->pushTable(typeVersion, expectedFieldCount);
  }

  TableScope(const TableScope&) = delete;
  TableScope& operator=(const TableScope&) = delete;

  TableScope(TableScope&& other) noexcept
    : _recorder{ other._recorder }
    , _popped{ other._popped }
  {
    other._recorder = nullptr;
    other._popped = true;
  }

  TableScope& operator=(TableScope&& other) noexcept
  {
    if (this != std::addressof(other)) {
      finalize();
      _recorder = other._recorder;
      _popped = other._popped;
      other._recorder = nullptr;
      other._popped = true;
    }
    return *this;
  }

  ~TableScope() { finalize(); }

  TableIndex pop()
  {
    if (!_recorder)
      return InvalidTableIndex;
    assert(_recorder);
    _popped = true;
    auto idx = _recorder->popTable();
    _recorder = nullptr;
    return idx;
  }

  void cancel()
  {
    _popped = true;
    _recorder = nullptr;
  }

private:
  void finalize()
  {
    if (_recorder && !_popped) {
      _recorder->popTable();
    }
    _recorder = nullptr;
  }

  OffsetTableRecorder* _recorder;
  bool _popped;
};

template<typename Adapter>
class FieldOffsetScope
{
public:
  using TableIndex = typename OffsetTableRecorder::TableIndex;

  FieldOffsetScope() = default;

  FieldOffsetScope(OffsetTableRecorder& recorder,
                   Adapter& adapter,
                   uint16_t fieldId,
                   FieldKind kind,
                   FieldFlags flags,
                   uint32_t elemSize,
                   TableIndex nestedTableIdx = InvalidTableIndex)
    : _recorder{ std::addressof(recorder) }
    , _adapter{ std::addressof(adapter) }
    , _fieldId{ fieldId }
    , _kind{ kind }
    , _flags{ flags }
    , _elemSize{ elemSize }
    , _nestedTableIdx{ nestedTableIdx }
    , _begin{ 0 }
  {
    if constexpr (HasCurrentWritePos<Adapter>::value) {
      _begin = adapter.currentWritePos();
    } else if constexpr (HasWrittenBytesCount<Adapter>::value) {
      _begin = adapter.writtenBytesCount();
    } else {
      _recorder = nullptr;
      _adapter = nullptr;
    }
  }

  FieldOffsetScope(const FieldOffsetScope&) = delete;
  FieldOffsetScope& operator=(const FieldOffsetScope&) = delete;

  FieldOffsetScope(FieldOffsetScope&& other) noexcept
    : _recorder{ other._recorder }
    , _adapter{ other._adapter }
    , _fieldId{ other._fieldId }
    , _kind{ other._kind }
    , _flags{ other._flags }
    , _elemSize{ other._elemSize }
    , _nestedTableIdx{ other._nestedTableIdx }
    , _begin{ other._begin }
  {
    other.cancel();
  }

  FieldOffsetScope& operator=(FieldOffsetScope&& other) noexcept
  {
    if (this != std::addressof(other)) {
      _recorder = other._recorder;
      _adapter = other._adapter;
      _fieldId = other._fieldId;
      _kind = other._kind;
      _flags = other._flags;
      _elemSize = other._elemSize;
      _nestedTableIdx = other._nestedTableIdx;
      _begin = other._begin;
      other.cancel();
    }
    return *this;
  }

  ~FieldOffsetScope()
  {
    if (_recorder) {
      size_t end = _begin;
      if constexpr (HasCurrentWritePos<Adapter>::value) {
        end = _adapter->currentWritePos();
      } else if constexpr (HasWrittenBytesCount<Adapter>::value) {
        end = _adapter->writtenBytesCount();
      } else {
        return;
      }
      _recorder->recordField(
        _fieldId, _kind, _flags, _begin, end, _elemSize, _nestedTableIdx);
    }
  }

  void nestedTableIdx(TableIndex idx) { _nestedTableIdx = idx; }

  void cancel()
  {
    _recorder = nullptr;
    _adapter = nullptr;
  }

private:
  OffsetTableRecorder* _recorder{};
  Adapter* _adapter{};
  uint16_t _fieldId{};
  FieldKind _kind{ FieldKind::Scalar };
  FieldFlags _flags{ FieldFlags::None };
  uint32_t _elemSize{};
  TableIndex _nestedTableIdx{ InvalidTableIndex };
  size_t _begin{};
};

inline Entry
toEntry(const RecordedEntry& src,
        const std::vector<uint32_t>* tableOffsets,
        size_t payloadSize)
{
  Entry dst{};
  dst.fieldId = src.fieldId;
  dst.kind = src.kind;
  dst.flags = src.flags;
  dst.payloadOff = src.payloadOff;
  dst.size = src.size;
  if (src.kind == FieldKind::NestedTable &&
      src.nestedTableIdx != InvalidRecordedTableIndex) {
    assert(tableOffsets);
    assert(src.nestedTableIdx < tableOffsets->size());
    const auto nestedOff =
      payloadSize + static_cast<size_t>((*tableOffsets)[src.nestedTableIdx]);
    assert(nestedOff <= std::numeric_limits<uint32_t>::max());
    dst.elemSize = static_cast<uint32_t>(nestedOff);
  } else {
    dst.elemSize = src.elemSize;
  }
  return dst;
}

inline std::vector<uint8_t>
serializeTable(const RecordedTable& table,
               const std::vector<uint32_t>* tableOffsets,
               size_t payloadSize)
{
  assert(table.entries.size() <= std::numeric_limits<uint16_t>::max());
  const auto totalSize =
    sizeof(TableHdr) + table.entries.size() * sizeof(Entry);
  std::vector<uint8_t> buffer;
  buffer.resize(totalSize);

  TableHdr hdr{};
  hdr.fieldCount = static_cast<uint16_t>(table.entries.size());
  hdr.typeVersion = table.typeVersion;
  std::memcpy(buffer.data(), &hdr, sizeof(hdr));

  auto* out = buffer.data() + sizeof(hdr);
  for (const auto& src : table.entries) {
    auto dst = toEntry(src, tableOffsets, payloadSize);
    std::memcpy(out, &dst, sizeof(dst));
    out += sizeof(dst);
  }

  return buffer;
}

inline std::vector<uint8_t>
serializeTable(const RecordedTable& table)
{
  return serializeTable(table, nullptr, 0u);
}

inline size_t
serializedTableSize(const RecordedTable& table)
{
  return sizeof(TableHdr) + table.entries.size() * sizeof(Entry);
}

inline void
serializeTableToBuffer(const RecordedTable& table,
                       const std::vector<uint32_t>* tableOffsets,
                       size_t payloadSize,
                       uint8_t* out)
{
  TableHdr hdr{};
  hdr.fieldCount = static_cast<uint16_t>(table.entries.size());
  hdr.typeVersion = table.typeVersion;
  std::memcpy(out, &hdr, sizeof(hdr));
  auto* ptr = out + sizeof(hdr);
  for (const auto& src : table.entries) {
    auto dst = toEntry(src, tableOffsets, payloadSize);
    std::memcpy(ptr, &dst, sizeof(dst));
    ptr += sizeof(dst);
  }
}

struct StaticCacheEntry
{
  size_t payloadSize{ 0 };
  std::vector<uint8_t> postPayload;
  std::vector<uint8_t> serializedSuffix;
  uint32_t rootPostOffset{ 0 };
  bool hasNested{ false };
  size_t signature{ 0 };

  StaticCacheEntry()
    : payloadSize{ 0 }
    , postPayload{}
    , serializedSuffix{}
    , rootPostOffset{ 0 }
    , hasNested{ false }
    , signature{ 0 }
  {}
  StaticCacheEntry(const StaticCacheEntry&) = default;
  StaticCacheEntry& operator=(const StaticCacheEntry&) = default;
  StaticCacheEntry(StaticCacheEntry&&) = default;
  StaticCacheEntry& operator=(StaticCacheEntry&&) = default;
};

struct ReflectedGeneratedWriterCache
{
  std::vector<uint8_t> postPayload;
  std::vector<GeneratedRuntimeEntry> lastEntries;
  StaticCacheEntry lastCache;
  size_t lastPayloadSize{ 0 };
  bool valid{ false };
};

struct FieldRegistryMetadata
{
  bool isStatic{ false };
  bool hasNested{ false };
  bool hasAligned{ false };
};

inline void
buildSerializedSuffix(StaticCacheEntry& entry)
{
  auto flags = TrailerFlags::OffsetsValid | TrailerFlags::CrossEndianDisallowed;
  if (entry.hasNested)
    flags |= TrailerFlags::HasNestedTables;

  Trailer trailer{};
  std::copy(
    std::begin(TRAILER_MAGIC), std::end(TRAILER_MAGIC), trailer.magic.begin());
  trailer.version = TRAILER_VERSION;
  trailer.flags = static_cast<uint8_t>(flags);
  trailer.reserved = 0;
  trailer.rootTableOff = static_cast<uint32_t>(
    entry.payloadSize + static_cast<size_t>(entry.rootPostOffset));

  entry.serializedSuffix.resize(entry.postPayload.size() + sizeof(Trailer));
  if (!entry.postPayload.empty()) {
    std::memcpy(entry.serializedSuffix.data(),
                entry.postPayload.data(),
                entry.postPayload.size());
  }
  std::memcpy(entry.serializedSuffix.data() + entry.postPayload.size(),
              &trailer,
              sizeof(Trailer));
}

#if BITSERY_HAS_CPP26_REFLECTION
inline uint32_t
checkedUint32(size_t value)
{
  assert(value <= std::numeric_limits<uint32_t>::max());
  return static_cast<uint32_t>(value);
}

template<typename T>
inline StaticCacheEntry
makeReflectedStaticCacheEntry()
{
  StaticCacheEntry entry{};
  if constexpr (EnableReflectedFieldRegistry<T>::value &&
                ReflectedPayloadTraits<T>::ContiguousObject) {
    constexpr auto kCount = FieldRegistry<T>::FieldCount;
    const auto* fields = FieldRegistry<T>::entries();
    if (fields == nullptr)
      return entry;

    RecordedTable table{};
    table.typeVersion = FieldRegistry<T>::TypeVersion;
    table.entries.reserve(kCount);
    for (size_t i = 0; i < kCount; ++i) {
      const auto& field = fields[i];
      RecordedEntry recorded{};
      recorded.payloadOff = checkedUint32(field.offset);
      recorded.size = checkedUint32(field.size);
      recorded.elemSize = checkedUint32(field.elemSize);
      recorded.nestedTableIdx = InvalidRecordedTableIndex;
      recorded.fieldId = field.id;
      recorded.kind = field.kind;
      recorded.flags = field.flags;
      table.entries.push_back(recorded);
    }

    entry.payloadSize = sizeof(T);
    entry.postPayload.resize(serializedTableSize(table));
    serializeTableToBuffer(table, nullptr, 0u, entry.postPayload.data());
    entry.rootPostOffset = 0u;
    entry.hasNested = false;
    buildSerializedSuffix(entry);
  }
  return entry;
}

template<typename T>
inline const StaticCacheEntry*
cachedReflectedStaticRootEntry()
{
  if constexpr (EnableReflectedFieldRegistry<T>::value &&
                ReflectedPayloadTraits<T>::ContiguousObject) {
    static const StaticCacheEntry cached = makeReflectedStaticCacheEntry<T>();
    return cached.serializedSuffix.empty() ? nullptr : std::addressof(cached);
  } else {
    return nullptr;
  }
}

constexpr uint32_t InvalidGeneratedTableIndex =
  std::numeric_limits<uint32_t>::max();

struct ReflectedGeneratedTableLayout
{
  uint32_t fieldBase{};
  uint32_t postOffset{};
  uint16_t fieldCount{};
  uint16_t typeVersion{};
};

struct ReflectedGeneratedFieldLayout
{
  FieldInfo info{};
  uint32_t childTableIndex{ InvalidGeneratedTableIndex };
};

template<size_t TableCount, size_t FieldCount>
struct ReflectedGeneratedLayoutData
{
  std::array<ReflectedGeneratedTableLayout, TableCount> tables{};
  std::array<ReflectedGeneratedFieldLayout, FieldCount> fields{};
  uint32_t postPayloadSize{};
  bool hasNested{};
};

template<typename T>
consteval bool
reflectedGeneratedNestedTableType()
{
  using RawT =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  return EnableReflectedFieldRegistry<RawT>::value;
}

template<typename T>
consteval size_t
reflectedGeneratedTableCount()
{
  using RawT =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  size_t count = 1u;
  constexpr auto ctx = std::meta::access_context::unchecked();
  static constexpr auto members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^RawT, ctx));
  template for (constexpr auto member : members) {
    using MemberT = typename[:std::meta::type_of(member):];
    if constexpr (reflectedGeneratedNestedTableType<MemberT>())
      count += reflectedGeneratedTableCount<MemberT>();
  }
  return count;
}

template<typename T>
consteval size_t
reflectedGeneratedFieldCount()
{
  using RawT =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  size_t count = 0u;
  constexpr auto ctx = std::meta::access_context::unchecked();
  static constexpr auto members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^RawT, ctx));
  count += members.size();
  template for (constexpr auto member : members) {
    using MemberT = typename[:std::meta::type_of(member):];
    if constexpr (reflectedGeneratedNestedTableType<MemberT>())
      count += reflectedGeneratedFieldCount<MemberT>();
  }
  return count;
}

template<typename T, size_t TableCount, size_t FieldCount>
consteval void
appendReflectedGeneratedLayout(
  ReflectedGeneratedLayoutData<TableCount, FieldCount>& layout,
  size_t& nextTable,
  size_t& nextField)
{
  using RawT =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  constexpr auto ctx = std::meta::access_context::unchecked();
  static constexpr auto members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^RawT, ctx));
  static_assert(FieldRegistry<RawT>::FieldCount == members.size(),
                "Generated reflected offset-table serialization requires the "
                "reflected registry to match reflected member order.");
  static_assert(members.size() <= std::numeric_limits<uint16_t>::max(),
                "Too many reflected fields for the offset-table format.");

  const auto tableIndex = nextTable++;
  const auto fieldBase = nextField;
  nextField += members.size();
  layout.tables[tableIndex].fieldBase = static_cast<uint32_t>(fieldBase);
  layout.tables[tableIndex].fieldCount =
    static_cast<uint16_t>(members.size());
  layout.tables[tableIndex].typeVersion = FieldRegistry<RawT>::TypeVersion;

  size_t localIndex = 0u;
  template for (constexpr auto member : members) {
    using MemberT = typename[:std::meta::type_of(member):];
    auto childTableIndex = InvalidGeneratedTableIndex;
    if constexpr (reflectedGeneratedNestedTableType<MemberT>()) {
      childTableIndex = static_cast<uint32_t>(nextTable);
      appendReflectedGeneratedLayout<MemberT>(layout, nextTable, nextField);
    }
    layout.fields[fieldBase + localIndex] = ReflectedGeneratedFieldLayout{
      reflectedFieldInfo<RawT, member>(localIndex), childTableIndex
    };
    ++localIndex;
  }
}

template<typename T>
consteval auto
reflectedGeneratedLayout()
{
  constexpr auto tableCount = reflectedGeneratedTableCount<T>();
  constexpr auto fieldCount = reflectedGeneratedFieldCount<T>();
  ReflectedGeneratedLayoutData<tableCount, fieldCount> layout{};
  size_t nextTable = 0u;
  size_t nextField = 0u;
  appendReflectedGeneratedLayout<T>(layout, nextTable, nextField);
  size_t runningOffset = 0u;
  for (auto& table : layout.tables) {
    table.postOffset = static_cast<uint32_t>(runningOffset);
    runningOffset += sizeof(TableHdr) +
                     static_cast<size_t>(table.fieldCount) * sizeof(Entry);
  }
  layout.postPayloadSize = static_cast<uint32_t>(runningOffset);
  layout.hasNested = tableCount > 1u;
  return layout;
}

template<typename T>
struct ReflectedGeneratedLayout
{
  static constexpr auto Data = reflectedGeneratedLayout<T>();
  static constexpr size_t TableCount = Data.tables.size();
  static constexpr size_t FieldCount = Data.fields.size();
};
#endif

struct OffsetTableWriterState
{
  OffsetTableRecorder recorder{};
  std::vector<uint8_t> postPayload;
  bool enabled{ true };
  const FieldInfo* rootEntries{ nullptr };
  size_t rootCount{ 0 };
  bool rootStatic{ false };
  bool captureEnabled{ false };
  const StaticCacheEntry* cachedRootStatic{ nullptr };
  size_t lastDynamicSignature{ 0 };
  size_t lastDynamicPayloadSize{ 0 };
  const StaticCacheEntry* lastDynamicCacheEntry{ nullptr };
  std::vector<RecordedTable> captureTables{};
  std::vector<size_t> captureStack{};
  std::vector<uint32_t> captureTableOffsets{};
  std::vector<size_t> captureWriteOrder{};
  std::vector<RecordedTable> lastCaptureTables{};
  StaticCacheEntry lastCaptureCache{};
  std::vector<GeneratedRuntimeEntry> lastGeneratedEntries{};
  StaticCacheEntry lastGeneratedCache{};
  size_t captureTableCount{ 0 };
  size_t captureRootIndex{ InvalidTableIndex };
  size_t lastCapturePayloadSize{ 0 };
  size_t lastGeneratedPayloadSize{ 0 };
  bool captureHasNested{ false };
  bool lastCaptureValid{ false };
  bool lastGeneratedValid{ false };

  struct Frame
  {
    using TableIndex = OffsetTableRecorder::TableIndex;
    const FieldInfo* entries{};
    size_t count{};
    size_t next{};
    TableScope scope;
    size_t captureTableIdx{ InvalidTableIndex };
    size_t parentCaptureTableIdx{ InvalidTableIndex };
    size_t parentCaptureEntryIdx{ InvalidTableIndex };
    bool active{ false };
    bool hasAligned{ false };
    bool captured{ false };

    Frame(OffsetTableRecorder& rec,
          const FieldInfo* e,
          size_t c,
          uint16_t typeVersion,
          bool aligned)
      : entries{ e }
      , count{ c }
      , next{ 0 }
      , scope{ rec, typeVersion, c }
      , captureTableIdx{ InvalidTableIndex }
      , parentCaptureTableIdx{ InvalidTableIndex }
      , parentCaptureEntryIdx{ InvalidTableIndex }
      , active{ true }
      , hasAligned{ aligned }
      , captured{ false }
    {
    }

    Frame(const FieldInfo* e,
          size_t c,
          bool aligned,
          size_t capturedIdx,
          size_t parentTableIdx,
          size_t parentEntryIdx)
      : entries{ e }
      , count{ c }
      , next{ 0 }
      , scope{}
      , captureTableIdx{ capturedIdx }
      , parentCaptureTableIdx{ parentTableIdx }
      , parentCaptureEntryIdx{ parentEntryIdx }
      , active{ true }
      , hasAligned{ aligned }
      , captured{ true }
    {
    }
    Frame(const Frame&) = delete;
    Frame& operator=(const Frame&) = delete;
    Frame(Frame&&) noexcept = default;
    Frame& operator=(Frame&&) noexcept = default;
  };
  std::vector<Frame> frames{};

  OffsetTableWriterState()
    : recorder{}
    , postPayload{}
    , enabled{ true }
    , rootEntries{ nullptr }
    , rootCount{ 0 }
    , rootStatic{ false }
    , captureEnabled{ false }
    , cachedRootStatic{ nullptr }
    , lastDynamicSignature{ 0 }
    , lastDynamicPayloadSize{ 0 }
    , lastDynamicCacheEntry{ nullptr }
    , captureTables{}
    , captureStack{}
    , captureTableOffsets{}
    , captureWriteOrder{}
    , lastCaptureTables{}
    , lastCaptureCache{}
    , lastGeneratedEntries{}
    , lastGeneratedCache{}
    , captureTableCount{ 0 }
    , captureRootIndex{ InvalidTableIndex }
    , lastCapturePayloadSize{ 0 }
    , lastGeneratedPayloadSize{ 0 }
    , captureHasNested{ false }
    , lastCaptureValid{ false }
    , lastGeneratedValid{ false }
    , frames{}
  {
  }

  OffsetTableWriterState(const OffsetTableWriterState&) = default;
  OffsetTableWriterState& operator=(const OffsetTableWriterState&) = default;
  OffsetTableWriterState(OffsetTableWriterState&&) = default;
  OffsetTableWriterState& operator=(OffsetTableWriterState&&) = default;

  void clear()
  {
    recorder.clear();
    postPayload.clear();
    frames.clear();
    enabled = true;
    rootEntries = nullptr;
    rootCount = 0;
    rootStatic = false;
    captureEnabled = false;
    cachedRootStatic = nullptr;
    // Keep the last dynamic cache across uses of this thread-local state.
    // Signature and payload size are checked before reuse.
    for (auto& table : captureTables) {
      table.typeVersion = 0;
      table.entries.clear();
    }
    captureStack.clear();
    captureTableOffsets.clear();
    captureWriteOrder.clear();
    captureTableCount = 0;
    captureRootIndex = InvalidTableIndex;
    captureHasNested = false;
  }
};

class OffsetTableWriterStateLease
{
public:
  OffsetTableWriterStateLease() = default;

  OffsetTableWriterStateLease(OffsetTableWriterState* state, size_t* depth)
    : _state{ state }
    , _depth{ depth }
  {
  }

  OffsetTableWriterStateLease(const OffsetTableWriterStateLease&) = delete;
  OffsetTableWriterStateLease& operator=(const OffsetTableWriterStateLease&) =
    delete;

  OffsetTableWriterStateLease(OffsetTableWriterStateLease&& other) noexcept
    : _state{ other._state }
    , _depth{ other._depth }
  {
    other._state = nullptr;
    other._depth = nullptr;
  }

  OffsetTableWriterStateLease& operator=(OffsetTableWriterStateLease&& other) noexcept
  {
    if (this != std::addressof(other)) {
      release();
      _state = other._state;
      _depth = other._depth;
      other._state = nullptr;
      other._depth = nullptr;
    }
    return *this;
  }

  ~OffsetTableWriterStateLease() { release(); }

  OffsetTableWriterState& get() const
  {
    assert(_state);
    return *_state;
  }

private:
  void release()
  {
    if (_depth) {
      assert(*_depth > 0u);
      --(*_depth);
      _depth = nullptr;
      _state = nullptr;
    }
  }

  OffsetTableWriterState* _state{ nullptr };
  size_t* _depth{ nullptr };
};

inline OffsetTableWriterStateLease
acquireOffsetTableWriterState()
{
  struct Pool
  {
    std::deque<OffsetTableWriterState> states{};
    size_t depth{ 0u };
  };

  thread_local Pool pool{};

  if (pool.depth == pool.states.size())
    pool.states.emplace_back();

  auto& state = pool.states[pool.depth++];
  state.clear();
  return OffsetTableWriterStateLease{
    std::addressof(state), std::addressof(pool.depth) };
}

inline bool isStaticLayout(const FieldInfo* entries, size_t count);
inline std::unordered_map<const FieldInfo*, StaticCacheEntry>& staticTableCache();

template<typename T>
inline const FieldRegistryMetadata&
fieldRegistryMetadata()
{
  static const FieldRegistryMetadata metadata = [] {
    FieldRegistryMetadata res{};
    if constexpr (FieldRegistry<T>::Enabled) {
      constexpr auto kCount = FieldRegistry<T>::FieldCount;
      const auto* entries = FieldRegistry<T>::entries();
      res.isStatic = isStaticLayout(entries, kCount);
      if (entries != nullptr) {
        for (size_t i = 0; i < kCount; ++i) {
          const auto& f = entries[i];
          if (!res.hasNested &&
              (f.nestedFieldCount > 0 || f.kind == FieldKind::NestedTable)) {
            res.hasNested = true;
          }
          if (!res.hasAligned && hasFlag(f.flags, FieldFlags::Aligned)) {
            res.hasAligned = true;
          }
          if (res.hasNested && res.hasAligned)
            break;
        }
      }
    }
    return res;
  }();
  return metadata;
}

template<typename T>
inline const StaticCacheEntry*
cachedStaticRootEntry()
{
  if constexpr (!FieldRegistry<T>::Enabled) {
    return nullptr;
  } else {
    const auto& metadata = fieldRegistryMetadata<T>();
    if (!metadata.isStatic)
      return nullptr;
    static const StaticCacheEntry* cached = nullptr;
    if (cached != nullptr && !cached->postPayload.empty())
      return cached;
    const auto* entries = FieldRegistry<T>::entries();
    if (entries == nullptr)
      return nullptr;
    auto& cache = staticTableCache();
    const auto it = cache.find(entries);
    if (it == cache.end() || it->second.postPayload.empty())
      return nullptr;
    cached = std::addressof(it->second);
    return cached;
  }
}

template<typename T>
inline OffsetTableWriterState::Frame*
pushOffsetFrame(OffsetTableWriterState& state)
{
  if (!FieldRegistry<T>::Enabled || !state.enabled)
    return nullptr;
  constexpr auto kCount = FieldRegistry<T>::FieldCount;
  const auto* entries = FieldRegistry<T>::entries();
  const auto& metadata = fieldRegistryMetadata<T>();
  if (state.frames.empty()) {
    state.rootEntries = entries;
    state.rootCount = kCount;
    state.rootStatic = metadata.isStatic;
    state.cachedRootStatic = nullptr;
    if (state.rootStatic) {
      if (const auto* cached = cachedStaticRootEntry<T>()) {
        // Cache hit: skip recording; finalize will emit cached tables.
        state.cachedRootStatic = cached;
        state.enabled = false;
        return nullptr;
      }
    }
    state.captureEnabled = !state.rootStatic;
    if (state.captureEnabled) {
      state.captureTableCount = 0;
      state.captureRootIndex = InvalidTableIndex;
      state.captureHasNested = false;
      if (state.captureTables.capacity() == 0u)
        state.captureTables.reserve(4u);
      if (state.captureStack.capacity() == 0u)
        state.captureStack.reserve(4u);
      state.postPayload.reserve(sizeof(TableHdr) +
                                kCount * sizeof(Entry) +
                                sizeof(Trailer));
    }
  }
  if (state.frames.capacity() == 0u)
    state.frames.reserve(4u);
  if (state.captureEnabled) {
    const auto parentTableIdx = state.captureStack.empty()
                                  ? InvalidTableIndex
                                  : state.captureStack.back();
    auto parentEntryIdx = InvalidTableIndex;
    if (parentTableIdx != InvalidTableIndex) {
      auto& parentEntries = state.captureTables[parentTableIdx].entries;
      if (parentEntries.empty()) {
        state.captureEnabled = false;
        state.enabled = false;
        return nullptr;
      }
      parentEntryIdx = parentEntries.size() - 1u;
      state.captureHasNested = true;
    }

    const auto tableIdx = state.captureTableCount++;
    if (tableIdx == state.captureTables.size())
      state.captureTables.emplace_back();
    auto& table = state.captureTables[tableIdx];
    table.typeVersion = FieldRegistry<T>::TypeVersion;
    table.entries.clear();
    table.entries.reserve(kCount);
    state.captureStack.push_back(tableIdx);
    state.frames.emplace_back(entries,
                              kCount,
                              metadata.hasAligned,
                              tableIdx,
                              parentTableIdx,
                              parentEntryIdx);
    return &state.frames.back();
  }
  state.frames.emplace_back(state.recorder,
                            entries,
                            kCount,
                            FieldRegistry<T>::TypeVersion,
                            metadata.hasAligned);
  state.recorder.currentTable().entries.reserve(kCount);
  return &state.frames.back();
}

inline OffsetTableWriterState::Frame*
currentOffsetFrame(OffsetTableWriterState& state)
{
  if (state.frames.empty() || !state.enabled)
    return nullptr;
  return &state.frames.back();
}

inline void
closeCapturedTableFieldAt(OffsetTableWriterState& state,
                          size_t tableIdx,
                          size_t end)
{
  if (!state.captureEnabled || tableIdx >= state.captureTableCount)
    return;
  auto& entries = state.captureTables[tableIdx].entries;
  if (entries.empty())
    return;
  auto& entry = entries.back();
  assert(end >= entry.payloadOff);
  assert(end <= std::numeric_limits<uint32_t>::max());
  const auto size = end - static_cast<size_t>(entry.payloadOff);
  assert(size <= std::numeric_limits<uint32_t>::max());
  entry.size = static_cast<uint32_t>(size);
}

inline void
closeCapturedFrameFieldAt(OffsetTableWriterState& state,
                          const OffsetTableWriterState::Frame& frame,
                          size_t end)
{
  if (!frame.captured || frame.next == 0u)
    return;
  closeCapturedTableFieldAt(state, frame.captureTableIdx, end);
}

inline OffsetTableWriterState::Frame::TableIndex
popOffsetFrame(OffsetTableWriterState& state, size_t payloadEnd)
{
  assert(!state.frames.empty());
  auto& frame = state.frames.back();
  if (frame.captured) {
    const auto idx = frame.captureTableIdx;
    closeCapturedFrameFieldAt(state, frame, payloadEnd);
    if (!state.captureStack.empty())
      state.captureStack.pop_back();
    if (frame.parentCaptureTableIdx != InvalidTableIndex &&
        frame.parentCaptureEntryIdx != InvalidTableIndex) {
      auto& parentEntry =
        state.captureTables[frame.parentCaptureTableIdx]
          .entries[frame.parentCaptureEntryIdx];
      assert(idx <= std::numeric_limits<uint32_t>::max());
      assert(payloadEnd >= parentEntry.payloadOff);
      const auto size =
        payloadEnd - static_cast<size_t>(parentEntry.payloadOff);
      assert(size <= std::numeric_limits<uint32_t>::max());
      parentEntry.size = static_cast<uint32_t>(size);
      parentEntry.nestedTableIdx = static_cast<uint32_t>(idx);
      state.captureHasNested = true;
    } else {
      state.captureRootIndex = idx;
    }
    state.frames.pop_back();
    return idx;
  }
  auto idx = frame.scope.pop();
  state.frames.pop_back();
  return idx;
}

inline const FieldInfo*
nextField(OffsetTableWriterState::Frame& frame)
{
  if (!frame.active)
    return nullptr;
  if (frame.next >= frame.count || frame.entries == nullptr) {
    frame.active = false;
    return nullptr;
  }
  return std::addressof(frame.entries[frame.next++]);
}

inline void
disableCurrentFrame(OffsetTableWriterState& state)
{
  auto* frame = currentOffsetFrame(state);
  if (frame)
    frame->active = false;
  state.captureEnabled = false;
  state.captureTableCount = 0;
  state.captureStack.clear();
  state.enabled = false;
}

inline bool
isStaticLayout(const FieldInfo* entries, size_t count)
{
  if (entries == nullptr || count == 0)
    return false;
  for (size_t i = 0; i < count; ++i) {
    const auto& f = entries[i];
    if (f.nestedFieldCount > 0)
      return false;
    if (f.kind == FieldKind::Span || f.kind == FieldKind::NestedTable)
      return false;
    if (hasFlag(f.flags, FieldFlags::CopyOnly))
      return false;
  }
  return true;
}

inline std::unordered_map<const FieldInfo*, StaticCacheEntry>&
staticTableCache()
{
  static std::unordered_map<const FieldInfo*, StaticCacheEntry> cache;
  return cache;
}

template<typename Adapter>
inline size_t
writePostPayloadAndTrailer(Adapter& adapter,
                           const std::vector<uint8_t>& postPayload,
                           size_t payloadSize,
                           uint32_t rootPostOffset,
                           bool hasNested)
{
  auto flags = TrailerFlags::OffsetsValid | TrailerFlags::CrossEndianDisallowed;
  if (hasNested)
    flags |= TrailerFlags::HasNestedTables;

  const auto totalPayloadSize = payloadSize + postPayload.size();
  assert(payloadSize <= std::numeric_limits<uint32_t>::max());
  assert(totalPayloadSize <= std::numeric_limits<uint32_t>::max());
  const auto rootTableOff =
    payloadSize + static_cast<size_t>(rootPostOffset);
  assert(rootTableOff <= std::numeric_limits<uint32_t>::max());

  Trailer trailer{};
  std::copy(
    std::begin(TRAILER_MAGIC), std::end(TRAILER_MAGIC), trailer.magic.begin());
  trailer.version = TRAILER_VERSION;
  trailer.flags = static_cast<uint8_t>(flags);
  trailer.reserved = 0;
  trailer.rootTableOff = static_cast<uint32_t>(rootTableOff);

  if (!postPayload.empty()) {
    adapter.template writeBuffer<1>(postPayload.data(), postPayload.size());
  }
  adapter.template writeBuffer<1>(reinterpret_cast<const uint8_t*>(&trailer),
                                  sizeof(trailer));
  return payloadSize + postPayload.size() + sizeof(trailer);
}

template<typename Adapter>
inline size_t
writeCachedTablesAndTrailer(Adapter& adapter,
                            const StaticCacheEntry& cached,
                            size_t payloadSize)
{
  assert(cached.payloadSize == payloadSize);
  if (!cached.serializedSuffix.empty()) {
    adapter.template writeBuffer<1>(cached.serializedSuffix.data(),
                                    cached.serializedSuffix.size());
    return payloadSize + cached.serializedSuffix.size();
  }
  return writePostPayloadAndTrailer(
    adapter,
    cached.postPayload,
    payloadSize,
    cached.rootPostOffset,
    cached.hasNested);
}

template<typename Adapter>
inline size_t
writeTablesAndTrailer(Adapter& adapter,
                      OffsetTableWriterState& state,
                      size_t payloadSize)
{
  // Capture-based fast path for dynamic layouts: record only table entries while
  // writing the payload, then reuse the serialized suffix for repeated shapes.
  if (state.captureEnabled && state.captureTableCount != 0u) {
    if (!state.frames.empty())
      closeCapturedFrameFieldAt(state, state.frames.back(), payloadSize);
    const auto tableCount = state.captureTableCount;
    const auto rootIdx = state.captureRootIndex;
    if (rootIdx == InvalidTableIndex || rootIdx >= tableCount)
      return payloadSize;

    if (state.lastCaptureValid &&
        state.lastCapturePayloadSize == payloadSize &&
        sameRecordedTables(state.lastCaptureTables,
                           state.lastCaptureTables.size(),
                           state.captureTables,
                           tableCount)) {
      state.enabled = true;
      return writeCachedTablesAndTrailer(
        adapter, state.lastCaptureCache, payloadSize);
    }

    state.captureWriteOrder.clear();
    state.captureWriteOrder.reserve(tableCount);
    state.captureWriteOrder.push_back(rootIdx);
    for (size_t idx = 0; idx < tableCount; ++idx) {
      if (idx != rootIdx)
        state.captureWriteOrder.push_back(idx);
    }

    state.captureTableOffsets.resize(tableCount);
    size_t runningOffset = 0;
    for (auto idx : state.captureWriteOrder) {
      const auto sz = serializedTableSize(state.captureTables[idx]);
      assert(runningOffset <= std::numeric_limits<uint32_t>::max());
      state.captureTableOffsets[idx] = static_cast<uint32_t>(runningOffset);
      runningOffset += sz;
    }

    state.postPayload.clear();
    state.postPayload.resize(runningOffset);
    for (auto idx : state.captureWriteOrder) {
      const auto offset = static_cast<size_t>(state.captureTableOffsets[idx]);
      serializeTableToBuffer(state.captureTables[idx],
                             &state.captureTableOffsets,
                             payloadSize,
                             state.postPayload.data() + offset);
    }

    state.lastCaptureTables.assign(state.captureTables.begin(),
                                   state.captureTables.begin() +
                                     static_cast<std::ptrdiff_t>(tableCount));
    state.lastCapturePayloadSize = payloadSize;
    state.lastCaptureCache = StaticCacheEntry{};
    state.lastCaptureCache.payloadSize = payloadSize;
    state.lastCaptureCache.postPayload = state.postPayload;
    state.lastCaptureCache.rootPostOffset = state.captureTableOffsets[rootIdx];
    state.lastCaptureCache.hasNested = state.captureHasNested;
    buildSerializedSuffix(state.lastCaptureCache);
    state.lastCaptureValid = true;
    state.enabled = true;
    return writeCachedTablesAndTrailer(
      adapter, state.lastCaptureCache, payloadSize);
  }

  const StaticCacheEntry* cached = state.cachedRootStatic;
  if (cached != nullptr &&
      (cached->payloadSize != payloadSize || cached->postPayload.empty())) {
    cached = nullptr;
  }

  if (!state.enabled) {
    if (cached != nullptr) {
      state.enabled = true;
      return writeCachedTablesAndTrailer(adapter, *cached, payloadSize);
    } else {
      for (auto& frame : state.frames) {
        frame.scope.cancel();
      }
      state.frames.clear();
      state.recorder = OffsetTableRecorder{};
      state.postPayload.clear();
      state.enabled = true;
      return payloadSize;
    }
  }
  auto& recorder = state.recorder;
  while (recorder.hasOpenTable()) {
    recorder.popTable();
  }

  const auto& tables = recorder.completedTables();
  if (tables.empty())
    return payloadSize;

  const auto rootIdx = recorder.rootTableIndex();
  if (rootIdx == InvalidTableIndex || rootIdx >= tables.size())
    return payloadSize;

  static std::unordered_map<size_t, StaticCacheEntry> staticCache;
  const size_t signature = recorder.signature();
  if (state.lastDynamicCacheEntry != nullptr &&
      state.lastDynamicSignature == signature &&
      state.lastDynamicPayloadSize == payloadSize &&
      !state.lastDynamicCacheEntry->postPayload.empty()) {
    state.enabled = true;
    return writeCachedTablesAndTrailer(
      adapter, *state.lastDynamicCacheEntry, payloadSize);
  }

  uint32_t rootPostOffset = 0;
  auto cacheIt = staticCache.find(signature);
  if (cacheIt != staticCache.end() &&
      cacheIt->second.payloadSize == payloadSize &&
      !cacheIt->second.postPayload.empty()) {
    state.lastDynamicSignature = signature;
    state.lastDynamicPayloadSize = payloadSize;
    state.lastDynamicCacheEntry = std::addressof(cacheIt->second);
    state.enabled = true;
    return writeCachedTablesAndTrailer(adapter, cacheIt->second, payloadSize);
  } else {
    state.postPayload.clear();
    std::vector<uint32_t> tableOffsets(tables.size(), 0u);
    std::vector<size_t> writeOrder;
    writeOrder.reserve(tables.size());
    writeOrder.push_back(rootIdx);
    for (size_t idx = 0; idx < tables.size(); ++idx) {
      if (idx != rootIdx)
        writeOrder.push_back(idx);
    }

    size_t runningOffset = 0;
    for (auto idx : writeOrder) {
      const auto sz = serializedTableSize(tables[idx]);
      assert(runningOffset <= std::numeric_limits<uint32_t>::max());
      tableOffsets[idx] = static_cast<uint32_t>(runningOffset);
      runningOffset += sz;
    }

    state.postPayload.clear();
    state.postPayload.resize(runningOffset);
    for (auto idx : writeOrder) {
      const auto offset = static_cast<size_t>(tableOffsets[idx]);
      serializeTableToBuffer(tables[idx],
                             &tableOffsets,
                             payloadSize,
                             state.postPayload.data() + offset);
    }

    rootPostOffset = tableOffsets[rootIdx];

    StaticCacheEntry entry{};
    entry.payloadSize = payloadSize;
    entry.postPayload = state.postPayload;
    entry.rootPostOffset = rootPostOffset;
    entry.hasNested = recorder.hasNestedTables();
    entry.signature = signature;
    buildSerializedSuffix(entry);
    staticCache[signature] = std::move(entry);
    state.lastDynamicSignature = signature;
    state.lastDynamicPayloadSize = payloadSize;
    state.lastDynamicCacheEntry = std::addressof(staticCache[signature]);
    if (state.rootStatic && state.rootEntries != nullptr &&
        !recorder.hasNestedTables()) {
      auto& tableCache = staticTableCache();
      tableCache[state.rootEntries] = staticCache[signature];
    }
  }

  state.enabled = true;
  return writePostPayloadAndTrailer(
    adapter,
    state.postPayload,
    payloadSize,
    rootPostOffset,
    recorder.hasNestedTables());
}

#if BITSERY_HAS_CPP26_REFLECTION
template<typename TAdapter>
inline size_t
reflectedGeneratedWritePos(TAdapter& adapter)
{
  if constexpr (HasCurrentWritePos<TAdapter>::value) {
    return adapter.currentWritePos();
  } else if constexpr (HasWrittenBytesCount<TAdapter>::value) {
    return adapter.writtenBytesCount();
  } else {
    return 0u;
  }
}

template<typename TAdapter>
inline size_t
reflectedGeneratedAlignedBegin(TAdapter& adapter, const FieldInfo& field)
{
  auto begin = reflectedGeneratedWritePos(adapter);
  if constexpr (HasCurrentWritePos<TAdapter>::value) {
    if (hasFlag(field.flags, FieldFlags::Aligned) && field.align > 1) {
      const auto padding =
        static_cast<size_t>((field.align - (begin % field.align)) % field.align);
      if (padding > 0u)
        adapter.currentWritePos(begin + padding);
      begin += padding;
    }
  }
  return begin;
}

template<typename TRoot,
         size_t TableIndex,
         typename TAdapter,
         typename T,
         typename RuntimeEntries>
inline void
writeReflectedGeneratedObjectPayload(TAdapter& adapter,
                                     RuntimeEntries& runtimeEntries,
                                     const T& obj);

template<typename TRoot,
         uint32_t ChildTableIndex,
         typename TAdapter,
         typename T,
         typename RuntimeEntries>
inline void
writeReflectedGeneratedFieldPayload(TAdapter& adapter,
                                    RuntimeEntries& runtimeEntries,
                                    size_t runtimeEntryIdx,
                                    const FieldInfo& field,
                                    const T& value)
{
  const auto begin = reflectedGeneratedAlignedBegin(adapter, field);
  auto& runtime = runtimeEntries[runtimeEntryIdx];
  runtime.payloadOff = static_cast<uint32_t>(begin);

  if constexpr (ChildTableIndex != InvalidGeneratedTableIndex) {
    writeReflectedGeneratedObjectPayload<TRoot,
                                         static_cast<size_t>(ChildTableIndex)>(
      adapter, runtimeEntries, value);
    runtime.size =
      static_cast<uint32_t>(reflectedGeneratedWritePos(adapter) - begin);
  } else {
    writeReflectedPayloadField(adapter, value);
    runtime.size =
      static_cast<uint32_t>(reflectedGeneratedWritePos(adapter) - begin);
  }
}

template<typename TRoot,
         size_t TableIndex,
         size_t FieldIndex,
         typename TAdapter,
         typename T,
         typename RuntimeEntries>
inline void
writeReflectedGeneratedObjectField(TAdapter& adapter,
                                   RuntimeEntries& runtimeEntries,
                                   const T& obj)
{
  using RawT =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  static constexpr auto layout = ReflectedGeneratedLayout<TRoot>::Data;
  static constexpr auto table = layout.tables[TableIndex];
  static constexpr auto runtimeEntryIdx = table.fieldBase + FieldIndex;
  static constexpr auto fieldLayout = layout.fields[runtimeEntryIdx];
  constexpr auto ctx = std::meta::access_context::unchecked();
  static constexpr auto members = std::define_static_array(
    std::meta::nonstatic_data_members_of(^^RawT, ctx));
  constexpr auto member = members[FieldIndex];
  writeReflectedGeneratedFieldPayload<TRoot,
                                      fieldLayout.childTableIndex>(
    adapter,
    runtimeEntries,
    runtimeEntryIdx,
    fieldLayout.info,
    obj.[:member:]);
}

template<typename TRoot,
         size_t TableIndex,
         typename TAdapter,
         typename T,
         typename RuntimeEntries,
         size_t... FieldIndex>
inline void
writeReflectedGeneratedObjectFields(TAdapter& adapter,
                                    RuntimeEntries& runtimeEntries,
                                    const T& obj,
                                    std::index_sequence<FieldIndex...>)
{
  (writeReflectedGeneratedObjectField<TRoot, TableIndex, FieldIndex>(
     adapter, runtimeEntries, obj),
   ...);
}

template<typename TRoot,
         size_t TableIndex,
         typename TAdapter,
         typename T,
         typename RuntimeEntries>
inline void
writeReflectedGeneratedObjectPayload(TAdapter& adapter,
                                     RuntimeEntries& runtimeEntries,
                                     const T& obj)
{
  static constexpr auto layout = ReflectedGeneratedLayout<TRoot>::Data;
  static constexpr auto table = layout.tables[TableIndex];
  writeReflectedGeneratedObjectFields<TRoot, TableIndex>(
    adapter,
    runtimeEntries,
    obj,
    std::make_index_sequence<table.fieldCount>{});
}

template<typename RuntimeEntries>
inline bool
sameGeneratedRuntimeEntries(const std::vector<GeneratedRuntimeEntry>& lhs,
                            const RuntimeEntries& rhs)
{
  if (lhs.size() != rhs.size())
    return false;
  if (rhs.size() == 0u)
    return true;
  return std::memcmp(lhs.data(),
                     rhs.data(),
                     rhs.size() * sizeof(GeneratedRuntimeEntry)) == 0;
}

template<typename TRoot,
         size_t TableIndex,
         size_t FieldIndex,
         typename RuntimeEntries>
inline void
writeReflectedGeneratedTableEntry(const RuntimeEntries& runtimeEntries,
                                  size_t payloadSize,
                                  uint8_t*& out)
{
  static constexpr auto layout = ReflectedGeneratedLayout<TRoot>::Data;
  static constexpr auto table = layout.tables[TableIndex];
  static constexpr auto runtimeEntryIdx = table.fieldBase + FieldIndex;
  static constexpr auto fieldLayout = layout.fields[runtimeEntryIdx];
  const auto& runtime = runtimeEntries[runtimeEntryIdx];

  Entry entry{};
  entry.fieldId = fieldLayout.info.id;
  entry.kind = fieldLayout.info.kind;
  entry.flags = fieldLayout.info.flags;
  entry.payloadOff = runtime.payloadOff;
  entry.size = runtime.size;
  if constexpr (fieldLayout.childTableIndex != InvalidGeneratedTableIndex) {
    static constexpr auto childTable =
      layout.tables[fieldLayout.childTableIndex];
    const auto nestedOff =
      payloadSize + static_cast<size_t>(childTable.postOffset);
    assert(nestedOff <= std::numeric_limits<uint32_t>::max());
    entry.elemSize = static_cast<uint32_t>(nestedOff);
  } else {
    entry.elemSize = fieldLayout.info.elemSize;
  }
  std::memcpy(out, &entry, sizeof(entry));
  out += sizeof(entry);
}

template<typename TRoot,
         size_t TableIndex,
         typename RuntimeEntries,
         size_t... FieldIndex>
inline void
writeReflectedGeneratedTableEntries(const RuntimeEntries& runtimeEntries,
                                    size_t payloadSize,
                                    uint8_t*& out,
                                    std::index_sequence<FieldIndex...>)
{
  (writeReflectedGeneratedTableEntry<TRoot, TableIndex, FieldIndex>(
     runtimeEntries, payloadSize, out),
   ...);
}

template<typename TRoot, size_t TableIndex, typename RuntimeEntries>
inline void
writeReflectedGeneratedTableToBuffer(const RuntimeEntries& runtimeEntries,
                                     size_t payloadSize,
                                     uint8_t* postPayload)
{
  static constexpr auto layout = ReflectedGeneratedLayout<TRoot>::Data;
  static constexpr auto table = layout.tables[TableIndex];
  auto* out = postPayload + table.postOffset;
  TableHdr hdr{};
  hdr.fieldCount = table.fieldCount;
  hdr.typeVersion = table.typeVersion;
  std::memcpy(out, &hdr, sizeof(hdr));
  out += sizeof(hdr);
  writeReflectedGeneratedTableEntries<TRoot, TableIndex>(
    runtimeEntries,
    payloadSize,
    out,
    std::make_index_sequence<table.fieldCount>{});
}

template<typename TRoot, typename RuntimeEntries, size_t... TableIndex>
inline void
writeReflectedGeneratedTablesToBuffer(const RuntimeEntries& runtimeEntries,
                                      size_t payloadSize,
                                      uint8_t* postPayload,
                                      std::index_sequence<TableIndex...>)
{
  (writeReflectedGeneratedTableToBuffer<TRoot, TableIndex>(
     runtimeEntries, payloadSize, postPayload),
   ...);
}

template<typename TRoot, typename Adapter, typename RuntimeEntries>
inline size_t
writeReflectedGeneratedTablesAndTrailer(Adapter& adapter,
                                        OffsetTableWriterState& state,
                                        const RuntimeEntries& runtimeEntries,
                                        size_t payloadSize)
{
  static constexpr auto layout = ReflectedGeneratedLayout<TRoot>::Data;
  if (state.lastGeneratedValid &&
      state.lastGeneratedPayloadSize == payloadSize &&
      sameGeneratedRuntimeEntries(state.lastGeneratedEntries, runtimeEntries) &&
      !state.lastGeneratedCache.serializedSuffix.empty()) {
    return writeCachedTablesAndTrailer(
      adapter, state.lastGeneratedCache, payloadSize);
  }

  state.postPayload.clear();
  state.postPayload.resize(layout.postPayloadSize);
  writeReflectedGeneratedTablesToBuffer<TRoot>(
    runtimeEntries,
    payloadSize,
    state.postPayload.data(),
    std::make_index_sequence<ReflectedGeneratedLayout<TRoot>::TableCount>{});

  state.lastGeneratedEntries.assign(runtimeEntries.begin(),
                                    runtimeEntries.end());
  state.lastGeneratedPayloadSize = payloadSize;
  state.lastGeneratedCache = StaticCacheEntry{};
  state.lastGeneratedCache.payloadSize = payloadSize;
  state.lastGeneratedCache.postPayload = state.postPayload;
  state.lastGeneratedCache.rootPostOffset = layout.tables[0].postOffset;
  state.lastGeneratedCache.hasNested = layout.hasNested;
  buildSerializedSuffix(state.lastGeneratedCache);
  state.lastGeneratedValid = true;
  return writeCachedTablesAndTrailer(
    adapter, state.lastGeneratedCache, payloadSize);
}

template<typename TRoot, typename Adapter, typename RuntimeEntries>
inline size_t
writeReflectedGeneratedTablesAndTrailer(
  Adapter& adapter,
  ReflectedGeneratedWriterCache& cache,
  const RuntimeEntries& runtimeEntries,
  size_t payloadSize)
{
  static constexpr auto layout = ReflectedGeneratedLayout<TRoot>::Data;
  if (cache.valid && cache.lastPayloadSize == payloadSize &&
      sameGeneratedRuntimeEntries(cache.lastEntries, runtimeEntries) &&
      !cache.lastCache.serializedSuffix.empty()) {
    return writeCachedTablesAndTrailer(adapter, cache.lastCache, payloadSize);
  }

  cache.postPayload.clear();
  cache.postPayload.resize(layout.postPayloadSize);
  writeReflectedGeneratedTablesToBuffer<TRoot>(
    runtimeEntries,
    payloadSize,
    cache.postPayload.data(),
    std::make_index_sequence<ReflectedGeneratedLayout<TRoot>::TableCount>{});

  cache.lastEntries.assign(runtimeEntries.begin(), runtimeEntries.end());
  cache.lastPayloadSize = payloadSize;
  cache.lastCache = StaticCacheEntry{};
  cache.lastCache.payloadSize = payloadSize;
  cache.lastCache.postPayload = cache.postPayload;
  cache.lastCache.rootPostOffset = layout.tables[0].postOffset;
  cache.lastCache.hasNested = layout.hasNested;
  buildSerializedSuffix(cache.lastCache);
  cache.valid = true;
  return writeCachedTablesAndTrailer(adapter, cache.lastCache, payloadSize);
}

template<typename TAdapter, typename T>
inline size_t
reflectSerializeGeneratedWithOffsetTable(OffsetTableWriterState& state,
                                         TAdapter adapter,
                                         const T& value)
{
  static_assert(HasCurrentWritePos<TAdapter>::value ||
                  HasWrittenBytesCount<TAdapter>::value,
                "Generated reflected offset-table serialization requires an "
                "adapter that can report the current write position.");
  state.clear();
  if (TAdapter::TConfig::Endianness != getSystemEndianness()) {
    writeReflectedPayload(adapter, value);
    adapter.flush();
    return reflectedGeneratedWritePos(adapter);
  }

  using RawT =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  std::array<GeneratedRuntimeEntry, ReflectedGeneratedLayout<RawT>::FieldCount>
    runtimeEntries{};
  writeReflectedGeneratedObjectPayload<RawT, 0u>(
    adapter, runtimeEntries, value);
  adapter.flush();
  return writeReflectedGeneratedTablesAndTrailer<RawT>(
    adapter, state, runtimeEntries, reflectedGeneratedWritePos(adapter));
}

template<typename TAdapter, typename T>
inline size_t
reflectSerializeGeneratedWithOffsetTable(TAdapter adapter, const T& value)
{
  static_assert(HasCurrentWritePos<TAdapter>::value ||
                  HasWrittenBytesCount<TAdapter>::value,
                "Generated reflected offset-table serialization requires an "
                "adapter that can report the current write position.");
  if (TAdapter::TConfig::Endianness != getSystemEndianness()) {
    writeReflectedPayload(adapter, value);
    adapter.flush();
    return reflectedGeneratedWritePos(adapter);
  }

  using RawT =
    typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  thread_local ReflectedGeneratedWriterCache cache{};
  std::array<GeneratedRuntimeEntry, ReflectedGeneratedLayout<RawT>::FieldCount>
    runtimeEntries;
  writeReflectedGeneratedObjectPayload<RawT, 0u>(
    adapter, runtimeEntries, value);
  adapter.flush();
  return writeReflectedGeneratedTablesAndTrailer<RawT>(
    adapter, cache, runtimeEntries, reflectedGeneratedWritePos(adapter));
}
#endif

struct TrailerInfo
{
  bool valid{};
  Trailer trailer{};
  size_t payloadSize{};
  size_t tablesSize{};
};

inline TrailerInfo
parseTrailer(const uint8_t* data, size_t size)
{
  TrailerInfo info{};
  if (size < sizeof(Trailer))
    return info;
  auto* trailerPos = data + (size - sizeof(Trailer));
  std::memcpy(&info.trailer, trailerPos, sizeof(Trailer));
  info.valid = std::equal(std::begin(TRAILER_MAGIC),
                          std::end(TRAILER_MAGIC),
                          info.trailer.magic.begin()) &&
               info.trailer.version == TRAILER_VERSION;
  if (!info.valid)
    return info;
  if (info.trailer.rootTableOff > size - sizeof(Trailer)) {
    info.valid = false;
    return info;
  }
  info.payloadSize = static_cast<size_t>(info.trailer.rootTableOff);
  info.tablesSize = size - sizeof(Trailer) - info.payloadSize;
  return info;
}

template<typename Config>
inline TrailerInfo
verifyTrailer(const uint8_t* data, size_t size)
{
  auto info = parseTrailer(data, size);
  if (!info.valid)
    return info;
  if (Config::Endianness != getSystemEndianness()) {
    info.valid = false;
    return info;
  }
  return info;
}

}

}

#endif // BITSERY_DETAILS_OFFSET_TABLE_H
