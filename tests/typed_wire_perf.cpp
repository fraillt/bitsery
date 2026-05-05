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

#include <bitsery/adapter/buffer.h>
#include <bitsery/serializer.h>
#include <bitsery/typed_wire.h>
#include <bitsery/traits/array.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>
#include <gtest/gtest.h>
#include <array>
#include <chrono>
#include <cstdint>
#include <cstdio>
#include <string>
#include <utility>
#include <vector>

using Buffer = std::vector<uint8_t>;

namespace {

struct ApplicationStateRequest
{
  uint8_t applicationState{};
};

struct ActivityRequest
{
  int64_t userId{};
  int64_t activityId{};
  uint64_t timestamp{};
};

struct VersionedState
{
  uint32_t version{ 7u };
  uint32_t state{};
};

struct StaticSample
{
  uint32_t a{};
  uint32_t b{};
  std::array<uint8_t, 32> data{};
};

struct DynamicSample
{
  uint32_t id{};
  std::string title{};
  std::vector<uint8_t> payload{};
  std::array<uint16_t, 4> fixed{};
};

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
void serialize(S& s, ApplicationStateRequest& v)
{
  s.value1b(v.applicationState);
}

template<typename S>
void serialize(S& s, ActivityRequest& v)
{
  s.value8b(v.userId);
  s.value8b(v.activityId);
  s.value8b(v.timestamp);
}

template<typename S>
void serialize(S& s, VersionedState& v)
{
  s.value4b(v.version);
  s.value4b(v.state);
}

template<typename S>
void serialize(S& s, StaticSample& v)
{
  s.value4b(v.a);
  s.value4b(v.b);
  s.container1b(v.data);
}

template<typename S>
void serialize(S& s, DynamicSample& v)
{
  s.value4b(v.id);
  s.text1b(v.title, 64);
  s.container1b(v.payload, 256);
  s.container2b(v.fixed);
}

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

template<typename T>
size_t normalBytes(const T& value)
{
  Buffer buf;
  const auto written = bitsery::quickSerialization(
    bitsery::OutputBufferAdapter<Buffer>{ buf }, value);
  return written;
}

template<typename T>
size_t typedBytes(const T& value)
{
  Buffer buf;
  return bitsery::ext::serializeTypedWire(
    bitsery::OutputBufferAdapter<Buffer>{ buf }, value);
}

template<typename Fn>
std::chrono::duration<double>
timeMany(size_t iterations, Fn&& fn)
{
  const auto start = std::chrono::steady_clock::now();
  for (size_t i = 0; i < iterations; ++i)
    fn();
  return std::chrono::steady_clock::now() - start;
}

template<typename T>
void benchCase(const char* name, size_t iterations, const T& value)
{
  const auto quickBytes = normalBytes(value);
  const auto reflectedBytes = typedBytes(value);
  EXPECT_EQ(reflectedBytes, quickBytes);

  auto benchQuick = [&]() {
    Buffer buf;
    bitsery::quickSerialization(bitsery::OutputBufferAdapter<Buffer>{ buf },
                                value);
  };
  auto benchTyped = [&]() {
    Buffer buf;
    bitsery::ext::serializeTypedWire(
      bitsery::OutputBufferAdapter<Buffer>{ buf }, value);
  };

  const auto quick = timeMany(iterations, benchQuick);
  const auto typed = timeMany(iterations, benchTyped);
  std::fprintf(stderr,
               "perf-typed-%s: quickSerialization=%0.2fms "
               "typedWire=%0.2fms bytes=%zu (iters=%zu)\n",
               name,
               std::chrono::duration<double, std::milli>(quick).count(),
               std::chrono::duration<double, std::milli>(typed).count(),
               quickBytes,
               iterations);
}

} // namespace

#if BITSERY_HAS_CPP26_REFLECTION
TEST(TypedWirePerf, DISABLED_NormalBitseryVsTypedWire)
{
  ApplicationStateRequest app{};
  app.applicationState = 0x2Au;
  benchCase("application-state", 100'000u, app);

  ActivityRequest activity{};
  activity.userId = -123456789;
  activity.activityId = 987654321;
  activity.timestamp = 0xAABBCCDDEEFF0011ULL;
  benchCase("activity", 50'000u, activity);

  VersionedState versioned{};
  versioned.state = 0xCAFEBABEu;
  benchCase("versioned-state", 50'000u, versioned);

  StaticSample stat{};
  stat.a = 0xAAu;
  stat.b = 0xBBu;
  stat.data.fill(0xCCu);
  benchCase("static-sample", 50'000u, stat);

  DynamicSample dynamic{};
  dynamic.id = 0xDEADBEEFu;
  dynamic.title = "typed dynamic payload";
  dynamic.payload.assign(200u, 0x5Au);
  dynamic.fixed = { { 0x1111u, 0x2222u, 0x3333u, 0x4444u } };
  benchCase("dynamic-sample", 5'000u, dynamic);

  KitchenSink kitchen{};
  kitchen.id = 0xDEADBEEFu;
  kitchen.title = "kitchen sink payload";
  kitchen.payload.assign(200u, 0x5Au);
  kitchen.pods.resize(12);
  for (size_t i = 0; i < kitchen.pods.size(); ++i) {
    kitchen.pods[i].x = static_cast<uint32_t>(i * 17u);
    kitchen.pods[i].y = static_cast<uint16_t>(i * 3u + 1u);
    kitchen.pods[i].z = static_cast<uint16_t>(kitchen.pods[i].y + 2u);
  }
  kitchen.nested.bytes.assign(80u, 0xC3u);
  kitchen.nested.note = "nested bytes";
  benchCase("kitchen-sink", 5'000u, kitchen);
}
#endif
