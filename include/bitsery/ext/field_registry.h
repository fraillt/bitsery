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

#ifndef BITSERY_EXT_FIELD_REGISTRY_H
#define BITSERY_EXT_FIELD_REGISTRY_H

#include "../details/offset_table.h"
#include <utility>

namespace bitsery {

namespace ext {

template<typename T, typename Member>
constexpr size_t
offsetOf(Member T::*member)
{
  return reinterpret_cast<size_t>(
    &(reinterpret_cast<T const volatile*>(0)->*member));
}

template<typename T, typename Member>
constexpr details::FieldInfo
makeField(uint16_t id,
          Member T::*member,
          details::FieldKind kind,
          details::FieldFlags flags = details::FieldFlags::None)
{
  uint16_t nestedCount = 0;
  uint16_t nestedVersion = 0;
  const details::FieldInfo* nestedEntries = nullptr;
  auto effectiveFlags = flags | details::defaultFieldFlags<Member>();
  if (kind == details::FieldKind::NestedTable) {
    if (!details::FieldRegistry<Member>::Enabled)
      effectiveFlags |= details::FieldFlags::CopyOnly;
    nestedEntries = details::FieldRegistry<Member>::entries();
    nestedCount = static_cast<uint16_t>(
      details::FieldRegistry<Member>::FieldCount);
    nestedVersion = details::FieldRegistry<Member>::TypeVersion;
  }
  return details::makeField(
    id,
    kind,
    effectiveFlags,
    offsetOf(member),
    sizeof(Member),
    alignof(Member),
    nestedCount,
    nestedVersion,
    nestedEntries);
}

template<uint16_t TypeVer, size_t N, const std::array<details::FieldInfo, N>* Ptr>
struct RegistryPtr
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = TypeVer;
  static constexpr size_t FieldCount = N;
  static const details::FieldInfo* entries()
  {
    return Ptr->data();
  }
};

template<uint16_t TypeVer = 0, size_t N>
inline auto
makeRegistry(const std::array<details::FieldInfo, N>& arr)
{
  static const std::array<details::FieldInfo, N> values = arr;
  return RegistryPtr<TypeVer, N, &values>{};
}

}

}

#endif // BITSERY_EXT_FIELD_REGISTRY_H
