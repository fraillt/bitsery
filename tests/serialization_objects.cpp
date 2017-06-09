//MIT License
//
//Copyright (c) 2017 Mindaugas Vinkelis
//
//Permission is hereby granted, free of charge, to any person obtaining a copy
//of this software and associated documentation files (the "Software"), to deal
//in the Software without restriction, including without limitation the rights
//to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
//copies of the Software, and to permit persons to whom the Software is
//furnished to do so, subject to the following conditions:
//
//The above copyright notice and this permission notice shall be included in all
//copies or substantial portions of the Software.
//
//THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
//IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
//FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
//AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
//LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
//OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
//SOFTWARE.


#include <gmock/gmock.h>
#include "serialization_test_utils.h"

#include <bitsery/delta_serializer.h>
#include <bitsery/delta_deserializer.h>

#include <list>



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
	return s.template value<sizeof(o.x)>(o.x)
		.template text<1>(o.s, 1000);
}

SERIALIZE(Y)
{
	auto writeInt = [](auto& s, auto& v) { s.template value<sizeof(v)>(v); };
	s.template text<1>(o.s, 10000);
	s.template value<sizeof(o.y)>(o.y);
	s.array(o.arr, writeInt);
	s.array(o.carr, writeInt);
	s.container(o.vx, 10000, [](auto& s, auto& v) { s.object(v); });
	return s;
}


TEST(SerializeObject, GeneralConceptTest) {
	//std::string buf;
	SerializationContext ctx;
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
	

	auto ser = ctx.createSerializer();
	ser.object(y);
	ser.object(z);


	Y yres{};
	Z zres{};

	auto des = ctx.createDeserializer();
	des.object(yres);
	des.object(zres);

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
	yNew.vx[0].s = "very opapa";
	yNew.vx[1].s = "bla";
	yNew.vx.push_back(X{ 3 });

	std::vector<uint8_t> buf;
	bitsery::BufferWriter bw{ buf };
	bitsery::DeltaSerializer<bitsery::BufferWriter, Y> ser(bw, y, yNew);
	serialize(ser, yNew);
	bw.flush();

	bitsery::BufferReader br{ buf };
	bitsery::DeltaDeserializer<bitsery::BufferReader, Y> des(br, y, yRead);
	serialize(des, yRead);

	EXPECT_THAT(yRead.y, Eq(yNew.y));
	EXPECT_THAT(yRead.vx, ContainerEq(yNew.vx));
	EXPECT_THAT(yRead.arr, ContainerEq(yNew.arr));
	EXPECT_THAT(yRead.carr, ContainerEq(yNew.carr));
	EXPECT_THAT(yRead.s, StrEq(yNew.s));
}

