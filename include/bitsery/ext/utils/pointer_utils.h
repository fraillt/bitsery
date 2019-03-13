//MIT License
//
//Copyright (c) 2018 Mindaugas Vinkelis
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

#ifndef BITSERY_POINTER_UTILS_H
#define BITSERY_POINTER_UTILS_H

#include <unordered_map>
#include <vector>
#include <memory>
#include <algorithm>
#include <cassert>
#include "polymorphism_utils.h"
#include "../../details/adapter_utils.h"
#include "../../details/serialization_common.h"

namespace bitsery {
    namespace ext {

        //change name
        enum class PointerType : uint8_t {
            Nullable,
            NotNull
        };

        // Observer - not responsible for pointer lifetime management.
        // Owner - only ONE owner is responsible for this pointers creation/destruction
        // SharedOwner, SharedObserver - MANY shared owners is responsible for pointer creation/destruction
        // requires additional context to manage shared owners themselves.
        // SharedOwner actually manages life time e.g. std::shared_ptr
        // SharedObserver do not manage life time of the pointer, but can observe shared state .e.. std::weak_ptr
        // and differently from Observer, creates new object if necessary and saves to shared state
        enum class PointerOwnershipType : uint8_t {
            Observer,
            Owner,
            SharedOwner,
            SharedObserver
        };

        namespace pointer_utils {

            //this class is used to store context for shared ptr owners
            struct PointerSharedStateBase {
                virtual ~PointerSharedStateBase() = default;
            };

            //PLC info is internal classes for serializer, and deserializer
            struct PLCInfo {
                explicit PLCInfo(PointerOwnershipType ownershipType_)
                    : ownershipType{ownershipType_},
                      isSharedProcessed{false} {};
                PointerOwnershipType ownershipType;
                bool isSharedProcessed;

                void update(PointerOwnershipType ptrType) {
                    //do nothing for observer
                    if (ptrType == PointerOwnershipType::Observer)
                        return;
                    if (ownershipType == PointerOwnershipType::Observer) {
                        //set ownership type
                        ownershipType = ptrType;
                        return;
                    }
                    //only shared ownership can get here multiple times
                    assert(ptrType == PointerOwnershipType::SharedOwner ||
                           ptrType == PointerOwnershipType::SharedObserver);
                    //check if need to update to SharedOwner
                    if (ptrType == PointerOwnershipType::SharedOwner)
                        ownershipType = ptrType;
                    //mark that object already processed, so we do not serialize/deserialize duplicate objects
                    isSharedProcessed = true;
                }
            };

            struct PLCInfoSerializer : PLCInfo {
                PLCInfoSerializer(size_t id_, PointerOwnershipType ownershipType_)
                    : PLCInfo(ownershipType_), id{id_} {}

                size_t id;
            };

            struct PLCInfoDeserializer : PLCInfo {
                PLCInfoDeserializer(void* ptr, PointerOwnershipType ownershipType_)
                    : PLCInfo(ownershipType_),
                      ownerPtr{ptr} {};

                //need to override these explicitly because we have pointer member
                PLCInfoDeserializer(const PLCInfoDeserializer&) = delete;

                PLCInfoDeserializer(PLCInfoDeserializer&&) = default;

                PLCInfoDeserializer& operator=(const PLCInfoDeserializer&) = delete;

                PLCInfoDeserializer& operator=(PLCInfoDeserializer&&) = default;

                void processOwner(void* ptr) {
                    ownerPtr = ptr;
                    assert(ownershipType != PointerOwnershipType::Observer);
                    for (auto& o:observersList)
                        o.get() = ptr;
                    observersList.clear();
                    observersList.shrink_to_fit();
                }

                void processObserver(void* (& ptr)) {
                    if (ownerPtr) {
                        ptr = ownerPtr;
                    } else {
                        observersList.emplace_back(ptr);
                    }
                }

                void* ownerPtr;
                std::vector<std::reference_wrapper<void*>> observersList{};
                std::unique_ptr<PointerSharedStateBase> sharedState{};
            };

            class PointerLinkingContextSerialization {
            public:
                explicit PointerLinkingContextSerialization()
                    : _currId{0},
                      _ptrMap{} {}

                PointerLinkingContextSerialization(const PointerLinkingContextSerialization&) = delete;

                PointerLinkingContextSerialization& operator=(const PointerLinkingContextSerialization&) = delete;

                PointerLinkingContextSerialization(PointerLinkingContextSerialization&&) = default;

                PointerLinkingContextSerialization& operator=(PointerLinkingContextSerialization&&) = default;

