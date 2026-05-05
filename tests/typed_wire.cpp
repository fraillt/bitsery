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
#include <cstdint>
#include <string>
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

struct PrefixV1
{
  uint8_t a{};
  uint16_t b{};
};

struct PrefixV2
{
  uint8_t a{};
  uint16_t b{};
  uint32_t c{};
};

struct DynamicRequest
{
  uint32_t id{};
  std::string title{};
  std::vector<uint8_t> payload{};
  uint16_t tail{};
};

struct Nested
{
  uint32_t a{};
  uint16_t b{};
};

struct Parent
{
  uint8_t tag{};
  Nested nested{};
  uint32_t tail{};
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
void serialize(S& s, PrefixV1& v)
{
  s.value1b(v.a);
  s.value2b(v.b);
}

template<typename S>
void serialize(S& s, PrefixV2& v)
{
  s.value1b(v.a);
  s.value2b(v.b);
  s.value4b(v.c);
}

template<typename S>
void serialize(S& s, DynamicRequest& v)
{
  s.value4b(v.id);
  s.text1b(v.title, 64);
  s.container1b(v.payload, 64);
  s.value2b(v.tail);
}

template<typename S>
void serialize(S& s, Nested& v)
{
  s.value4b(v.a);
  s.value2b(v.b);
}

template<typename S>
void serialize(S& s, Parent& v)
{
  s.value1b(v.tag);
  s.object(v.nested);
  s.value4b(v.tail);
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

} // namespace

#if BITSERY_HAS_CPP26_REFLECTION
namespace {

template<typename T>
void expectReadable(const T& value)
{
  const auto buf = normalSerialize(value);
  auto view = bitsery::tw::makeTypedWireView<T>(buf.data(), buf.size());
  EXPECT_TRUE(view.valid());
}

} // namespace

TEST(TypedWire, ReadsNormalBitserySerializedBytes)
{
  ApplicationStateRequest app{};
  app.applicationState = 0x2Au;
  expectReadable(app);

  ActivityRequest activity{};
  activity.userId = -123456789;
  activity.activityId = 987654321;
  activity.timestamp = 0xAABBCCDDEEFF0011ULL;
  expectReadable(activity);

  VersionedState versioned{};
  versioned.state = 0xCAFEBABEu;
  expectReadable(versioned);

  PrefixV2 prefix{};
  prefix.a = 0x11u;
  prefix.b = 0x2233u;
  prefix.c = 0x44556677u;
  expectReadable(prefix);

  DynamicRequest dynamic{};
  dynamic.id = 0xDEADBEEFu;
  dynamic.title = "typed";
  dynamic.payload = { 9u, 7u, 5u, 3u, 1u };
  dynamic.tail = 0xCAFEu;
  expectReadable(dynamic);

  Parent parent{};
  parent.tag = 0x7Fu;
  parent.nested.a = 0xAABBCCDDu;
  parent.nested.b = 0xEEFFu;
  parent.tail = 0x11223344u;
  expectReadable(parent);
}

TEST(TypedWire, UnversionedTinyMessageHasNoHeader)
{
  ApplicationStateRequest value{};
  value.applicationState = 0x2Au;

  const auto buf = normalSerialize(value);
  ASSERT_EQ(buf.size(), 1u);
  EXPECT_EQ(buf[0], value.applicationState);

  auto view =
    bitsery::tw::makeTypedWireView<ApplicationStateRequest>(buf.data(), buf.size());
  ASSERT_TRUE(view.valid());
  EXPECT_FALSE(view.versioned());

  auto state = view.field<0>();
  ASSERT_TRUE(state.present);
  ASSERT_EQ(state.status, bitsery::tw::Status::Ok);
  ASSERT_NE(state.value, nullptr);
  EXPECT_EQ(*state.value, value.applicationState);
}

TEST(TypedWire, FixedActivityRequestIsPayloadOnly)
{
  ActivityRequest value{};
  value.userId = -123456789;
  value.activityId = 987654321;
  value.timestamp = 0xAABBCCDDEEFF0011ULL;

  const auto buf = normalSerialize(value);
  ASSERT_EQ(buf.size(), 24u);

  auto view =
    bitsery::tw::makeTypedWireView<ActivityRequest>(buf.data(), buf.size());
  ASSERT_TRUE(view.valid());

  auto userId = view.field<0>();
  auto activityId = view.field<1>();
  auto timestamp = view.field<2>();
  ASSERT_EQ(userId.status, bitsery::tw::Status::Ok);
  ASSERT_EQ(activityId.status, bitsery::tw::Status::Ok);
  ASSERT_EQ(timestamp.status, bitsery::tw::Status::Ok);
  ASSERT_NE(userId.value, nullptr);
  ASSERT_NE(activityId.value, nullptr);
  ASSERT_NE(timestamp.value, nullptr);
  EXPECT_EQ(*userId.value, value.userId);
  EXPECT_EQ(*activityId.value, value.activityId);
  EXPECT_EQ(*timestamp.value, value.timestamp);
}

TEST(TypedWire, VersionFieldIsPayloadAndMismatchRejects)
{
  VersionedState value{};
  value.state = 0xCAFEBABEu;

  const auto buf = normalSerialize(value);
  ASSERT_EQ(buf.size(), 8u);

  auto defaultExpected =
    bitsery::tw::makeTypedWireView<VersionedState>(buf.data(), buf.size());
  ASSERT_TRUE(defaultExpected.valid());
  EXPECT_TRUE(defaultExpected.versioned());
  EXPECT_EQ(defaultExpected.field<0>().copy(), value.version);
  EXPECT_EQ(defaultExpected.field<1>().copy(), value.state);

  auto explicitExpected =
    bitsery::tw::makeTypedWireView<VersionedState>(buf.data(), buf.size(), 7u);
  EXPECT_TRUE(explicitExpected.valid());

  auto mismatch =
    bitsery::tw::makeTypedWireView<VersionedState>(buf.data(), buf.size(), 8u);
  EXPECT_FALSE(mismatch.valid());
  EXPECT_EQ(mismatch.status(), bitsery::tw::Status::VersionMismatch);
}

TEST(TypedWire, UnversionedReadIsAppendOnlyCompatible)
{
  PrefixV2 newer{};
  newer.a = 0x11u;
  newer.b = 0x2233u;
  newer.c = 0x44556677u;

  const auto newerBuf = normalSerialize(newer);
  ASSERT_EQ(newerBuf.size(), 7u);

  auto oldView =
    bitsery::tw::makeTypedWireView<PrefixV1>(newerBuf.data(), newerBuf.size());
  ASSERT_TRUE(oldView.valid());
  EXPECT_EQ(oldView.field<0>().copy(), newer.a);
  EXPECT_EQ(oldView.field<1>().copy(), newer.b);

  PrefixV1 older{};
  older.a = 0x44u;
  older.b = 0x5566u;
  const auto olderBuf = normalSerialize(older);

  auto newView =
    bitsery::tw::makeTypedWireView<PrefixV2>(olderBuf.data(), olderBuf.size());
  ASSERT_TRUE(newView.valid());
  EXPECT_EQ(newView.field<0>().copy(), older.a);
  EXPECT_EQ(newView.field<1>().copy(), older.b);
  auto appended = newView.field<2>();
  EXPECT_FALSE(appended.present);
  EXPECT_EQ(appended.status, bitsery::tw::Status::Absent);
}

TEST(TypedWire, DynamicFieldsUseInlineLengthsOnly)
{
  DynamicRequest value{};
  value.id = 0xDEADBEEFu;
  value.title = "typed";
  value.payload = { 9u, 7u, 5u, 3u, 1u };
  value.tail = 0xCAFEu;

  const auto buf = normalSerialize(value);
  ASSERT_EQ(buf.size(),
            sizeof(value.id) + 1u + value.title.size() + 1u +
              value.payload.size() + sizeof(value.tail));

  auto view =
    bitsery::tw::makeTypedWireView<DynamicRequest>(buf.data(), buf.size());
  ASSERT_TRUE(view.valid());
  EXPECT_EQ(view.field<0>().copy(), value.id);

  auto title = view.field<1>();
  ASSERT_TRUE(title.present);
  EXPECT_EQ(std::string(reinterpret_cast<const char*>(title.bytes.data),
                        title.bytes.size),
            value.title);

  auto payload = view.field<2>();
  ASSERT_TRUE(payload.present);
  EXPECT_EQ(std::vector<uint8_t>(payload.bytes.begin(), payload.bytes.end()),
            value.payload);

  EXPECT_EQ(view.field<3>().copy(), value.tail);
}

TEST(TypedWire, NestedFieldsAreParsedWithoutTables)
{
  Parent value{};
  value.tag = 0x7Fu;
  value.nested.a = 0xAABBCCDDu;
  value.nested.b = 0xEEFFu;
  value.tail = 0x11223344u;

  const auto buf = normalSerialize(value);
  ASSERT_EQ(buf.size(), 11u);

  auto view = bitsery::tw::makeTypedWireView<Parent>(buf.data(), buf.size());
  ASSERT_TRUE(view.valid());
  EXPECT_EQ(view.field<0>().copy(), value.tag);
  EXPECT_EQ(view.field<2>().copy(), value.tail);

  auto nested = view.field<1>();
  ASSERT_TRUE(nested.present);
  EXPECT_EQ(nested.bytes.size, 6u);
}
#else
TEST(TypedWire, ReflectionUnavailable)
{
  auto view = bitsery::tw::TypedWireView<ApplicationStateRequest>{};
  EXPECT_FALSE(view.valid());
  EXPECT_EQ(view.status(), bitsery::tw::Status::NoReflection);
  EXPECT_FALSE(view.versioned());

  auto factoryView =
    bitsery::tw::makeTypedWireView<ApplicationStateRequest>(nullptr, 0u);
  EXPECT_FALSE(factoryView.valid());
  EXPECT_EQ(factoryView.status(), bitsery::tw::Status::NoReflection);
  EXPECT_FALSE(factoryView.versioned());

  auto versionedFactoryView =
    bitsery::tw::makeTypedWireView<VersionedState>(nullptr, 0u, 7u);
  EXPECT_FALSE(versionedFactoryView.valid());
  EXPECT_EQ(versionedFactoryView.status(), bitsery::tw::Status::NoReflection);
  EXPECT_FALSE(versionedFactoryView.versioned());
}
#endif
