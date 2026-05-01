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
#include <bitsery/adapter/buffer.h>
#include <bitsery/details/offset_table.h>
#include <bitsery/ext/offset_table.h>
#include <bitsery/ext/field_registry.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

using Buffer = std::vector<uint8_t>;

namespace {

struct Sample
{
  uint32_t a{};
  uint32_t b{};
  std::array<uint8_t, 32> data{};
};

struct ReflectedSample
{
  uint32_t a{};
  uint32_t b{};
  std::array<uint8_t, 32> data{};
};

struct ReflectedOnlySample
{
  uint32_t a{};
  uint32_t b{};
  std::array<uint8_t, 32> data{};
};

struct ReflectedDynamicNested
{
  std::vector<uint8_t> bytes{};
  std::string note{};
  uint32_t tail{};
};

struct ReflectedDynamic
{
  uint32_t id{};
  std::string title{};
  std::vector<uint8_t> payload{};
  ReflectedDynamicNested nested{};
  std::array<uint16_t, 4> fixed{};
};

struct SerializerDynamicNested
{
  std::vector<uint8_t> bytes{};
  std::string note{};
  uint32_t tail{};
};

struct SerializerDynamic
{
  uint32_t id{};
  std::string title{};
  std::vector<uint8_t> payload{};
  SerializerDynamicNested nested{};
  std::array<uint16_t, 4> fixed{};
};

template<typename S>
void serialize(S& s, Sample& v)
{
  s.value4b(v.a);
  s.value4b(v.b);
  s.container1b(v.data);
}

template<typename S>
void serialize(S& s, ReflectedSample& v)
{
  s.value4b(v.a);
  s.value4b(v.b);
  s.container1b(v.data);
}

template<typename S>
void serialize(S& s, SerializerDynamicNested& v)
{
  s.container1b(v.bytes, 128);
  s.text1b(v.note, 64);
  s.value4b(v.tail);
}

template<typename S>
void serialize(S& s, SerializerDynamic& v)
{
  s.value4b(v.id);
  s.text1b(v.title, 64);
  s.container1b(v.payload, 256);
  s.object(v.nested);
  s.container2b(v.fixed);
}

struct Pod
{
  uint32_t x{};
  uint16_t y{};
  uint16_t z{};
};

struct Nested
{
  std::vector<uint8_t> bytes{};
  std::string note{};
};

struct KitchenSink
{
  uint32_t id{};
  std::string title{};
  std::vector<uint8_t> payload{};
  std::vector<Pod> pods{};
  Nested nested{};
};

template<typename S>
void serialize(S& s, Pod& v)
{
  s.value4b(v.x);
  s.value2b(v.y);
  s.value2b(v.z);
}

template<typename S>
void serialize(S& s, Nested& v)
{
  s.container1b(v.bytes, 128);
  s.text1b(v.note, 64);
}

template<typename S>
void serialize(S& s, KitchenSink& v)
{
  s.value4b(v.id);
  s.text1b(v.title, 64);
  s.container1b(v.payload, 256);
  s.container(
    v.pods, static_cast<size_t>(32), [](S& ser, Pod& p) { ser.object(p); });
  s.object(v.nested);
}

} // namespace

namespace bitsery { namespace details {

template<>
struct FieldRegistry<Sample>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 3;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<Sample>(1, &Sample::a, FieldKind::Scalar),
    ext::makeField<Sample>(2, &Sample::b, FieldKind::Scalar),
    ext::makeField<Sample>(3, &Sample::data, FieldKind::Array)
  };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

template<>
struct FieldRegistry<Pod>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 3;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<Pod>(1, &Pod::x, FieldKind::Scalar),
    ext::makeField<Pod>(2, &Pod::y, FieldKind::Scalar),
    ext::makeField<Pod>(3, &Pod::z, FieldKind::Scalar)
  };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

template<>
struct FieldRegistry<Nested>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 2;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<Nested>(
      1, &Nested::bytes, FieldKind::Array, FieldFlags::CopyOnly),
    ext::makeField<Nested>(
      2, &Nested::note, FieldKind::Array, FieldFlags::CopyOnly)
  };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