                ~PointerLinkingContextSerialization() = default;

                const PLCInfoSerializer& getInfoByPtr(const void* ptr, PointerOwnershipType ptrType) {
                    auto res = _ptrMap.emplace(ptr, PLCInfoSerializer{_currId + 1u, ptrType});
                    auto& ptrInfo = res.first->second;
                    if (res.second) {
                        ++_currId;
                        return ptrInfo;
                    }
                    ptrInfo.update(ptrType);
                    return ptrInfo;
                }

                //valid, when all pointers have owners.
                //we cannot serialize pointers, if we haven't serialized objects themselves
                bool isPointerSerializationValid() const {
                    return std::all_of(_ptrMap.begin(), _ptrMap.end(),
                                       [](const std::pair<const void*, PLCInfoSerializer>& p) {
                                           return p.second.ownershipType == PointerOwnershipType::SharedOwner ||
                                                  p.second.ownershipType == PointerOwnershipType::Owner;
                                       });
                }

            private:
                size_t _currId;
                std::unordered_map<const void*, PLCInfoSerializer> _ptrMap;

            };

            class PointerLinkingContextDeserialization {
            public:
                explicit PointerLinkingContextDeserialization()
                    : _idMap{} {}

                PointerLinkingContextDeserialization(const PointerLinkingContextDeserialization&) = delete;

                PointerLinkingContextDeserialization& operator=(const PointerLinkingContextDeserialization&) = delete;

                PointerLinkingContextDeserialization(PointerLinkingContextDeserialization&&) = default;

                PointerLinkingContextDeserialization& operator=(PointerLinkingContextDeserialization&&) = default;

                ~PointerLinkingContextDeserialization() = default;

                PLCInfoDeserializer& getInfoById(size_t id, PointerOwnershipType ptrType) {
                    auto res = _idMap.emplace(id, PLCInfoDeserializer{nullptr, ptrType});
                    auto& ptrInfo = res.first->second;
                    if (!res.second)
                        ptrInfo.update(ptrType);
                    return ptrInfo;
                }

                void clearSharedState() {
                    for (auto& item: _idMap)
                        item.second.sharedState.reset();
                }

                //valid, when all pointers has owners
                bool isPointerDeserializationValid() const {
                    return std::all_of(_idMap.begin(), _idMap.end(),
                                       [](const std::pair<const size_t, PLCInfoDeserializer>& p) {
                                           return p.second.ownershipType == PointerOwnershipType::SharedOwner ||
                                                  p.second.ownershipType == PointerOwnershipType::Owner;
                                       });
                }

            private:
                std::unordered_map<size_t, PLCInfoDeserializer> _idMap;
            };
        }

        //this class is for convenience
        class PointerLinkingContext :
            public pointer_utils::PointerLinkingContextSerialization,
            public pointer_utils::PointerLinkingContextDeserialization,
            public PolymorphicAllocator {
        public:
            explicit PointerLinkingContext() = default;

            bool isValid() {
                return isPointerSerializationValid() && isPointerDeserializationValid();
            }
        };

        namespace pointer_utils {

            template<template<typename> class TPtrManager,
                template<typename> class TPolymorphicContext, typename RTTI>
            class PointerObjectExtensionBase {
            public:

                // helper types
                template<typename T>
                struct IsPolymorphic : std::integral_constant<bool,
                    RTTI::template isPolymorphic<typename TPtrManager<T>::TElement>()> {
                };

                template<PointerOwnershipType Value>
                using OwnershipType = std::integral_constant<PointerOwnershipType, Value>;


                explicit PointerObjectExtensionBase(PointerType ptrType = PointerType::Nullable,
                                                    MemResourceBase* resource = nullptr,
                                                    bool resourcePropagate = false) :
                    _ptrType{ptrType},
                    _resourcePropagate{resourcePropagate},
                    _resource{resource} {
                }

                template<typename Ser, typename Writer, typename T, typename Fnc>
                void serialize(Ser& ser, Writer& w, const T& obj, Fnc&& fnc) const {

                    auto ptr = TPtrManager<T>::getPtr(const_cast<T&>(obj));
                    if (ptr) {
                        auto ctx = ser.template context<PointerLinkingContext>();
                        assert(ctx != nullptr);
                        auto& ptrInfo = ctx->getInfoByPtr(getBasePtr(ptr), TPtrManager<T>::getOwnership());
                        details::writeSize(w, ptrInfo.id);
                        if (TPtrManager<T>::getOwnership() != PointerOwnershipType::Observer) {
                            if (!ptrInfo.isSharedProcessed)
                                serializeImpl(ser, ptr, std::forward<Fnc>(fnc), w, IsPolymorphic<T>{});
                        }
                    } else {
                        assert(_ptrType == PointerType::Nullable);
                        details::writeSize(w, 0);
                    }

                }

