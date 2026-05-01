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

#ifndef BITSERY_DETAILS_OFFSET_TABLE_SERIALIZER_H
#define BITSERY_DETAILS_OFFSET_TABLE_SERIALIZER_H

#include "serialization_common.h"
#include "serializer_shared.h"
#include "offset_table.h"

namespace bitsery { namespace ext { template<typename TContainer> class Entropy; } }
namespace bitsery { namespace ext { template<typename TBase> class BaseClass; } }
namespace bitsery { namespace ext { template<typename TBase> class VirtualBaseClass; } }
namespace bitsery { namespace ext { namespace pointer_utils {
template<template<typename> class, template<typename> class, typename>
class PointerObjectExtensionBase;
}} }

namespace bitsery {

namespace details {

template<typename Ext>
struct IsUnsupportedOffsetTableExt : std::false_type
{};

template<typename TContainer>
struct IsUnsupportedOffsetTableExt<::bitsery::ext::Entropy<TContainer>>
  : std::true_type
{};

template<template<typename> class TPtrManager,
         template<typename> class TPolyCtx,
         typename RTTI>
struct IsUnsupportedOffsetTableExt<
  ::bitsery::ext::pointer_utils::PointerObjectExtensionBase<TPtrManager,
                                                            TPolyCtx,
                                                            RTTI>>
  : std::true_type
{};

template<typename TBase>
struct IsUnsupportedOffsetTableExt<::bitsery::ext::BaseClass<TBase>>
  : std::true_type
{};

template<typename TBase>
struct IsUnsupportedOffsetTableExt<::bitsery::ext::VirtualBaseClass<TBase>>
  : std::true_type
{};

template<typename TOutputAdapter>
class OffsetTableWriteSerializer final
  : public AdapterAndContextRef<TOutputAdapter, OffsetTableWriterState>
  , public SerializerShorthand<OffsetTableWriteSerializer<TOutputAdapter>>
{
  using Base = AdapterAndContextRef<TOutputAdapter, OffsetTableWriterState>;

public:
  static_assert(!IsStreamAdapter<TOutputAdapter>::value,
                "OffsetTableWriteSerializer doesn't support stream adapters.");

  using BPEnabledType =
    OffsetTableWriteSerializer<typename TOutputAdapter::BitPackingEnabled>;
  using TConfig = typename TOutputAdapter::TConfig;
  using OffsetScope = FieldOffsetScope<TOutputAdapter>;

  using Base::Base;

  template<typename T>
  void object(const T& obj)
  {
    auto _bits_ot_field_scope =
      makeOffsetFieldScope(FieldKind::NestedTable,
                           defaultFieldFlags<T>() | FieldFlags::None,
                           0u);
    auto _bits_ot_type_scope = makeOffsetTypeScope<T>();
    SerializeFunction<OffsetTableWriteSerializer, T>::invoke(
      *this, const_cast<T&>(obj));
    auto _bits_ot_nested_idx = _bits_ot_type_scope.pop();
    if (_bits_ot_nested_idx != InvalidTableIndex)
      _bits_ot_field_scope.nestedTableIdx(_bits_ot_nested_idx);
    else
      _bits_ot_field_scope.cancel();
  }

#if BITSERY_HAS_CPP26_REFLECTION
  template<typename T>
  void reflectedObject(const T& obj)
  {
    static_assert(FieldRegistry<T>::Enabled,
                  "Reflected offset-table serialization requires a reflected "
                  "or explicit FieldRegistry.");
    auto _bits_ot_field_scope =
      makeOffsetFieldScope(FieldKind::NestedTable,
                           defaultFieldFlags<T>() | FieldFlags::None,
                           0u);
    auto _bits_ot_type_scope = makeOffsetTypeScope<T>();
    reflectedMembers(obj);
    auto _bits_ot_nested_idx = _bits_ot_type_scope.pop();
    if (_bits_ot_nested_idx != InvalidTableIndex)
      _bits_ot_field_scope.nestedTableIdx(_bits_ot_nested_idx);
    else
      _bits_ot_field_scope.cancel();
  }

#endif

  template<typename T, typename Fnc>
  void object(const T& obj, Fnc&& fnc)
  {
    auto _bits_ot_field_scope =
      makeOffsetFieldScope(FieldKind::NestedTable,
                           defaultFieldFlags<T>() | FieldFlags::None,
                           0u);
    auto _bits_ot_type_scope = makeOffsetTypeScope<T>();
    fnc(*this, const_cast<T&>(obj));
    auto _bits_ot_nested_idx = _bits_ot_type_scope.pop();
    if (_bits_ot_nested_idx != InvalidTableIndex)
      _bits_ot_field_scope.nestedTableIdx(_bits_ot_nested_idx);
    else
      _bits_ot_field_scope.cancel();
  }

  template<size_t VSIZE, typename T>
  void value(const T& v)
  {
    static_assert(IsFundamentalType<T>::value,
                  "Value must be integral, float or enum type.");
    using TValue = typename IntegralFromFundamental<T>::TValue;
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Scalar, FieldFlags::None, sizeof(TValue));
    this->_adapter.template writeBytes<VSIZE>(
      reinterpret_cast<const TValue&>(v));
  }

