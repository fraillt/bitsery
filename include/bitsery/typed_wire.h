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

#ifndef BITSERY_TYPED_WIRE_H
#define BITSERY_TYPED_WIRE_H

#include "common.h"
#include "details/adapter_common.h"
#include "details/serialization_common.h"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <type_traits>

#if defined(__cpp_impl_reflection) && __cpp_impl_reflection >= 202506L
#include <meta>
#define BITSERY_HAS_CPP26_REFLECTION 1
#else
#define BITSERY_HAS_CPP26_REFLECTION 0
#endif

namespace bitsery {
namespace tw {

enum class Status
{
  Ok,
  NoReflection,
  WrongEndianness,
  Truncated,
  VersionMismatch,
  Absent,
  Misaligned,
  Unsupported
};

struct ByteSpan
{
  const uint8_t* data{ nullptr };
  size_t size{ 0 };

  const uint8_t* begin() const { return data; }
  const uint8_t* end() const { return data + size; }
};

namespace detail {

template<typename T>
using Raw = typename std::remove_cv<typename std::remove_reference<T>::type>::type;

template<typename T>
struct AlwaysFalse : std::false_type
{};

struct Cursor
{
  const uint8_t* data{};
  size_t size{};
  Status status{ Status::Ok };
};

inline bool
take(Cursor& in, size_t& pos, size_t bytes)
{
  if (pos > in.size)
    return false;
  if (bytes > in.size - pos) {
    if (pos < in.size)
      in.status = Status::Truncated;
    return false;
  }
  pos += bytes;
  return true;
}

} // namespace detail

template<typename TValue>
struct FieldView
{
  using RawT = detail::Raw<TValue>;

  const RawT* value{ nullptr };
  ByteSpan bytes{};
  bool present{ false };
  Status status{ Status::Absent };

