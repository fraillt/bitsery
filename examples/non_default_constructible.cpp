//
// example of how to deserialize non default constructible objects
//

#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/traits/vector.h>

class MyData {
    //define your private data
    float _x{0};
    float _y{0};
    //make bitsery:Access friend
    friend class bitsery::Access;
    //create default constructor, don't worry about class invariant, it will be restored in deserialization
    MyData() = default;
    //define serialize function

    template <typename S>
    void serialize(S& s) {
        s.value4b(_x);
        s.value4b(_y);
    }
public:
    //define non default public constructor
    MyData(float x, float y):_x{x}, _y{y} {}
    //this is for convenience
    bool operator ==(const MyData&rhs) const {
        return _x == rhs._x && _y == rhs._y;
    }
};

using namespace bitsery;

//some helper types
using Buffer = std::vector<uint8_t>;
using Writer = AdapterWriter<OutputBufferAdapter<Buffer>, DefaultConfig>;
using Reader = AdapterReader<InputBufferAdapter<Buffer>, DefaultConfig>;

int main() {

    //initialize our data
    std::vector<MyData> data{};
    data.emplace_back(145.4f, 84.48f);
    std::vector<MyData> res{};

    //we cant use quick (de)serialization helper methods, because we ant to serialize container directly
    //create buffer
    Buffer buffer{};

    //create writer and serialize container
    Writer writer{buffer};
    BasicSerializer<Writer> ser{writer};
    ser.container(data, 10);
    writer.flush();

    //create reader and deserialize container
    Reader reader{{buffer.begin(), writer.writtenBytesCount()}};
    BasicDeserializer<Reader> des{reader};
    des.container(res, 10);

    //check if everything went ok
    assert(reader.error() == ReaderError::NoError && reader.isCompletedSuccessfully());
    assert(res == data);
}