template<>
struct FieldRegistry<KitchenSink>
{
  static constexpr bool Enabled = true;
  static constexpr uint16_t TypeVersion = 0;
  static constexpr size_t FieldCount = 5;
  static inline const std::array<FieldInfo, FieldCount> Fields{
    ext::makeField<KitchenSink>(1, &KitchenSink::id, FieldKind::Scalar),
    ext::makeField<KitchenSink>(
      2, &KitchenSink::title, FieldKind::Array, FieldFlags::CopyOnly),
    ext::makeField<KitchenSink>(
      3, &KitchenSink::payload, FieldKind::Array, FieldFlags::CopyOnly),
    ext::makeField<KitchenSink>(
      4, &KitchenSink::pods, FieldKind::Array, FieldFlags::CopyOnly),
    ext::makeField<KitchenSink>(
      5, &KitchenSink::nested, FieldKind::NestedTable)
  };
  static const FieldInfo* entries()
  {
    return Fields.data();
  }
};

}} // namespace bitsery::details

namespace {

template<typename Fn>
std::chrono::duration<double>
timeMany(size_t iterations, Fn&& fn)
{
  const auto start = std::chrono::steady_clock::now();
  for (size_t i = 0; i < iterations; ++i) {
    fn();
  }
  const auto end = std::chrono::steady_clock::now();
  return end - start;
}

} // namespace

TEST(OffsetTablePerf, DISABLED_SerializeBaselineVsOffsetTable)
{
  const size_t iterations = 50'000;
  Sample value{};
  value.a = 0xAAu;
  value.b = 0xBBu;
  value.data.fill(0xCCu);

  auto benchQuick = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    bitsery::Serializer<decltype(adapter)> ser{ std::move(adapter) };
    ser.object(value);
    ser.adapter().flush();
  };

  auto benchOffsetEnabled = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    // registry is enabled for Sample, so this builds an offset table
    bitsery::ext::serializeWithOffsetTable(std::move(adapter), value);
  };

  const auto quick = timeMany(iterations, benchQuick);
  const auto offsetEnabled = timeMany(iterations, benchOffsetEnabled);

  const auto quickMs = std::chrono::duration<double, std::milli>(quick).count();
  const auto offsetMs =
    std::chrono::duration<double, std::milli>(offsetEnabled).count();

  std::fprintf(stderr,
               "perf: quickSerialization=%0.2fms offsetTableEnabled=%0.2fms "
               "(iters=%zu)\n",
               quickMs,
               offsetMs,
               iterations);
}

#if BITSERY_HAS_CPP26_REFLECTION
TEST(OffsetTablePerf, DISABLED_ReflectedSerializeVsOffsetTable)
{
  const size_t iterations = 50'000;
  ReflectedSample value{};
  value.a = 0xAAu;
  value.b = 0xBBu;
  value.data.fill(0xCCu);
  ReflectedOnlySample reflectedOnlyValue{};
  reflectedOnlyValue.a = value.a;
  reflectedOnlyValue.b = value.b;
  reflectedOnlyValue.data = value.data;

  auto benchQuick = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    bitsery::Serializer<decltype(adapter)> ser{ std::move(adapter) };
    ser.object(value);
    ser.adapter().flush();
  };

  auto benchOffsetEnabled = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    bitsery::ext::serializeWithOffsetTable(std::move(adapter), value);
  };

  auto benchReflectedOffset = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    bitsery::ext::reflectSerializeWithOffsetTable(std::move(adapter), value);
  };

  auto benchReflectedDirectOffset = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    bitsery::ext::reflectSerializeWithOffsetTable(
      std::move(adapter), reflectedOnlyValue);
  };

  const auto quick = timeMany(iterations, benchQuick);
  const auto offsetEnabled = timeMany(iterations, benchOffsetEnabled);
  const auto reflectedOffset = timeMany(iterations, benchReflectedOffset);
  const auto reflectedDirectOffset =
    timeMany(iterations, benchReflectedDirectOffset);

  const auto quickMs = std::chrono::duration<double, std::milli>(quick).count();
  const auto offsetMs =
    std::chrono::duration<double, std::milli>(offsetEnabled).count();
  const auto reflectedMs =
    std::chrono::duration<double, std::milli>(reflectedOffset).count();
  const auto reflectedDirectMs =
    std::chrono::duration<double, std::milli>(reflectedDirectOffset).count();

  std::fprintf(stderr,
               "perf-reflect: quickSerialization=%0.2fms "
               "offsetTableRegistry=%0.2fms reflectedOffsetTable=%0.2fms "
               "reflectedDirectOffsetTable=%0.2fms "
               "(iters=%zu)\n",
               quickMs,
               offsetMs,
               reflectedMs,
               reflectedDirectMs,
               iterations);
}