  RawT copy() const
  {
    RawT out{};
    if (bytes.size >= sizeof(RawT))
      std::memcpy(&out, bytes.data, sizeof(RawT));
    return out;
  }
};

namespace detail {

inline bool
readSize(Cursor& in, size_t& pos, size_t& value)
{
  if (pos == in.size)
    return false;
  const auto hb = in.data[pos++];
  if (hb < 0x80u) {
    value = hb;
    return true;
  }
  if (pos == in.size) {
    in.status = Status::Truncated;
    return false;
  }
  const auto lb = in.data[pos++];
  if ((hb & 0x40u) == 0u) {
    value = (static_cast<size_t>(hb & 0x7Fu) << 8u) | lb;
    return true;
  }
  if (in.size - pos < 2u) {
    in.status = Status::Truncated;
    return false;
  }
  uint16_t lw{};
  std::memcpy(&lw, in.data + pos, sizeof(lw));
  pos += sizeof(lw);
  value = (((static_cast<size_t>(hb & 0x3Fu) << 8u) | lb) << 16u) | lw;
  return true;
}

template<typename T>
constexpr bool
reflectedAggregate()
{
#if BITSERY_HAS_CPP26_REFLECTION
  return std::is_aggregate<T>::value && std::is_standard_layout<T>::value &&
         !details::IsFundamentalType<T>::value &&
         !details::IsContainerTraitsDefined<T>::value &&
         !details::IsTextTraitsDefined<T>::value;
#else
  return false;
#endif
}

template<typename TAdapter, typename T>
inline void writeOne(TAdapter& adapter, const T& value);

template<typename TAdapter, typename T>
inline void
writeRange(TAdapter& adapter, const T& value, size_t count)
{
  auto first = std::begin(value);
  using ValueT = Raw<decltype(*first)>;
  if constexpr (traits::ContainerTraits<T>::isContiguous &&
                details::IsFundamentalType<ValueT>::value) {
    if (count != 0u) {
      using IntT = typename details::IntegralFromFundamental<ValueT>::TValue;
      adapter.template writeBuffer<sizeof(ValueT)>(
        reinterpret_cast<const IntT*>(&(*first)), count);
    }
  } else {
    using DiffT = typename std::iterator_traits<decltype(first)>::difference_type;
    auto last = std::next(first, static_cast<DiffT>(count));
    for (; first != last; ++first)
      writeOne(adapter, *first);
  }
}

#if BITSERY_HAS_CPP26_REFLECTION
template<typename T>
consteval auto
members()
{
  constexpr auto ctx = std::meta::access_context::unchecked();
  return std::define_static_array(std::meta::nonstatic_data_members_of(^^T, ctx));
}

template<typename T>
consteval size_t
fieldCount()
{
  return members<T>().size();
}

template<typename T, size_t Index>
consteval std::meta::info
member()
{
  return members<T>()[Index];
}

template<typename T, size_t Index>
using MemberT = typename[:std::meta::type_of(member<T, Index>()):];

consteval bool
isVersionMember(std::meta::info info)
{
  return std::meta::has_identifier(info) &&
         std::meta::identifier_of(info) == "version";
}

template<typename T>
consteval bool
hasVersion()
{
  if constexpr (fieldCount<T>() == 0u)
    return false;
  else
    return isVersionMember(member<T, 0u>());
}

template<typename T>
consteval bool
misplacedVersion()
{
  bool bad = false;
  size_t index = 0u;
  template for (constexpr auto field : members<T>()) {
    if (isVersionMember(field) && index != 0u)
      bad = true;
    ++index;
  }
  return bad;
}

template<typename T>
consteval void
validateVersion()
{
  static_assert(!misplacedVersion<T>(),
                "typed wire version must be the first member");
  if constexpr (hasVersion<T>()) {
    using VersionT = MemberT<T, 0u>;
    static_assert(std::is_integral<VersionT>::value &&
                    !std::is_same<VersionT, bool>::value,
                  "typed wire version must be a non-bool integral member");
  }
}

template<typename TAdapter, typename T>
inline void
writeObject(TAdapter& adapter, const T& value)
{
  validateVersion<T>();
  template for (constexpr auto field : members<T>())
    writeOne(adapter, value.[:field:]);
}
#endif

template<typename TAdapter, typename T>
inline void
writeOne(TAdapter& adapter, const T& value)
{
  using TClean = Raw<T>;
  if constexpr (details::IsTextTraitsDefined<TClean>::value) {
    const auto count = traits::TextTraits<TClean>::length(value);
    details::writeSize(adapter, count);
    writeRange(adapter, value, count);
  } else if constexpr (details::IsContainerTraitsDefined<TClean>::value) {
    const auto count = traits::ContainerTraits<TClean>::size(value);
    if constexpr (traits::ContainerTraits<TClean>::isResizable)
      details::writeSize(adapter, count);
    writeRange(adapter, value, count);
  } else if constexpr (details::IsFundamentalType<TClean>::value) {
    using IntT = typename details::IntegralFromFundamental<TClean>::TValue;
    adapter.template writeBytes<sizeof(TClean)>(
      reinterpret_cast<const IntT&>(value));
  } else if constexpr (reflectedAggregate<TClean>()) {
#if BITSERY_HAS_CPP26_REFLECTION
    writeObject(adapter, value);
#else
    static_assert(AlwaysFalse<TClean>::value,
                  "typed wire serialization requires C++26 reflection");
#endif
  } else {
    static_assert(AlwaysFalse<TClean>::value,
                  "unsupported typed wire field type");
  }
}

template<typename T>
inline bool skipOne(Cursor& in, size_t& pos);

#if BITSERY_HAS_CPP26_REFLECTION
template<typename T>
inline bool
skipObject(Cursor& in, size_t& pos)
{
  validateVersion<T>();
  template for (constexpr auto field : members<T>()) {
    using FieldT = typename[:std::meta::type_of(field):];
    if (!skipOne<FieldT>(in, pos) && in.status != Status::Ok)
      return false;
  }
  return in.status == Status::Ok;
}
#endif

template<typename T>
inline bool
skipElements(Cursor& in, size_t& pos, size_t count)
{
  using ValueT = typename traits::ContainerTraits<T>::TValue;
  if constexpr (traits::ContainerTraits<T>::isContiguous &&
                details::IsFundamentalType<ValueT>::value) {
    if (count > std::numeric_limits<size_t>::max() / sizeof(ValueT)) {
      in.status = Status::Truncated;
      return false;
    }
    return take(in, pos, count * sizeof(ValueT));
  } else {
    for (size_t i = 0u; i < count; ++i) {
      if (!skipOne<ValueT>(in, pos))
        return false;
    }
    return true;
  }
}

template<typename T>
inline bool
skipOne(Cursor& in, size_t& pos)
{
  using TClean = Raw<T>;
  if (pos == in.size)
    return false;
  if constexpr (details::IsTextTraitsDefined<TClean>::value) {
    size_t count{};
    return readSize(in, pos, count) && skipElements<TClean>(in, pos, count);
  } else if constexpr (details::IsContainerTraitsDefined<TClean>::value) {
    size_t count{};
    if constexpr (traits::ContainerTraits<TClean>::isResizable) {
      if (!readSize(in, pos, count))
        return false;
    } else {
      count = traits::ContainerTraits<TClean>::size(TClean{});
    }
    return skipElements<TClean>(in, pos, count);
  } else if constexpr (details::IsFundamentalType<TClean>::value) {
    return take(in, pos, sizeof(TClean));
  } else if constexpr (reflectedAggregate<TClean>()) {
#if BITSERY_HAS_CPP26_REFLECTION
    return skipObject<TClean>(in, pos);
#else
    in.status = Status::NoReflection;
    return false;
#endif
  } else {
    in.status = Status::Unsupported;
    return false;
  }
}

template<typename T>
inline FieldView<T>
readOne(Cursor& in, size_t& pos)
{
  using TClean = Raw<T>;
  FieldView<TClean> out{};
  if (pos == in.size)
    return out;

  size_t offset = pos;
  if constexpr (details::IsTextTraitsDefined<TClean>::value) {
    size_t count{};
    if (readSize(in, pos, count)) {
      offset = pos;
      out.present = skipElements<TClean>(in, pos, count);
    }
  } else if constexpr (details::IsContainerTraitsDefined<TClean>::value) {
    size_t count{};
    if constexpr (traits::ContainerTraits<TClean>::isResizable) {
      if (!readSize(in, pos, count))
        return out;
      offset = pos;
    } else {
      count = traits::ContainerTraits<TClean>::size(TClean{});
    }
    out.present = skipElements<TClean>(in, pos, count);
  } else {
    out.present = skipOne<TClean>(in, pos);
  }
  if (!out.present)
    return out;

  out.bytes = ByteSpan{ in.data + offset, pos - offset };
  out.status = in.status;
  if constexpr (std::is_trivially_copyable<TClean>::value) {
    if (out.bytes.size >= sizeof(TClean)) {
      auto* ptr = out.bytes.data;
      if (reinterpret_cast<uintptr_t>(ptr) % alignof(TClean) == 0u)
        out.value = reinterpret_cast<const TClean*>(ptr);
      else
        out.status = Status::Misaligned;
    }
  }
  return out;
}

#if BITSERY_HAS_CPP26_REFLECTION
template<typename T, size_t... Index>
inline bool
skipPrefix(Cursor& in, size_t& pos, std::index_sequence<Index...>)
{
  return (skipOne<MemberT<T, Index>>(in, pos) && ...);
}

template<typename T>
inline uint64_t
defaultVersion()
{
  T value{};
  constexpr auto version = member<T, 0u>();
  return static_cast<uint64_t>(value.[:version:]);
}

template<typename T>
inline Status
checkVersion(const uint8_t* data,
             size_t size,
             bool hasExpected,
             uint64_t expected)
{
  validateVersion<T>();
  if constexpr (!hasVersion<T>()) {
    (void)data;
    (void)size;
    (void)hasExpected;
    (void)expected;
    return Status::Ok;
  } else {
    using VersionT = MemberT<T, 0u>;
    if (size < sizeof(VersionT))
      return Status::Truncated;
    VersionT wire{};
    std::memcpy(&wire, data, sizeof(wire));
    const auto want = hasExpected ? expected : defaultVersion<T>();
    return static_cast<uint64_t>(wire) == want ? Status::Ok
                                               : Status::VersionMismatch;
  }
}
#endif

} // namespace detail

#if BITSERY_HAS_CPP26_REFLECTION
template<typename T>
class TypedWireView
{
  using TClean = detail::Raw<T>;
  static constexpr auto Count = detail::fieldCount<TClean>();

public:
  TypedWireView() = default;
  TypedWireView(const TypedWireView&) = default;
  TypedWireView& operator=(const TypedWireView&) = default;
  TypedWireView(TypedWireView&&) = default;
  TypedWireView& operator=(TypedWireView&&) = default;