  template<typename Fnc>
  void enableBitPacking(Fnc&& fnc)
  {
    disableOffsetRecording();
    procEnableBitPacking(
      std::forward<Fnc>(fnc),
      std::is_same<TOutputAdapter,
                   typename TOutputAdapter::BitPackingEnabled>{},
      std::true_type{});
  }

  template<typename T, typename Ext, typename Fnc>
  void ext(const T& obj, const Ext& extension, Fnc&& fnc)
  {
    static_assert(IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportLambdaOverload,
                  "extension doesn't support overload with lambda");
    if (IsUnsupportedOffsetTableExt<Ext>::value)
      disableOffsetRecording();
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::NestedStruct,
                           FieldFlags::CopyOnly | defaultFieldFlags<T>(),
                           0u);
    extension.serialize(*this, obj, std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T, typename Ext>
  void ext(const T& obj, const Ext& extension)
  {
    static_assert(IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportValueOverload,
                  "extension doesn't support overload with `value<N>`");
    if (IsUnsupportedOffsetTableExt<Ext>::value)
      disableOffsetRecording();
    using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
    using VType = typename std::conditional<std::is_void<ExtVType>::value,
                                            DummyType,
                                            ExtVType>::type;
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::NestedStruct,
                           FieldFlags::CopyOnly | defaultFieldFlags<T>(),
                           0u);
    extension.serialize(*this,
                        obj,
                        [](OffsetTableWriteSerializer& s, VType& v) {
                          s.template value<VSIZE>(v);
                        });
  }

  template<typename T, typename Ext>
  void ext(const T& obj, const Ext& extension)
  {
    static_assert(IsExtensionTraitsDefined<Ext, T>::value,
                  "Please define ExtensionTraits");
    static_assert(traits::ExtensionTraits<Ext, T>::SupportObjectOverload,
                  "extension doesn't support overload with `object`");
    if (IsUnsupportedOffsetTableExt<Ext>::value)
      disableOffsetRecording();
    using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
    using VType = typename std::conditional<std::is_void<ExtVType>::value,
                                            DummyType,
                                            ExtVType>::type;
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::NestedStruct,
                           FieldFlags::CopyOnly | defaultFieldFlags<T>(),
                           0u);
    extension.serialize(*this,
                        obj,
                        [](OffsetTableWriteSerializer& s, VType& v) {
                          s.object(v);
                        });
  }