                template<typename Des, typename Reader, typename T, typename Fnc>
                void deserialize(Des& des, Reader& r, T& obj, Fnc&& fnc) const {
                    size_t id{};
                    details::readSize(r, id, std::numeric_limits<size_t>::max());
                    auto ctx = des.template context<PointerLinkingContext>();
                    assert(ctx != nullptr);
                    if (id) {
                        auto& ptrInfo = ctx->getInfoById(id, TPtrManager<T>::getOwnership());
                        deserializeImpl(*ctx, ptrInfo, des, obj, std::forward<Fnc>(fnc), r, IsPolymorphic<T>{},
                                        OwnershipType<TPtrManager<T>::getOwnership()>{});
                    } else {
                        if (_ptrType == PointerType::Nullable) {
                            if (auto ptr = TPtrManager<T>::getPtr(obj)) {
                                auto prevMemResource = ctx->getMemResource();
                                if (_resource) ctx->setMemResource(_resource);
                                destroyPtr(*ctx, des, obj, IsPolymorphic<T>{});
                                ctx->setMemResource(prevMemResource);
                            };
                        } else
                            r.setError(ReaderError::InvalidPointer);
                    }
                }

            private:

                template<typename Des, typename TObj>
                void destroyPtr(PointerLinkingContext& plc, Des& des, TObj& obj,
                                std::true_type /*polymorphic*/) const {
                    const auto& ctx = des.template context<TPolymorphicContext<RTTI>>();
                    auto ptr = TPtrManager<TObj>::getPtr(obj);
                    TPtrManager<TObj>::destroyPolymorphic(obj, plc, ctx->getPolymorphicHandler(*ptr));
                }

                template<typename Des, typename TObj>
                void destroyPtr(PointerLinkingContext& plc, Des&, TObj& obj,
                                std::false_type /*polymorphic*/) const {
                    TPtrManager<TObj>::destroy(obj, plc, RTTI::template get<typename TPtrManager<TObj>::TElement>());
                }


                template<typename T>
                const void* getBasePtr(const T* ptr) const {
                    // todo implement handling of types with virtual inheritance
                    // this is required to correctly track same object, when one object is derived and other is base class
                    // e.g. shared_ptr<Base> and weak_ptr<Derived> or pointer observer Base*
                    return ptr;
                }

                template<typename Ser, typename TPtr, typename Fnc, typename Writer>
                void serializeImpl(Ser& ser, TPtr& ptr, Fnc&&, Writer& w, std::true_type) const {
                    const auto& ctx = ser.template context<TPolymorphicContext<RTTI>>();
                    ctx->serialize(ser, w, *ptr);
                }

