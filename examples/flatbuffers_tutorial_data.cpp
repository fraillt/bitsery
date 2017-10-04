#include <bitsery/bitsery.h>
#include <bitsery/ext/growable.h>
#include <bitsery/traits/string.h>
#include <bitsery/traits/array.h>

namespace MyTypes {

    //define data
    enum Color:uint8_t { Red, Green, Blue };

    struct Vec3 { float x, y, z; };

    struct Weapon {
        std::string name;
        int16_t damage;
    };

    struct Monster {
        Vec3 pos;
        int16_t mana;
        int16_t hp;
        std::string name;
        std::vector<uint8_t> inventory;
        Color color;
        std::vector<Weapon> weapons;
        Weapon equipped;
        std::vector<Vec3> path;
    };

    template <typename S>
    void serialize(S& s, Vec3& o) {
        s.value4b(o.x);
        s.value4b(o.y);
        s.value4b(o.z);
    }

    //define serialization functions
    template <typename S>
    void serialize (S& s, Weapon& o) {
        //forward/backward compatibility for monsters
        s.ext(o, bitsery::ext::Growable{}, [&s](Weapon& o1) {
            s.text1b(o1.name, 20);
            s.value2b(o1.damage);
        });
    }

    template <typename S>
    void serialize (S& s, Monster& o) {
        //forward/backward compatibility for monsters
        s.ext(o, bitsery::ext::Growable{}, [&s](Monster& o1) {
            s.value1b(o1.color);
            s.value2b(o1.mana);
            s.value2b(o1.hp);
            s.object(o1.equipped);
            s.object(o1.pos);
            s.container(o1.path, 1000);
            s.container(o1.weapons, 100);
            s.container1b(o1.inventory, 50);
            s.text1b(o1.name, 20);
        });
    }
}

using namespace bitsery;

using Buffer =std::array<uint8_t, 1000000>;
//change configuration
struct NonDefaultConfig: public bitsery::DefaultConfig {
    //change underlying buffer
    using BufferType = Buffer;
};


int main() {
    //set some random data
    MyTypes::Monster data{};
    data.name = "lew";

    //create serializer
    //1) create buffer to store data
    Buffer buffer{};
    //2) create buffer writer that is able to write bytes or bits to buffer
    BasicBufferWriter<NonDefaultConfig> bw{buffer};
    //3) create serializer
    BasicSerializer<NonDefaultConfig, false> ser{bw};

    //serialize object, can also be invoked like this: serialize(ser, data)
    ser.object(data);

    //flush to buffer, before creating buffer reader, this will always write sessions data for forward/backward compatibility
    bw.flush();

    //create deserializer
    //1) create buffer reader
    BasicBufferReader<NonDefaultConfig> br{bw.getWrittenRange()};
    //2) create deserializer
    BasicDeserializer<NonDefaultConfig, false> des{br};

    //deserialize same object, can also be invoked like this: serialize(des, data)
    MyTypes::Monster res{};
    des.object(res);

}
