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

SERIALIZE(MyStruct) {
    return s.
            value(o.i).
            value(o.e).
            container(o.fs);
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
    std::vector<uint8_t> buffer;
    BufferWriter bw{buffer};
    Serializer<BufferWriter> ser{bw};
    //call serialize function
    serialize(ser, data);
    //this is required if using bit operations
    bw.flush();

    MyStruct result{};
    //create deserializer
    BufferReader br{buffer};
    Deserializer<BufferReader> des{br};

    //call same function with different arguments
    serialize(des, result);

    //print results
    print("initial data", data);
    print("result", result);
}