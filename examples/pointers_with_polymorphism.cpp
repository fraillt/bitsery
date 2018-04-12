//
// Created by fraillt on 18.4.26.
//
#include <cassert>
#include <memory>
#include <bitsery/bitsery.h>
#include <bitsery/traits/vector.h>
#include <bitsery/adapter/buffer.h>
#include <bitsery/ext/pointer.h>
#include <bitsery/ext/std_smart_ptr.h>

//in order to work with polymorphic types, we need to describe few steps:
// 1) describe relationships between base and derived types
// this will allow to know what are possible types reachable from base class
// 2) bind serializer to base class
// this will allow to iterate through all types, and add serialization functions,
// without this step compiler would simply remove functions that are not bound at compile-time even it we use type at runtime.

using bitsery::ext::BaseClass;

using bitsery::ext::PointerOwner;

using bitsery::ext::PointerType;

//define our data structures
struct Color {
    float r, g, b;
};

struct Shape {
    Color clr;

    virtual ~Shape() = 0;
};

Shape::~Shape() = default;

struct Circle : Shape {
    int32_t radius;
};

struct Rectangle : Shape {
    int32_t width;
    int32_t height;
};

struct RoundedRectangle : Rectangle {
    int32_t radius;
};

struct SomeShapes {
    std::unique_ptr<Shape> main;
    std::vector<Shape *> list;
};

//define serialization functions
template<typename S>
void serialize(S &s, Color &o) {
    //in real world scenario, it might be possible to serialize this using ValueRange, to map values in smaller space
    //but for the sake of this example keep it simple
    s.value4b(o.r);
    s.value4b(o.g);
    s.value4b(o.b);
}

template<typename S>
void serialize(S &s, Shape &o) {
    s.object(o.clr);
}

template<typename S>
void serialize(S &s, Circle &o) {
    s.ext(o, bitsery::ext::BaseClass<Shape>{});
    s.value4b(o.radius);
}

template<typename S>
void serialize(S &s, Rectangle &o) {
    s.ext(o, bitsery::ext::BaseClass<Shape>{});
    s.value4b(o.width);
    s.value4b(o.height);
}

template<typename S>
void serialize(S &s, RoundedRectangle &o) {
    s.ext(o, bitsery::ext::BaseClass<Rectangle>{});
    s.value4b(o.radius);
}

template<typename S>
void serialize(S &s, SomeShapes &o) {
    s.ext(o.main, bitsery::ext::StdUniquePtr{});
    s.container(o.list, 100, [&s](Shape *(&item)) {
        s.ext(item, bitsery::ext::PointerOwner{});
    });
}

// STEP 1
// define relationships between base and derived classes

namespace bitsery {
    namespace ext {

        template<>
        struct PolymorphicBaseClass<Shape> : PolymorphicDerivedClasses<Circle, Rectangle> {
        };

        template<>
        struct PolymorphicBaseClass<Rectangle> : PolymorphicDerivedClasses<RoundedRectangle> {
        };
    }
}

//use bitsery namespace for convenience
using namespace bitsery;

//some helper types
using Buffer = std::vector<uint8_t>;
using OutputAdapter = OutputBufferAdapter<Buffer>;
using InputAdapter = InputBufferAdapter<Buffer>;

//we need to define few things in order to work with polymorphism
//1) we need pointer linking context to work with pointers
//2) we need polymorphic context to be able to work with polymorphic types
using TContext = std::tuple<ext::PointerLinkingContext, ext::PolymorphicContext<ext::StandardRTTI>>;
//NOTE:
// RTTI can be customizable, if you can't use dynamic_cast and typeid, and have 'custom' solution

using MySerializer = BasicSerializer<AdapterWriter<OutputAdapter, DefaultConfig>, TContext>;
using MyDeserializer = BasicDeserializer<AdapterReader<InputAdapter, DefaultConfig>, TContext>;

