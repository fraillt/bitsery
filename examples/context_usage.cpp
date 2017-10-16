#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>

#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

#include <bitsery/ext/value_range.h>

//defines game mode parameters
struct GameMode {
    uint32_t maxMonsters;
    uint32_t damageRange[2];
};

namespace MyTypes {

    struct Monster {
        Monster() = default;
        Monster(std::string _name, uint32_t minDmg, uint32_t maxDmg)
                :name{_name}, minDamage{minDmg}, maxDamage{maxDmg} {}

        std::string name{};
        uint32_t minDamage{};
        uint32_t maxDamage{};
        //...
    };

    struct GameState {
        std::vector<Monster> monsters;
    };

    //default flow for monster
    template <typename S>
    void serialize (S& s, Monster& o) {
        s.text1b(o.name, 20);
        s.value4b(o.minDamage);
        s.value4b(o.maxDamage);
    }

    template<typename S>
    void serialize(S& s, GameState &o) {
        auto mode = static_cast<GameMode*>(s.context());
        //we know max number of monsters from context
        s.container(o.monsters, mode->maxMonsters, [&s, mode] (Monster& m) {
            s.text1b(m.name, 20);
            //we know min/max damage range for monsters, so we can use this range instead of full value
            bitsery::ext::ValueRange<uint32_t> range{mode->damageRange[0], mode->damageRange[1]};
            //enable bit packing
            s.enableBitPacking([&m, &range](typename S::BPEnabledType& sbp) {
                sbp.ext(m.minDamage, range);
                sbp.ext(m.maxDamage, range);
            });
        });
    }

}

using namespace bitsery;

//use fixed-size buffer
using Buffer = std::vector<uint8_t>;
using OutputAdapter = OutputBufferAdapter<Buffer>;
using InputAdapter = InputBufferAdapter<Buffer>;

int main() {
    MyTypes::GameState data{};
    data.monsters.push_back({"weaksy", 100, 200});
    data.monsters.push_back({"bigsy", 500, 1000});
    data.monsters.push_back({"tootoo", 350, 750});

    //set game mode
    //this store game mode parameters
    GameMode mode{};
    mode.maxMonsters = 4;
    mode.damageRange[0] = 100;
    mode.damageRange[1] = 1000;

    //create buffer to store data to
    Buffer buffer{};
    //pass game mode object to serializer as context
    Serializer<OutputAdapter> ser{buffer, &mode};
    ser.object(data);

    auto& w = AdapterAccess::getWriter(ser);
    w.flush();
    auto writtenSize = w.writtenBytesCount();

    MyTypes::GameState res{};
    Deserializer <InputAdapter> des { InputAdapter{buffer.begin(), writtenSize}, &mode};
    des.object(res);
    auto& r = AdapterAccess::getReader(des);

    assert(r.error() == ReaderError::NoError && r.isCompletedSuccessfully());
}