TEST(OffsetTablePerf, DISABLED_ReflectedDynamicGeneratedVsGeneric)
{
  const size_t iterations = 5'000;
  ReflectedDynamic generatedValue{};
  generatedValue.id = 0xDEADBEEFu;
  generatedValue.title = "generated reflected dynamic";
  generatedValue.payload.assign(200, 0x5Au);
  generatedValue.nested.bytes.assign(80, 0xC3u);
  generatedValue.nested.note = "nested reflected bytes";
  generatedValue.nested.tail = 0xAABBCCDDu;
  generatedValue.fixed = { { 0x1111u, 0x2222u, 0x3333u, 0x4444u } };

  SerializerDynamic genericValue{};
  genericValue.id = generatedValue.id;
  genericValue.title = generatedValue.title;
  genericValue.payload = generatedValue.payload;
  genericValue.nested.bytes = generatedValue.nested.bytes;
  genericValue.nested.note = generatedValue.nested.note;
  genericValue.nested.tail = generatedValue.nested.tail;
  genericValue.fixed = generatedValue.fixed;

  auto benchGeneric = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    bitsery::ext::reflectSerializeWithOffsetTable(
      std::move(adapter), genericValue);
  };

  auto benchGenerated = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    bitsery::ext::reflectSerializeWithOffsetTable(
      std::move(adapter), generatedValue);
  };

  const auto generic = timeMany(iterations, benchGeneric);
  const auto generated = timeMany(iterations, benchGenerated);

  const auto genericMs =
    std::chrono::duration<double, std::milli>(generic).count();
  const auto generatedMs =
    std::chrono::duration<double, std::milli>(generated).count();

  std::fprintf(stderr,
               "perf-reflect-dynamic: genericSerializerOffsetTable=%0.2fms "
               "generatedReflectedOffsetTable=%0.2fms (iters=%zu)\n",
               genericMs,
               generatedMs,
               iterations);
}
#endif

TEST(OffsetTablePerf, DISABLED_KitchenSinkVsOffsetTable)
{
  const size_t iterations = 5'000;
  KitchenSink value{};
  value.id = 0xDEADBEEFu;
  value.title = "kitchen sink payload";
  value.payload.assign(200, 0x5Au);
  value.pods.resize(12);
  for (size_t i = 0; i < value.pods.size(); ++i) {
    value.pods[i].x = static_cast<uint32_t>(i * 17u);
    value.pods[i].y = static_cast<uint16_t>(i * 3u + 1u);
    value.pods[i].z = static_cast<uint16_t>(value.pods[i].y + 2u);
  }
  value.nested.bytes.assign(80, 0xC3u);
  value.nested.note = "nested bytes";

  auto benchQuick = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    bitsery::Serializer<decltype(adapter)> ser{ std::move(adapter) };
    ser.object(value);
    ser.adapter().flush();
  };

  auto benchOffsetEnabled = [&]() {
    Buffer buf;
    auto adapter = bitsery::OutputBufferAdapter<Buffer>{ buf };
    bitsery::ext::serializeWithOffsetTable(std::move(adapter), value);
  };

  const auto quick = timeMany(iterations, benchQuick);
  const auto offsetEnabled = timeMany(iterations, benchOffsetEnabled);

  const auto quickMs = std::chrono::duration<double, std::milli>(quick).count();
  const auto offsetMs =
    std::chrono::duration<double, std::milli>(offsetEnabled).count();

  std::fprintf(stderr,
               "perf-kitchen: quickSerialization=%0.2fms "
               "offsetTableEnabled=%0.2fms (iters=%zu)\n",
               quickMs,
               offsetMs,
               iterations);
}
