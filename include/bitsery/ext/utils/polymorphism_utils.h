//
// Created by fraillt on 17.11.3.
//

#ifndef BITSERY_EXT_POLYMORPHISM_H
#define BITSERY_EXT_POLYMORPHISM_H

#include "../inheritance.h"

#include <unordered_map>
#include <functional>
#include <memory>

namespace bitsery {

    namespace ext {
        namespace details_polymorphism {
            //stores template types
            template<typename ...>
            struct List {
            };
        }

        //specialize for your base class by deriving from DerivedClasses with list of derivatives that DIRECTLY inherits from your base class.
//e.g.
// template <> PolymorphicBase<Animal>: DerivedClasses<Dog, Cat>{};
// template <> PolymorphicBase<Dog>: DerivedClasses<Bulldog, GoldenRetriever> {};
// IMPORTANT !!!
// although you can add all derivates to same base like this:
// SuperClass<Animal>:DerivedClasses<Dog, Cat, Bulldog, GoldenRetriever>{};
// it will not work when you try to serialize Dog*, because it will not find Bulldog and GoldenRetriever
        template<typename TBase>
        struct PolymorphicBaseClass {
            using childs = details_polymorphism::List<>;
        };

//derive from this class when specifying childs for your base class, atleast one child must exists, hence T1
//e.g.
// template <> SuperClass<Animal>: DerivedClasses<Dog, Cat>{};
        template<typename T1, typename ... Tn>
        struct DerivedClasses {
            using childs = details_polymorphism::List<T1, Tn...>;
        };

        namespace utils {
            //this object will be used to serialize/deserialize polymorphic type id within inheritance tree from base class
            struct InheritanceTreeTypeId {
                uint16_t depth{};
                uint16_t index{};
                template <typename S>
                void serialize(S& s) {
                    s.value2b(depth);
                    s.value2b(index);
                }
            };

            template <typename S, typename TObject>
            struct PolymorphicObjectHandlerInterface {
                virtual void process(S& s, TObject& obj) const = 0;
                virtual void create(TObject& obj) const = 0;
                virtual ~PolymorphicObjectHandlerInterface() = default;
            };
        }

        namespace details_polymorphism {


            template<typename S, typename TObject, typename TDerived, typename RTTI, typename ObjectHandler>
            struct PolymorphicObjectHandlerBase : public utils::PolymorphicObjectHandlerInterface<S, TObject> {
                void process(S &s, TObject &obj) const final {
                    s.object(dynamic_cast<TDerived &>(*_handler.getPtr(obj)));
                };

                void create(TObject &obj) const final {
                    auto ptr = _handler.getPtr(obj);
                    if (ptr && RTTI::get(*ptr) != RTTI::template get<TDerived>()) {
                        _handler.destroy(obj);
                        _handler.template create<TDerived>(obj);
                    } else {
                        if (ptr == nullptr)
                            _handler.template create<TDerived>(obj);
                    }
                }
                ObjectHandler _handler{};
            };

            template<typename S, typename TObject, template<typename> class TObjectManager>
            class PolymorphicHandlersGenerator {
            public:
                template<typename Fnc>
                static void generate(Fnc &&addHandlerFnc) {
                    PolymorphicHandlersGenerator tmp{std::forward<Fnc>(addHandlerFnc)};
                }

            private:
                using RTTI = typename TObjectManager<TObject>::RTTI;

                template <typename TDerived>
                using TPolymorphicObjectHandler = PolymorphicObjectHandlerBase<S, TObject, TDerived, RTTI, typename TObjectManager<TObject>::THandler>;

                template<typename Fnc>
                explicit PolymorphicHandlersGenerator(Fnc &&addHandlerFnc):_addHandler(std::forward<Fnc>(addHandlerFnc)) {
                    //fill inheritance tree
                    using TBase = typename TObjectManager<TObject>::TValue;
                    add<TBase>();
                }

                template<typename TClass>
                void add() {
                    addClass<TClass>();
                    //save current index and increase depth
                    auto saveIndex = index;
                    index = 0;
                    depth++;
                    addChilds<TClass>(typename PolymorphicBaseClass<TClass>::childs{});
                    //restore index and depth
                    depth--;
                    index = saveIndex;
                }

