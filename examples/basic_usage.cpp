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

#include <iostream>
#include <vector>
#include <bitsery/bitsery.h>


enum class MyEnum { V1,V2,V3 };
struct MyStruct {
    int i;
    MyEnum e;
    std::vector<float> fs;
};

//define how object should be serialized/deserialized
SERIALIZE(MyStruct) {
    return s.
            value4(o.i).
            value4(o.e).
            container4(o.fs, 100);
}

using namespace bitsery;

int main() {
    //set some random data
    MyStruct data{8941, MyEnum::V2, {15.0f, -8.5f, 0.045f}};
    MyStruct res{};

    //create serializer
    //1) create buffer to store data
    std::vector<uint8_t> buffer;
    //2) create buffer writer that is able to write bytes or bits to buffer
    BufferWriter bw{buffer};
    //3) create serializer
    Serializer<BufferWriter> ser{bw};

    //serialize object, can also be invoked like this: serialize(ser, data)
    ser.object(data);

    //flush to buffer, before creating buffer reader
    bw.flush();

    //create deserializer
    //1) create buffer reader
    BufferReader br{buffer};
    //2) create deserializer
    Deserializer<BufferReader> des{br};

    //deserialize same object, can also be invoked like this: serialize(des, data)
    des.object(res);

    //check is equal
    std::cout << "is equal: " << (data.fs == res.fs && data.i == res.i && data.e == res.e) << std::endl;
}