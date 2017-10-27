#include <bitsery/bitsery.h>
#include <bitsery/adapter/buffer.h>

#include <bitsery/traits/string.h>
#include <bitsery/traits/vector.h>

#include <bitsery/ext/value_range.h>

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
        //we can have multiple types in context with std::tuple
        //this cast also works if our context is the same as cast
        auto maxMonsters = s.template context<int>();
        //all data from context is always pointer
        //if data type doesn't match then it will be compile time error
        auto dmgRange = s.template context<std::pair<uint32_t, uint32_t>>();
        s.container(o.monsters, *maxMonsters, [&s, dmgRange] (Monster& m) {
            s.text1b(m.name, 20);
            //we know min/max damage range for monsters, so we can use this range instead of full value
            bitsery::ext::ValueRange<uint32_t> range{dmgRange->first, dmgRange->second};
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
//context can contain multiple types
//it would make more sense to define separate structure for context, but for sake of this example make it more complex
//in serialization function we can cast it like this:
//  s.template context<int>();
//if we want to get whole tuple, just call s.context() without template paramter.
//this templated version also works if our context is the same as cast:
//  struct MyContext {...};
//  ...
//  s.template context<MyContext>();
using Context = std::tuple<int, std::pair<uint32_t, uint32_t>>;

int main() {

    MyTypes::GameState data{};
    data.monsters.push_back({"weaksy", 100, 200});
    data.monsters.push_back({"bigsy", 500, 1000});
    data.monsters.push_back({"tootoo", 350, 750});

    //set context
    Context ctx{};
    //max monsters
    std::get<0>(ctx) = 4;
    //damage range
    std::get<1>(ctx).first = 100;
    std::get<1>(ctx).second = 1000;


    //create buffer to store data to
    Buffer buffer{};
    //pass game mode object to serializer as context
    BasicSerializer<AdapterWriter<OutputAdapter, bitsery::DefaultConfig>, Context> ser{buffer, &ctx};
    ser.object(data);

    auto& w = AdapterAccess::getWriter(ser);
    w.flush();
    auto writtenSize = w.writtenBytesCount();

    MyTypes::GameState res{};
    BasicDeserializer <AdapterReader<InputAdapter, bitsery::DefaultConfig>, Context> des { InputAdapter{buffer.begin(), writtenSize}, &ctx};
    des.object(res);
    auto& r = AdapterAccess::getReader(des);

    assert(r.error() == ReaderError::NoError && r.isCompletedSuccessfully());
}