                template<typename TClassBase, typename T1, typename ... Tn>
                void addChilds(details_polymorphism::List<T1, Tn...>) {
                    static_assert(std::is_base_of<TClassBase, T1>::value,
                                  "PolymorphicBaseClass<TBase> must derive a list of derived classes from TBase.");
                    add<T1>();
                    addChilds<TClassBase>(details_polymorphism::List<Tn...>{});
                }

                template<typename>
                void addChilds(details_polymorphism::List<>) {
                }


                template<typename TClass>
                void addClass() {
                    if (!std::is_abstract<TClass>::value) {
                        utils::InheritanceTreeTypeId tid{};
                        tid.index = index;
                        tid.depth = depth;
#if __cplusplus > 201103L
                        auto handler = std::make_unique<TPolymorphicObjectHandler<TClass>>();
#else
                        auto handler = std::unique_ptr<TPolymorphicObjectHandler<TClass>>(
                                new TPolymorphicObjectHandler<TClass>{});
#endif
                        if (_addHandler(RTTI::template get<TClass>(), tid, std::move(handler)))
                            ++index;
                    }
                }

                std::function<bool(size_t, utils::InheritanceTreeTypeId,
                                   std::unique_ptr<utils::PolymorphicObjectHandlerInterface<S, TObject>> &&)> _addHandler;
                uint16_t depth{};
                uint16_t index{};
            };

        }

        namespace utils {

            template<typename Ser, typename TObject, template<typename> class TObjectManager>
            class InheritanceTreeSerialize {
            public:

                InheritanceTreeSerialize() {
                    details_polymorphism::PolymorphicHandlersGenerator<Ser, TObject, TObjectManager>::generate(
                            [this](size_t typeHash, InheritanceTreeTypeId id,
                                   std::unique_ptr<PolymorphicObjectHandlerInterface<Ser, TObject>> &&handler) {
                                return _map.emplace(std::make_pair(typeHash, std::make_pair(id, std::move(handler)))).second;
                            }
                    );

                }

                const std::pair<InheritanceTreeTypeId, PolymorphicObjectHandlerInterface<Ser, TObject>*> getHandler(const TObject &obj) {
                    using RTTI = typename TObjectManager<TObject>::RTTI;
                    auto handler = typename TObjectManager<TObject>::THandler{};
                    auto it = _map.find(RTTI::get(*handler.getPtr(obj)));
                    if (it != _map.end()) {
                        return std::make_pair(it->second.first, it->second.second.get());
                    }
                    return {};
                }

            private:
                std::unordered_map<size_t, std::pair<InheritanceTreeTypeId, std::unique_ptr<PolymorphicObjectHandlerInterface<Ser, TObject>>>> _map{};
            };


            template<typename Des, typename TObject, template<typename> class TObjectManager>
            class InheritanceTreeDeserialize {
            public:
                InheritanceTreeDeserialize() {
                    details_polymorphism::PolymorphicHandlersGenerator<Des, TObject, TObjectManager>::generate(
                            [this](size_t, InheritanceTreeTypeId id, std::unique_ptr<PolymorphicObjectHandlerInterface<Des, TObject>> &&handler) {
                                return _map.emplace(std::make_pair(getHashFromId(id), std::move(handler))).second;
                            }
                    );
                }

                const PolymorphicObjectHandlerInterface<Des, TObject> *getHandler(const InheritanceTreeTypeId &id) {
                    auto it = _map.find(getHashFromId(id));
                    if (it != _map.end())
                        return it->second.get();
                    return nullptr;
                }

            private:
                size_t getHashFromId(const InheritanceTreeTypeId &id) const {
                    size_t res = id.depth << 16;
                    return res + id.index;
                }

                std::unordered_map<size_t, std::unique_ptr<PolymorphicObjectHandlerInterface<Des, TObject>>> _map{};
            };


        }


    }

}

#endif //BITSERY_EXT_POLYMORPHISM_H