  TypedWireView(const uint8_t* data,
                size_t size,
                bool hasExpected,
                uint64_t expected)
    : _data{ data }
    , _size{ size }
  {
    parse(hasExpected, expected);
  }

  bool valid() const { return _status == Status::Ok; }
  Status status() const { return _status; }
  bool versioned() const { return detail::hasVersion<TClean>(); }

  template<size_t Index>
  auto field() const -> FieldView<detail::MemberT<TClean, Index>>
  {
    static_assert(Index < Count, "typed wire field index out of range");
    using FieldT = detail::MemberT<TClean, Index>;
    detail::Cursor in{ _data, _size, Status::Ok };
    size_t pos = 0u;
    if constexpr (Index != 0u) {
      if (!detail::skipPrefix<TClean>(
            in, pos, std::make_index_sequence<Index>{})) {
        return FieldView<FieldT>{};
      }
    }
    return detail::readOne<FieldT>(in, pos);
  }

private:
  void parse(bool hasExpected, uint64_t expected)
  {
    if (DefaultConfig::Endianness != details::getSystemEndianness()) {
      _status = Status::WrongEndianness;
      return;
    }
    _status = detail::checkVersion<TClean>(_data, _size, hasExpected, expected);
    if (_status != Status::Ok)
      return;
  }