                template<typename Ser, typename TPtr, typename Fnc, typename Writer>
                void serializeImpl(Ser&, TPtr& ptr, Fnc&& fnc, Writer&, std::false_type) const {
                    fnc(*ptr);
                }

                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContext& plc, PLCInfoDeserializer& ptrInfo, Des& des, T& obj, Fnc&&,
                                     Reader& r, std::true_type, OwnershipType<PointerOwnershipType::Owner>) const {
                    const auto& ctx = des.template context<TPolymorphicContext<RTTI>>();
                    auto prevMemResource = plc.getMemResource();
                    ctx->deserialize(des, r, TPtrManager<T>::getPtr(obj),
                                     [&obj, &plc, this, prevMemResource](
                                         const std::shared_ptr<PolymorphicHandlerBase>& handler) {
                                         if (_resource) plc.setMemResource(_resource);
                                         TPtrManager<T>::createPolymorphic(obj, plc, handler);
                                         if (!_resourcePropagate) plc.setMemResource(prevMemResource);
                                         return TPtrManager<T>::getPtr(obj);
                                     },
                                     [&obj, &plc, this](const std::shared_ptr<PolymorphicHandlerBase>& handler) {
                                         if (_resource) plc.setMemResource(_resource);
                                         TPtrManager<T>::destroyPolymorphic(obj, plc, handler);
                                     });
                    plc.setMemResource(prevMemResource);
                    ptrInfo.processOwner(TPtrManager<T>::getPtr(obj));
                }

                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContext& plc, PLCInfoDeserializer& ptrInfo, Des&, T& obj, Fnc&& fnc,
                                     Reader&, std::false_type, OwnershipType<PointerOwnershipType::Owner>) const {
                    auto ptr = TPtrManager<T>::getPtr(obj);
                    if (ptr) {
                        fnc(*ptr);
                    } else {
                        auto prevMemResource = plc.getMemResource();
                        if (_resource) plc.setMemResource(_resource);
                        TPtrManager<T>::create(obj, plc, RTTI::template get<typename TPtrManager<T>::TElement>());
                        if (!_resourcePropagate) plc.setMemResource(prevMemResource);
                        ptr = TPtrManager<T>::getPtr(obj);
                        fnc(*ptr);
                        plc.setMemResource(prevMemResource);
                    }
                    ptrInfo.processOwner(ptr);
                }

                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContext& plc, PLCInfoDeserializer& ptrInfo, Des& des, T& obj, Fnc&&,
                                     Reader& r, std::true_type,
                                     OwnershipType<PointerOwnershipType::SharedOwner>) const {
                    auto& sharedState = ptrInfo.sharedState;
                    if (!sharedState) {
                        const auto& ctx = des.template context<TPolymorphicContext<RTTI>>();
                        auto prevMemResource = plc.getMemResource();
                        ctx->deserialize(des, r, TPtrManager<T>::getPtr(obj),
                                         [&obj, &plc, &sharedState, this, prevMemResource](
                                             const std::shared_ptr<PolymorphicHandlerBase>& handler) {
                                             if (_resource) plc.setMemResource(_resource);
                                             sharedState = TPtrManager<T>::createSharedPolymorphic(obj, plc, handler);
                                             if (!_resourcePropagate) plc.setMemResource(prevMemResource);
                                             return TPtrManager<T>::getPtr(obj);
                                         },
                                         [&obj, &plc, this](const std::shared_ptr<PolymorphicHandlerBase>& handler) {
                                             if (_resource) plc.setMemResource(_resource);
                                             TPtrManager<T>::destroyPolymorphic(obj, plc, handler);
                                         });
                        plc.setMemResource(prevMemResource);
                        if (!sharedState)
                            sharedState = TPtrManager<T>::getSharedState(obj);
                    }
                    TPtrManager<T>::loadFromSharedState(sharedState.get(), obj);
                    ptrInfo.processOwner(TPtrManager<T>::getPtr(obj));
                }

                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContext& plc, PLCInfoDeserializer& ptrInfo, Des&, T& obj, Fnc&& fnc,
                                     Reader&, std::false_type, OwnershipType<PointerOwnershipType::SharedOwner>) const {
                    auto& sharedState = ptrInfo.sharedState;
                    if (!sharedState) {
                        auto ptr = TPtrManager<T>::getPtr(obj);
                        auto prevMemResource = plc.getMemResource();
                        if (ptr) {
                            sharedState = TPtrManager<T>::getSharedState(obj);
                        } else {
                            if (_resource) plc.setMemResource(_resource);
                            sharedState = TPtrManager<T>::createShared(obj, plc,
                                                                       RTTI::template get<typename TPtrManager<T>::TElement>());
                            if (!_resourcePropagate) plc.setMemResource(prevMemResource);
                            ptr = TPtrManager<T>::getPtr(obj);
                        }
                        fnc(*ptr);
                        plc.setMemResource(prevMemResource);
                    }
                    TPtrManager<T>::loadFromSharedState(sharedState.get(), obj);
                    ptrInfo.processOwner(TPtrManager<T>::getPtr(obj));
                }

                template<typename Des, typename T, typename Fnc, typename Reader, typename isPolymorph>
                void deserializeImpl(PointerLinkingContext& plc, PLCInfoDeserializer& ptrInfo, Des& des, T& obj,
                                     Fnc&& fnc, Reader& r, isPolymorph polymorph,
                                     OwnershipType<PointerOwnershipType::SharedObserver>) const {
                    deserializeImpl(plc, ptrInfo, des, obj, fnc, r, polymorph,
                                    OwnershipType<PointerOwnershipType::SharedOwner>{});
                }

                template<typename Des, typename T, typename Fnc, typename Reader, typename isPolymorphic>
                void deserializeImpl(PointerLinkingContext&, PLCInfoDeserializer& ptrInfo, Des&, T& obj, Fnc&&,
                                     Reader&, isPolymorphic, OwnershipType<PointerOwnershipType::Observer>) const {
                    ptrInfo.processObserver(reinterpret_cast<void*&>(TPtrManager<T>::getPtrRef(obj)));
                }


                PointerType _ptrType;
                bool _resourcePropagate;
                bitsery::ext::MemResourceBase* _resource;
            };

        }

    }
}

#endif //BITSERY_POINTER_UTILS_H