//creates object, and populates some data
SomeShapes createData() {
    SomeShapes data{};
    {
        auto tmp = new RoundedRectangle{};
        tmp->height = 151572;
        tmp->width = 488795;
        tmp->radius = 898;
        tmp->clr.r = 0.5f;
        tmp->clr.g = 1.0f;
        tmp->clr.b = 1.0f;
        data.main.reset(tmp);
    }
    {
        auto tmp = new Circle{};
        tmp->radius = 75987;
        tmp->clr.r = 0.5f;
        tmp->clr.g = 0.0f;
        tmp->clr.b = 1.0f;
        data.list.push_back(tmp);
    }
    {
        auto tmp = new Rectangle{};
        tmp->height = 15157;
        tmp->width = 48879;
        tmp->clr.r = 1.0f;
        tmp->clr.g = 0.0f;
        tmp->clr.b = 0.0f;
        data.list.push_back(tmp);
    }

    return data;
}

//checks if deserialized data is equal
void assertSameShapes(const SomeShapes &data, const SomeShapes &res) {
    {
        auto d = dynamic_cast<RoundedRectangle *>(data.main.get());
        auto r = dynamic_cast<RoundedRectangle *>(res.main.get());
        assert(r != nullptr);
        assert(d->clr.r == r->clr.r);
        assert(d->clr.g == r->clr.g);
        assert(d->clr.b == r->clr.b);
        assert(d->radius == r->radius);
        assert(d->width == r->width);
        assert(d->height == r->height);
    }
    {
        auto d = dynamic_cast<Circle *>(data.list[0]);
        auto r = dynamic_cast<Circle *>(res.list[0]);
        assert(r != nullptr);
        assert(d->clr.r == r->clr.r);
        assert(d->clr.g == r->clr.g);
        assert(d->clr.b == r->clr.b);
        assert(d->radius == r->radius);
    }
    {
        auto d = dynamic_cast<Rectangle *>(data.list[1]);
        auto r = dynamic_cast<Rectangle *>(res.list[1]);
        assert(r != nullptr);
        assert(d->clr.r == r->clr.r);
        assert(d->clr.g == r->clr.g);
        assert(d->clr.b == r->clr.b);
        assert(d->width == r->width);
        assert(d->height == r->height);
    }

}

int main() {

    auto data = createData();

    //create buffer to store data
    Buffer buffer{};
    size_t writtenSize{};
    {
        TContext ctx{};
        MySerializer ser{OutputAdapter{buffer}, &ctx};
        //STEP 2
        //bind serializer with base polymorphic types, it will go through all reachable classes that is defined in first step.
        //so you dont need to add Rectangle to reach for RoundedRectangle
        std::get<1>(ctx).registerBasesList(ser, ext::PolymorphicClassesList<Shape>{});
        //serialize our data
        ser.object(data);
        auto &w = AdapterAccess::getWriter(ser);
        w.flush();
        writtenSize = w.writtenBytesCount();

        //make sure that pointer linking context is valid
        //this ensures that all non-owning pointers points to data that has been serialized,
        //so we can successfully reconstruct pointers after deserialization
        assert(std::get<0>(ctx).isValid());
    }
    SomeShapes res{};
    {
        TContext ctx{};
        MyDeserializer des{InputAdapter{buffer.begin(), writtenSize}, &ctx};
        //same as in serialization
        std::get<1>(ctx).registerBasesList(des, ext::PolymorphicClassesList<Shape>{});
        //serialize our data
        des.object(res);
        auto &r = AdapterAccess::getReader(des);
        //check if everything went find
        assert(r.error() == ReaderError::NoError && r.isCompletedSuccessfully());
        //also check for dangling pointers, after deserialization
        assert(std::get<0>(ctx).isValid());
    }
    assertSameShapes(data, res);
    //delete raw pointers
    for (auto &s:res.list)
        delete s;
    for (auto &s:data.list)
        delete s;
    return 0;
}