  const uint8_t* _data{};
  size_t _size{};
  Status _status{ Status::NoReflection };
};

template<typename T>
inline TypedWireView<T>
makeTypedWireView(const uint8_t* data, size_t size)
{
  return TypedWireView<T>{ data, size, false, 0u };
}

template<typename T>
inline TypedWireView<T>
makeTypedWireView(const uint8_t* data, size_t size, uint64_t expectedVersion)
{
  return TypedWireView<T>{ data, size, true, expectedVersion };
}
#else
template<typename T>
class TypedWireView
{
public:
  bool valid() const { return false; }
  Status status() const { return Status::NoReflection; }
};
#endif

} // namespace tw

namespace ext {

template<typename TAdapter, typename T>
inline size_t
serializeTypedWire(TAdapter adapter, const T& value)
{
#if BITSERY_HAS_CPP26_REFLECTION
  using TClean = typename std::remove_cv<typename std::remove_reference<T>::type>::type;
  static_assert(tw::detail::reflectedAggregate<TClean>(),
                "serializeTypedWire requires a reflected aggregate root type");
  tw::detail::writeObject(adapter, value);
  adapter.flush();
  return adapter.writtenBytesCount();
#else
  (void)adapter;
  (void)value;
  static_assert(tw::detail::AlwaysFalse<T>::value,
                "serializeTypedWire requires C++26 reflection");
  return 0u;
#endif
}

} // namespace ext
} // namespace bitsery

#endif // BITSERY_TYPED_WIRE_H
