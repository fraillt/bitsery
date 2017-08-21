#include <vector>
#include <bitsery/bitsery.h>
#include <cstring>
#include <iostream>

struct Vector3f {
    float x;
    float y;
    float z;
    bool operator == (const Vector3f& o) const {
        return x == o.x && y == o.y && z == o.z;
    }
};

struct Player {
    Vector3f pos;
    char name[50];
};

using namespace bitsery;

SERIALIZE(Player) {
    s.value4(o.pos.x);
    s.value4(o.pos.y);
    s.value4(o.pos.z);
    s.text1(o.name);
    return s;
}

Player createData() {
    Player data;
    data.pos.x = 0.45f;
    data.pos.y = 50.9f;
    data.pos.z = -15687.87f;
    strcpy(data.name,"Yolo");
    return data;
}

int main() {
    const Player data = createData();
    Player res{};

    std::vector<uint8_t> buf;

    BufferWriter bw{buf};
    Serializer<BufferWriter> ser{bw};

    serialize(ser, data);

    bw.flush();

    BufferReader br{buf};
    Deserializer<BufferReader> des{br};

    serialize(des, res);

    std::cout << "deserializer state: " << des.isValid() << std::endl
              << "buffer completed: " << br.isCompleted() << std::endl
              << "pos equals: " << (res.pos == data.pos) << std::endl
              << "name equals: " << (strcmp(res.name, data.name) == 0);
    return 0;
}
