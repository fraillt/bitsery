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
#include <bitsery/deserializer.h>
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

#if defined(__GNUC__) || defined(__clang__)
#define BITSERY_NOINLINE __attribute__((noinline))
#else
#define BITSERY_NOINLINE
#endif

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

volatile uint64_t gSink{};

void consume(uint64_t value)
{
  gSink = gSink + value;
}

uint64_t signature(const ApplicationStateRequest& v)
{
  return v.applicationState;
}

uint64_t signature(const ActivityRequest& v)
{
  return static_cast<uint64_t>(v.userId) ^ static_cast<uint64_t>(v.activityId) ^
         v.timestamp;
}

uint64_t signature(const VersionedState& v)
{
  return static_cast<uint64_t>(v.version) << 32u | v.state;
}

uint64_t signature(const StaticSample& v)
{
  return static_cast<uint64_t>(v.a) + v.b + v.data.size();
}

uint64_t signature(const DynamicSample& v)
{
  return static_cast<uint64_t>(v.id) + v.title.size() + v.payload.size() +
         v.fixed.size();
}

uint64_t signature(const KitchenSink& v)
{
  return static_cast<uint64_t>(v.id) + v.title.size() + v.payload.size() +
         v.pods.size() + v.nested.bytes.size() + v.nested.note.size();
}

template<typename T>
Buffer normalSerialize(const T& value)
{
  Buffer buf;
  const auto written = bitsery::quickSerialization(
    bitsery::OutputBufferAdapter<Buffer>{ buf }, value);
  buf.resize(written);
  return buf;
}

template<typename T>
BITSERY_NOINLINE uint64_t normalReadOnce(const Buffer& buf)
{
  T out{};
  const auto res = bitsery::quickDeserialization(
    bitsery::InputBufferAdapter<Buffer>{ buf.begin(), buf.size() }, out);
  return signature(out) + static_cast<uint64_t>(res.second);
}

template<typename T, typename Fnc>
BITSERY_NOINLINE uint64_t typedReadOnce(const Buffer& buf, Fnc fnc)
{
  auto view = bitsery::tw::makeTypedWireView<T>(buf.data(), buf.size());
  return fnc(view) + static_cast<uint64_t>(view.valid());
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

template<typename T, typename Fnc>
void benchCase(const char* name, size_t iterations, const T& value, Fnc fnc)
{
  const auto buf = normalSerialize(value);

  auto benchNormal = [&]() {
    consume(normalReadOnce<T>(buf));
  };
  auto benchTyped = [&]() {
    consume(typedReadOnce<T>(buf, fnc));
  };

  const auto quick = timeMany(iterations, benchNormal);
  const auto typed = timeMany(iterations, benchTyped);
  std::fprintf(stderr,
               "perf-typed-read-%s: quickDeserialization=%0.2fms "
               "typedWireRead=%0.2fms bytes=%zu (iters=%zu)\n",
               name,
               std::chrono::duration<double, std::milli>(quick).count(),
               std::chrono::duration<double, std::milli>(typed).count(),
               buf.size(),
               iterations);
}

} // namespace

#if BITSERY_HAS_CPP26_REFLECTION
TEST(TypedWirePerf, DISABLED_NormalBitseryVsTypedWireRead)
{
  ApplicationStateRequest app{};
  app.applicationState = 0x2Au;
  benchCase("application-state", 100'000u, app, [](auto& view) {
    return static_cast<uint64_t>(view.template field<0>().copy());
  });

  ActivityRequest activity{};
  activity.userId = -123456789;
  activity.activityId = 987654321;
  activity.timestamp = 0xAABBCCDDEEFF0011ULL;
  benchCase("activity", 50'000u, activity, [](auto& view) {
    return static_cast<uint64_t>(view.template field<0>().copy()) ^
           static_cast<uint64_t>(view.template field<1>().copy()) ^
           view.template field<2>().copy();
  });

  VersionedState versioned{};
  versioned.state = 0xCAFEBABEu;
  benchCase("versioned-state", 50'000u, versioned, [](auto& view) {
    return static_cast<uint64_t>(view.template field<0>().copy()) << 32u |
           view.template field<1>().copy();
  });

  StaticSample stat{};
  stat.a = 0xAAu;
  stat.b = 0xBBu;
  stat.data.fill(0xCCu);
  benchCase("static-sample", 50'000u, stat, [](auto& view) {
    return static_cast<uint64_t>(view.template field<0>().copy()) +
           view.template field<1>().copy() +
           view.template field<2>().bytes.size;
  });

  DynamicSample dynamic{};
  dynamic.id = 0xDEADBEEFu;
  dynamic.title = "typed dynamic payload";
  dynamic.payload.assign(200u, 0x5Au);
  dynamic.fixed = { { 0x1111u, 0x2222u, 0x3333u, 0x4444u } };
  benchCase("dynamic-sample", 5'000u, dynamic, [](auto& view) {
    return static_cast<uint64_t>(view.template field<0>().copy()) +
           view.template field<1>().bytes.size +
           view.template field<2>().bytes.size +
           view.template field<3>().bytes.size;
  });

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
  benchCase("kitchen-sink", 5'000u, kitchen, [](auto& view) {
    return static_cast<uint64_t>(view.template field<0>().copy()) +
           view.template field<1>().bytes.size +
           view.template field<2>().bytes.size +
           view.template field<3>().bytes.size +
           view.template field<4>().bytes.size;
  });
}
#endif

#undef BITSERY_NOINLINE
