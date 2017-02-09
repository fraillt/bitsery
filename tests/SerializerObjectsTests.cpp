//
// Created by fraillt on 17.1.31.
//

#include <gmock/gmock.h>
#include <Deserializer.h>
#include <BufferReader.h>
#include "BufferWriter.h"
#include "Serializer.h"

#include "DeltaSerializer.h"
#include "DeltaDeserializer.h"

#include <list>
#include <array>

using testing::Eq;
using testing::StrEq;
using testing::ContainerEq;

struct X {
	X() {};
	X(int v) :x{ v } {}
	std::string s{};
	int x{};
	bool operator ==(const X& r) const {
		return r.x == x && r.s == s;
	}
};


struct Y {
	int y{};
	int carr[3];
	std::array<int, 3> arr;
	std::vector<X> vx;
	std::string s;
};
struct Z { X x{}; Y y{}; };


SERIALIZE(Z)
{
	s.object(o.x);
	s.object(o.y);
	return s;
}

SERIALIZE(X)
{
	return s.value(o.x)
		.text(o.s);
}

SERIALIZE(Y)
{
	auto writeInt = [&s](auto& v) { s.template value<4>(v); };
	s.text(o.s);
	s.template value<4>(o.y);
	s.array(o.arr, writeInt);
	s.array(o.carr, writeInt);
	s.container(o.vx, [&s](auto& v) { s.object(v); });
	return s;
}


TEST(Serializer, GeneralConceptTest) {
	//std::string buf;
	std::vector<uint8_t> buf{};
	BufferWriter bw{ buf };
	Serializer<BufferWriter> ser(bw);
	Y y{};
	y.y = 3423;
	y.arr[0] = 111;
	y.arr[1] = 222;
	y.arr[2] = 333;
	y.carr[0] = 123;
	y.carr[1] = 456;
	y.carr[2] = 789;
	y.vx.push_back(X(234));
	y.vx.push_back(X(6245));
	y.vx.push_back(X(613461));
	y.s = "labal diena";

	Z z{};
	z.y = y;
	z.x = X{ 234 };
	

	serialize(ser, y);
	serialize(ser, z);

	BufferReader br{ buf };
	Deserializer<BufferReader> des(br);

	Y yres{};
	Z zres{};
	serialize(des, yres);
	serialize(des, zres);

	EXPECT_THAT(yres.y, Eq(y.y));
	EXPECT_THAT(yres.vx, ContainerEq(y.vx));
	EXPECT_THAT(yres.arr, ContainerEq(y.arr));
	EXPECT_THAT(yres.carr, ContainerEq(y.carr));
	EXPECT_THAT(yres.s, StrEq(y.s));

	EXPECT_THAT(zres.y.y, Eq(z.y.y));
	EXPECT_THAT(zres.y.vx, ContainerEq(z.y.vx));
	EXPECT_THAT(zres.y.arr, ContainerEq(z.y.arr));
	EXPECT_THAT(zres.y.carr, ContainerEq(z.y.carr));
	EXPECT_THAT(zres.y.s, StrEq(z.y.s));
	EXPECT_THAT(zres.x.s, StrEq(z.x.s));
	EXPECT_THAT(zres.x.x, Eq(z.x.x));

}

TEST(DeltaSerializer, GeneralConceptTest) {
	//std::string buf;
	Y y{};
	y.y = 3423;
	y.arr[0] = 111;
	y.arr[1] = 222;
	y.arr[2] = 333;
	y.carr[0] = 123;
	y.carr[1] = 456;
	y.carr[2] = 789;
	y.vx.push_back(X(234));
	y.vx.push_back(X(6245));
	y.vx.push_back(X(613461));
	y.s = "labal diena";
	y.vx[0].s = "very nice";
	y.vx[1].s = "very nice string, that is a little bit longer that previous";

	Y yRead = y;
	Y yNew = y;	
	yNew.y = 111111;
	yNew.arr[2] = 0xFFFFFFFF;
	yNew.carr[1] = 0xFFFFFFFF;
	yNew.s = "labas dienaABC";
	yNew.vx[0].s = "very nigga";
	yNew.vx[1].s = "bla";
	yNew.vx.push_back(X{ 3 });

	std::vector<uint8_t> buf;
	BufferWriter bw{ buf };
	DeltaSerializer<BufferWriter, Y> ser(bw, y, yNew);
	serialize(ser, yNew);
	bw.flush();
	
	BufferReader br{ buf };
	DeltaDeserializer<BufferReader, Y> des(br, y, yRead);
	serialize(des, yRead);

	EXPECT_THAT(yRead.y, Eq(yNew.y));
	EXPECT_THAT(yRead.vx, ContainerEq(yNew.vx));
	EXPECT_THAT(yRead.arr, ContainerEq(yNew.arr));
	EXPECT_THAT(yRead.carr, ContainerEq(yNew.carr));
	EXPECT_THAT(yRead.s, StrEq(yNew.s));
}