  void boolValue(bool v)
  {
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Scalar,
                           defaultFieldFlags<unsigned char>() |
                             FieldFlags::None,
                           1u);
    procBoolValue(v,
                  std::is_same<TOutputAdapter,
                               typename TOutputAdapter::BitPackingEnabled>{});
  }

  template<size_t VSIZE, typename T>
  void text(const T& str, size_t maxSize)
  {
    static_assert(
      IsTextTraitsDefined<T>::value,
      "Please define TextTraits or include from <bitsery/traits/...>");
    static_assert(
      traits::ContainerTraits<T>::isResizable,
      "use text(const T&) overload without `maxSize` for static container");
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Array,
                           defaultFieldFlags<
                             typename traits::ContainerTraits<T>::TValue>(),
                           sizeof(typename traits::ContainerTraits<T>::TValue));
    procText<VSIZE>(str, maxSize);
  }

  template<size_t VSIZE, typename T>
  void text(const T& str)
  {
    static_assert(
      IsTextTraitsDefined<T>::value,
      "Please define TextTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use text(const T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Array,
                           defaultFieldFlags<
                             typename traits::ContainerTraits<T>::TValue>(),
                           sizeof(typename traits::ContainerTraits<T>::TValue));
    procText<VSIZE>(str, traits::ContainerTraits<T>::size(str));
  }

  template<typename T, typename Fnc>
  void container(const T& obj, size_t maxSize, Fnc&& fnc)
  {
    static_assert(
      IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, Fnc) overload without `maxSize` for "
                  "static containers");
    auto size = traits::ContainerTraits<T>::size(obj);
    (void)maxSize;
    assert(size <= maxSize);
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Array,
                           defaultFieldFlags<
                             typename traits::ContainerTraits<T>::TValue>(),
                           sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    writeSize(this->_adapter, size);
    procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T>
  void container(const T& obj, size_t maxSize)
  {
    static_assert(
      IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(traits::ContainerTraits<T>::isResizable,
                  "use container(const T&) overload without `maxSize` for "
                  "static containers");
    static_assert(VSIZE > 0, "");
    auto size = traits::ContainerTraits<T>::size(obj);
    (void)maxSize;
    assert(size <= maxSize);
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Array,
                           defaultFieldFlags<
                             typename traits::ContainerTraits<T>::TValue>(),
                           sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    writeSize(this->_adapter, size);
    procContainer<VSIZE>(
      std::begin(obj),
      std::end(obj),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  template<typename T>
  void container(const T& obj, size_t maxSize)
  {
    static_assert(
      IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(traits::ContainerTraits<T>::isResizable,
                  "use container(const T&) overload without `maxSize` for "
                  "static containers");
    auto size = traits::ContainerTraits<T>::size(obj);
    (void)maxSize;
    assert(size <= maxSize);
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Array,
                           defaultFieldFlags<
                             typename traits::ContainerTraits<T>::TValue>(),
                           sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    writeSize(this->_adapter, size);
    procContainer(std::begin(obj), std::end(obj));
  }

  template<typename T,
           typename Fnc,
           typename std::enable_if<!std::is_integral<Fnc>::value>::type* =
             nullptr>
  void container(const T& obj, Fnc&& fnc)
  {
    static_assert(
      IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, size_t, Fnc) overload with "
                  "`maxSize` for dynamic containers");
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Array,
                           defaultFieldFlags<
                             typename traits::ContainerTraits<T>::TValue>(),
                           sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
  }

  template<size_t VSIZE, typename T>
  void container(const T& obj)
  {
    static_assert(
      IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    static_assert(VSIZE > 0, "");
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Array,
                           defaultFieldFlags<
                             typename traits::ContainerTraits<T>::TValue>(),
                           sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    procContainer<VSIZE>(
      std::begin(obj),
      std::end(obj),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  template<typename T>
  void container(const T& obj)
  {
    static_assert(
      IsContainerTraitsDefined<T>::value,
      "Please define ContainerTraits or include from <bitsery/traits/...>");
    static_assert(!traits::ContainerTraits<T>::isResizable,
                  "use container(const T&, size_t) overload with `maxSize` for "
                  "dynamic containers");
    [[maybe_unused]] auto _bits_ot_scope =
      makeOffsetFieldScope(FieldKind::Array,
                           defaultFieldFlags<
                             typename traits::ContainerTraits<T>::TValue>(),
                           sizeof(typename traits::ContainerTraits<T>::TValue));
    [[maybe_unused]] auto _bits_ot_pause = pauseOffsetRecording();
    procContainer(std::begin(obj), std::end(obj));
  }

  size_t finalize()
  {
    return writeTablesAndTrailer(
      this->_adapter, this->_context, this->_adapter.writtenBytesCount());
  }

  OffsetTableWriterState& state() { return this->_context; }
  const OffsetTableWriterState& state() const { return this->_context; }

  void object(const DummyType&) {}

  template<size_t VSIZE>
  void value(const DummyType&)
  {
  }

private:
#if BITSERY_HAS_CPP26_REFLECTION
  template<typename T>
  void reflectedMembers(const T& obj)
  {
    constexpr auto ctx = std::meta::access_context::unchecked();
    static constexpr auto members = std::define_static_array(
      std::meta::nonstatic_data_members_of(^^T, ctx));
    template for (constexpr auto member : members) {
      reflectedField(obj.[:member:]);
    }
  }

  template<typename T>
  void reflectedField(const T& field)
  {
    using RawT =
      typename std::remove_cv<typename std::remove_reference<T>::type>::type;
    if constexpr (IsTextTraitsDefined<RawT>::value) {
      constexpr auto valueSize =
        sizeof(typename traits::ContainerTraits<RawT>::TValue);
      if constexpr (traits::ContainerTraits<RawT>::isResizable) {
        text<valueSize>(field, traits::TextTraits<RawT>::length(field));
      } else {
        text<valueSize>(field);
      }
    } else if constexpr (IsContainerTraitsDefined<RawT>::value) {
      constexpr auto valueSize =
        sizeof(typename traits::ContainerTraits<RawT>::TValue);
      if constexpr (traits::ContainerTraits<RawT>::isResizable) {
        container<valueSize>(field, traits::ContainerTraits<RawT>::size(field));
      } else {
        container<valueSize>(field);
      }
    } else if constexpr (IsFundamentalType<RawT>::value) {
      value<sizeof(RawT)>(field);
    } else if constexpr (FieldRegistry<RawT>::Enabled) {
      reflectedObject(field);
    } else {
      object(field);
    }
  }

#endif

  template<size_t VSIZE, typename It>
  void procContainer(It first, It last, std::false_type)
  {
    for (; first != last; ++first)
      value<VSIZE>(*first);
  }

  template<size_t VSIZE, typename It>
  void procContainer(It first, It last, std::true_type)
  {
    using TValue = typename std::decay<decltype(*first)>::type;
    using TIntegral = typename IntegralFromFundamental<TValue>::TValue;
    if (first != last)
      this->_adapter.template writeBuffer<VSIZE>(
        reinterpret_cast<const TIntegral*>(&(*first)),
        static_cast<size_t>(std::distance(first, last)));
  }

  template<typename It, typename Fnc>
  void procContainer(It first, It last, Fnc fnc)
  {
    using TValue = typename std::decay<decltype(*first)>::type;
    for (; first != last; ++first) {
      fnc(*this, const_cast<TValue&>(*first));
    }
  }

  template<size_t VSIZE, typename T>
  void procText(const T& str, size_t maxSize)
  {
    const size_t length = traits::TextTraits<T>::length(str);
    (void)maxSize;
    assert((length + (traits::TextTraits<T>::addNUL ? 1u : 0u)) <= maxSize);
    writeSize(this->_adapter, length);
    auto begin = std::begin(str);
    using diff_t =
      typename std::iterator_traits<decltype(begin)>::difference_type;
    procContainer<VSIZE>(
      begin,
      std::next(begin, static_cast<diff_t>(length)),
      std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
  }

  template<typename It>
  void procContainer(It first, It last)
  {
    for (; first != last; ++first)
      object(*first);
  }

  void procBoolValue(bool v, std::true_type)
  {
    this->_adapter.writeBits(static_cast<unsigned char>(v ? 1 : 0), 1);
  }

  void procBoolValue(bool v, std::false_type)
  {
    this->_adapter.template writeBytes<1>(
      static_cast<unsigned char>(v ? 1 : 0));
  }

  template<typename Fnc, typename HasContext>
  void procEnableBitPacking(const Fnc& fnc, std::true_type, HasContext)
  {
    fnc(*this);
  }

  template<typename Fnc>
  void procEnableBitPacking(const Fnc& fnc, std::false_type, std::true_type)
  {
    BPEnabledType ser{ this->_context, this->_adapter };
    fnc(ser);
  }

  struct OffsetRecordingPause
  {
    OffsetTableWriterState* state{};
    bool prev{};

    explicit OffsetRecordingPause(OffsetTableWriterState* s)
      : state{ s }
    {
      if (state) {
        prev = state->enabled;
        state->enabled = false;
      }
    }

    OffsetRecordingPause(const OffsetRecordingPause&) = delete;
    OffsetRecordingPause& operator=(const OffsetRecordingPause&) = delete;
    OffsetRecordingPause(OffsetRecordingPause&& other) noexcept
      : state{ other.state }
      , prev{ other.prev }
    {
      other.state = nullptr;
    }
    OffsetRecordingPause& operator=(OffsetRecordingPause&& other) noexcept
    {
      if (this != std::addressof(other)) {
        restore();
        state = other.state;
        prev = other.prev;
        other.state = nullptr;
      }
      return *this;
    }

    ~OffsetRecordingPause() { restore(); }

  private:
    void restore()
    {
      if (state) {
        state->enabled = prev;
        state = nullptr;
      }
    }
  };

  OffsetRecordingPause pauseOffsetRecording()
  {
    return OffsetRecordingPause{ std::addressof(this->_context) };
  }

  struct OffsetTypeScope
  {
    OffsetTableWriterState* state{};
    TOutputAdapter* adapter{};
    OffsetTableWriterState::Frame* frame{};

    OffsetTableWriterState::Frame::TableIndex pop()
    {
      if (!frame || !state || !state->enabled)
        return InvalidTableIndex;
      size_t payloadEnd = 0;
      if constexpr (HasCurrentWritePos<TOutputAdapter>::value) {
        payloadEnd = adapter->currentWritePos();
      } else if constexpr (HasWrittenBytesCount<TOutputAdapter>::value) {
        payloadEnd = adapter->writtenBytesCount();
      }
      return popOffsetFrame(*state, payloadEnd);
    }
  };

  template<typename T>
  OffsetTypeScope makeOffsetTypeScope()
  {
    OffsetTypeScope res{};
    auto& st = this->_context;
    if (!st.enabled)
      return res;
    if (TConfig::Endianness != getSystemEndianness()) {
      st.enabled = false;
      return res;
    }
    if (!FieldRegistry<T>::Enabled) {
      st.enabled = false;
      return res;
    }
    if (FieldRegistry<T>::FieldCount > 0 &&
        FieldRegistry<T>::entries() == nullptr) {
      st.enabled = false;
      return res;
    }
    res.state = std::addressof(st);
    res.adapter = std::addressof(this->_adapter);
    res.frame = pushOffsetFrame<T>(st);
    return res;
  }

  OffsetScope makeOffsetFieldScope(FieldKind kind,
                                   FieldFlags flags,
                                   uint32_t elemSize)
  {
    auto& st = this->_context;
    if (!st.enabled)
      return {};
    auto* frame = currentOffsetFrame(st);
    if (!frame)
      return {};
    auto* info = nextField(*frame);
    if (!info) {
      disableCurrentFrame(st);
      return {};
    }
    size_t begin = 0;
    if constexpr (HasCurrentWritePos<TOutputAdapter>::value) {
      begin = this->_adapter.currentWritePos();
    } else if constexpr (HasWrittenBytesCount<TOutputAdapter>::value) {
      begin = this->_adapter.writtenBytesCount();
    }
    if (st.captureEnabled && frame->captured)
      closeCapturedFrameFieldAt(st, *frame, begin);
    if (frame->hasAligned && hasFlag(info->flags, FieldFlags::Aligned) &&
        info->align > 1 && HasCurrentWritePos<TOutputAdapter>::value) {
      const auto padding =
        static_cast<size_t>((info->align - (begin % info->align)) %
                            info->align);
      if (padding > 0)
        this->_adapter.currentWritePos(begin + padding);
      begin += padding;
    }
    if (info->kind != kind) {
      disableCurrentFrame(st);
      return {};
    }
    auto mergedFlags = info->flags | flags;
    if (st.captureEnabled && frame->captured) {
      assert(begin <= std::numeric_limits<uint32_t>::max());
      assert(frame->captureTableIdx < st.captureTableCount);
      RecordedEntry entry{};
      entry.fieldId = info->id;
      entry.kind = info->kind;
      entry.flags = mergedFlags;
      entry.payloadOff = static_cast<uint32_t>(begin);
      entry.size = 0u;
      entry.elemSize = elemSize;
      entry.nestedTableIdx = InvalidRecordedTableIndex;
      st.captureTables[frame->captureTableIdx].entries.push_back(entry);
      return {};
    }
    return FieldOffsetScope<TOutputAdapter>(
      st.recorder, this->_adapter, info->id, info->kind, mergedFlags, elemSize);
  }

  void disableOffsetRecording()
  {
    disableCurrentFrame(this->_context);
  }
};

} // namespace details

} // namespace bitsery

#endif // BITSERY_DETAILS_OFFSET_TABLE_SERIALIZER_H
