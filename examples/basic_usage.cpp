//
// Created by fraillt on 17.2.9.
//
#include <iostream>
#include <vector>
#include "BufferReader.h"
#include "BufferWriter.h"
#include "Serializer.h"
#include "Deserializer.h"

enum class MyEnum {
    V1,V2,V3
};

struct MyStruct {
    int i;
    MyEnum e;
    std::vector<float> fs;
};

//define how object should be serialized
SERIALIZE(MyStruct) {
    return s.
            value(o.i).
            value(o.e).
            container(o.fs, 100);
}

void print(const char* msg, const MyStruct& v) {
    std::cout << msg << std::endl;
    std::cout << "i:" << v.i << std::endl;
    std::cout << "e:" << (int)v.e << std::endl;
    std::cout << "fs:";
    for (auto p:v.fs)
        std::cout << '\t' << p;
    std::cout << std::endl << std::endl;
}

using namespace bitsery;

int main() {
    //set some random data
    MyStruct data{};
    data.e = MyEnum::V2;
    data.i = 48465;
    data.fs.resize(4);
    float tmp = 4253;
    for (auto& v: data.fs) {
        tmp /=2;
        v = tmp;
    }

    //create serializer
    //1) create buffer to store data
    std::vector<uint8_t> buffer;
    //2) create buffer writer that is able to write bytes or bits to buffer
    BufferWriter bw{buffer};
    //3) create serializer
    Serializer<BufferWriter> ser{bw};

    //call serialize function
    serialize(ser, data);

    //flush to buffer
    bw.flush();

    MyStruct result{};

    //create deserializer
    //1) create buffer reader
    BufferReader br{buffer};
    //2) create deserializer
    Deserializer<BufferReader> des{br};

    //call same function with different arguments
    serialize(des, result);

    //print results
    print("initial data", data);
    print("result", result);
}