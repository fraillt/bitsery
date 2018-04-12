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
#include "../../details/adapter_utils.h"

namespace bitsery {
    namespace ext {

        //change name
        enum class PointerType {
            Nullable,
            NotNull
        };

        enum class PointerOwnershipType : uint8_t {
            //is not responsible for pointer lifetime management.
                    Observer,
            //only ONE owner is responsible for this pointers creation/destruction
                    Owner,
            //MANY shared owners is responsible for pointer creation/destruction
            //requires additional context to manage shared owners themselves.
                    Shared
        };

        //forward declaration
        class PointerLinkingContext;

        namespace pointer_utils {
            
            enum SharedSerializationStatus {
                NotSerialized,
                SerializedWeak,
                SerializedShared
            };
            
            class PointerLinkingContextSerialization {
            public:
                explicit PointerLinkingContextSerialization()
                        : _currId{0},
                          _ptrMap{} {}

                PointerLinkingContextSerialization(const PointerLinkingContextSerialization &) = delete;

                PointerLinkingContextSerialization &operator=(const PointerLinkingContextSerialization &) = delete;

                PointerLinkingContextSerialization(PointerLinkingContextSerialization &&) = default;

                PointerLinkingContextSerialization &operator=(PointerLinkingContextSerialization &&) = default;

                ~PointerLinkingContextSerialization() = default;

                struct PointerInfo {
                    PointerInfo(size_t id_, PointerOwnershipType ownershipType_)
                            : id{id_},
                              ownershipType{ownershipType_},
                              sharedCount{0} {};
                    size_t id;
                    PointerOwnershipType ownershipType;
                    size_t sharedCount;
                };

                const PointerInfo &getInfoByPtr(const void *ptr, PointerOwnershipType ptrType) {
                    auto res = _ptrMap.emplace(ptr, PointerInfo{_currId + 1u, ptrType});
                    auto &ptrInfo = res.first->second;
                    if (res.second) {
                        ++_currId;
                        return ptrInfo;
                    }
                    //ptr already exists
                    //for observer return success
                    if (ptrType == PointerOwnershipType::Observer)
                        return ptrInfo;
                    //set owner and return success
                    if (ptrInfo.ownershipType == PointerOwnershipType::Observer) {
                        ptrInfo.ownershipType = ptrType;
                        return ptrInfo;
                    }
                    //only shared ownership can get here multiple times
                    assert(ptrType == PointerOwnershipType::Shared);
                    ptrInfo.sharedCount++;
                    return ptrInfo;
                }

                //valid, when all pointers have owners.
                //we cannot serialize pointers, if we haven't serialized objects themselves
                bool isPointerSerializationValid() const {
                    return std::all_of(_ptrMap.begin(), _ptrMap.end(),
                                       [](const std::pair<const void *, PointerInfo> &p) {
                                           return p.second.ownershipType != PointerOwnershipType::Observer;
                                       });
                }

            private:
                size_t _currId;
                std::unordered_map<const void *, PointerInfo> _ptrMap;

            };

            //this class is used to store context for shared ptr owners
            struct PointerSharedStateBase {
                virtual ~PointerSharedStateBase() = default;
            };

            class PointerLinkingContextDeserialization {
            public:
                explicit PointerLinkingContextDeserialization()
                        : _idMap{} {}

                PointerLinkingContextDeserialization(const PointerLinkingContextDeserialization &) = delete;

                PointerLinkingContextDeserialization &operator=(const PointerLinkingContextDeserialization &) = delete;

                PointerLinkingContextDeserialization(PointerLinkingContextDeserialization &&) = default;

                PointerLinkingContextDeserialization &operator=(PointerLinkingContextDeserialization &&) = default;

                ~PointerLinkingContextDeserialization() = default;


                struct PointerInfo {
                    PointerInfo(size_t id_, void *ptr, PointerOwnershipType ownershipType_)
                            : id{id_},
                              ownershipType{ownershipType_},
                              ownerPtr{ptr},
                              observersList{},
                              sharedContext{},
                              sharedCount{}
                    {};

                    PointerInfo(const PointerInfo &) = delete;

                    PointerInfo &operator=(const PointerInfo &) = delete;

                    PointerInfo(PointerInfo &&) = default;

                    PointerInfo &operator=(PointerInfo &&) = default;

                    ~PointerInfo() = default;

                    void processOwner(void *ptr) {
                        ownerPtr = ptr;
                        assert(ownershipType != PointerOwnershipType::Observer);
                        for (auto &o:observersList)
                            o.get() = ptr;
                        observersList.clear();
                        observersList.shrink_to_fit();
                    }

                    void processObserver(void *(&ptr)) {
                        if (ownerPtr) {
                            ptr = ownerPtr;
                        } else {
                            observersList.push_back(ptr);
                        }
                    }

                    size_t id;
                    PointerOwnershipType ownershipType;
                    void *ownerPtr;
                    std::vector<std::reference_wrapper<void *>> observersList;
                    std::unique_ptr<PointerSharedStateBase> sharedContext;
                    size_t sharedCount;
                };

                PointerInfo &getInfoById(size_t id, PointerOwnershipType ptrType) {
                    auto res = _idMap.emplace(id, PointerInfo{id, nullptr, ptrType});
                    auto &ptrInfo = res.first->second;
                    if (!res.second) {
                        //id already exists
                        //for observer return success
                        if (ptrType == PointerOwnershipType::Observer)
                            return ptrInfo;

                        //set owner and return success
                        if (ptrInfo.ownershipType == PointerOwnershipType::Observer) {
                            ptrInfo.ownershipType = ptrType;
                            return ptrInfo;
                        }
                        //only shared ownership can get here multiple times
                        assert(ptrType == PointerOwnershipType::Shared);
                        ptrInfo.sharedCount++;
                    }
                    return ptrInfo;
                }

                void clearSharedState() {
                    _idMap.clear();
                }

                //valid, when all pointers has owners
                bool isPointerDeserializationValid() const {
                    return std::all_of(_idMap.begin(), _idMap.end(), [](const std::pair<const size_t, PointerInfo> &p) {
                        return p.second.ownershipType != PointerOwnershipType::Observer;
                    });
                }

            private:
                std::unordered_map<size_t, PointerInfo> _idMap;
            };

            template<template <typename> typename TPtrManager, template <typename> typename TPolymorphicContext, typename RTTI>
            class PointerObjectExtensionBase {
            public:

                explicit PointerObjectExtensionBase(PointerType ptrType = PointerType::Nullable) :
                        _ptrType{ptrType}
                {}

                template<typename Ser, typename Writer, typename T, typename Fnc>
                void serialize(Ser &ser, Writer &w, const T &obj, Fnc &&fnc) const {

                    auto ptr = TPtrManager<T>::getPtr(const_cast<T&>(obj));
                    if (ptr) {
                        auto ctx = ser.template context<PointerLinkingContext>();
                        assert(ctx != nullptr);
                        auto &ptrInfo = ctx->getInfoByPtr(getBasePtr(ptr), TPtrManager<T>::getOwnership());
                        details::writeSize(w, ptrInfo.id);
                        if (TPtrManager<T>::getOwnership() != PointerOwnershipType::Observer) {
                            if (ptrInfo.sharedCount == 0)
                                serializeImpl(ser, ptr, std::forward<Fnc>(fnc), w, IsPolymorphic<T>{});
                        }
                    } else {
                        assert(_ptrType == PointerType::Nullable);
                        details::writeSize(w, 0);
                    }

                }

                template<typename Des, typename Reader, typename T, typename Fnc>
                void deserialize(Des &des, Reader &r, T &obj, Fnc &&fnc) const {
                    size_t id{};
                    details::readSize(r, id, std::numeric_limits<size_t>::max());
                    if (id) {
                        auto ctx = des.template context<PointerLinkingContext>();
                        assert(ctx != nullptr);
                        auto &ptrInfo = ctx->getInfoById(id, TPtrManager<T>::getOwnership());
                        deserializeImpl(ptrInfo, des, obj, std::forward<Fnc>(fnc), r, IsPolymorphic<T>{},
                                        std::integral_constant<PointerOwnershipType, TPtrManager<T>::getOwnership()>{});
                    } else {
                        if (_ptrType == PointerType::Nullable) {
                            TPtrManager<T>::clear(obj);
                        } else
                            r.setError(ReaderError::InvalidPointer);
                    }
                }

            private:
                
                template <typename T>
                struct IsPolymorphic:std::integral_constant<bool, RTTI::template isPolymorphic<typename TPtrManager<T>::TElement>()> {
                };
                
                template <typename T>
                const void* getBasePtr(const T* ptr) const {
                    // todo implement handling of types with virtual inheritance
                    // this is required to correctly track same shared object, e.g. shared_ptr<Base> and shared_ptr<Derived>
                    return ptr;
                }
                
                template<typename Ser, typename TPtr, typename Fnc, typename Writer>
                void serializeImpl(Ser &ser, TPtr &ptr, Fnc &&, Writer &w, std::true_type) const {
                    const auto &ctx = ser.template context<TPolymorphicContext<RTTI>>();
                    ctx->serialize(ser, w, *ptr);
                }

                template<typename Ser, typename TPtr, typename Fnc, typename Writer>
                void serializeImpl(Ser &, TPtr &ptr, Fnc &&fnc, Writer &, std::false_type) const {
                    fnc(*ptr);
                }
                
                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContextDeserialization::PointerInfo& ptrInfo, Des &des, T &obj, Fnc &&,
                                     Reader &r, std::true_type polymorph, std::integral_constant<PointerOwnershipType, PointerOwnershipType::Owner>) const {
                    const auto &ctx = des.template context<TPolymorphicContext<RTTI>>();
                    ctx->deserialize(des,r, TPtrManager<T>::getPtr(obj),
                                     [&obj, this](typename TPtrManager<T>::TElement *valuePtr) {
                                         TPtrManager<T>::assign(obj, valuePtr);
                                     });
                    ptrInfo.processOwner(TPtrManager<T>::getPtr(obj));
                }
                
                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContextDeserialization::PointerInfo& ptrInfo, Des &, T &obj, Fnc &&fnc,
                                     Reader &, std::false_type polymorph, std::integral_constant<PointerOwnershipType, PointerOwnershipType::Owner>) const {
                    auto ptr = TPtrManager<T>::getPtr(obj);
                    if (ptr) {
                        fnc(*ptr);
                    } else {
                        ptr = new typename TPtrManager<T>::TElement{};
                        fnc(*ptr);
                        TPtrManager<T>::assign(obj, ptr);
                    }
                    ptrInfo.processOwner(ptr);
                }
                
                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContextDeserialization::PointerInfo& ptrInfo, Des &des, T &obj, Fnc &&,
                                     Reader &r, std::true_type polymorph, std::integral_constant<PointerOwnershipType, PointerOwnershipType::Shared> ) const {
                    auto& sharedState = ptrInfo.sharedContext;
                    if (!sharedState) {
                        const auto &ctx = des.template context<TPolymorphicContext<RTTI>>();
                        ctx->deserialize(des,r, TPtrManager<T>::getPtr(obj),
                                         [&obj, &sharedState](typename TPtrManager<T>::TElement *valuePtr) {
                                             sharedState = TPtrManager<T>::createSharedState(valuePtr);
                                         });
                        if (!sharedState)
                            sharedState = TPtrManager<T>::saveToSharedState(obj);
                    }
                    TPtrManager<T>::loadFromSharedState(sharedState.get(), obj);
                    ptrInfo.processOwner(TPtrManager<T>::getPtr(obj));
                }
                
                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContextDeserialization::PointerInfo& ptrInfo, Des &, T &obj, Fnc &&fnc,
                                     Reader &, std::false_type polymorph, std::integral_constant<PointerOwnershipType, PointerOwnershipType::Shared>) const {
                    auto& sharedState = ptrInfo.sharedContext;
                    if (!sharedState) {
                        if (auto ptr = TPtrManager<T>::getPtr(obj)) {
                            fnc(*ptr);
                            sharedState = TPtrManager<T>::saveToSharedState(obj);
                        } else {
                            auto res = new typename TPtrManager<T>::TElement{};
                            fnc(*res);
                            sharedState = TPtrManager<T>::createSharedState(res);
                        }
                    }
                    TPtrManager<T>::loadFromSharedState(sharedState.get(), obj);
                    ptrInfo.processOwner(TPtrManager<T>::getPtr(obj));
                }
                
                template<typename Des, typename T, typename Fnc, typename Reader, typename isPolymorphic>
                void deserializeImpl(PointerLinkingContextDeserialization::PointerInfo& ptrInfo, Des &, T &obj, Fnc &&fnc,
                                     Reader &, isPolymorphic, std::integral_constant<PointerOwnershipType, PointerOwnershipType::Observer>) const {
                    ptrInfo.processObserver(reinterpret_cast<void *&>(TPtrManager<T>::getPtrRef(obj)));
                }

                PointerType _ptrType;
            };

        }

        //this class is for convenience
        class PointerLinkingContext :
                public pointer_utils::PointerLinkingContextSerialization,
                public pointer_utils::PointerLinkingContextDeserialization {
        public:
            bool isValid() {
                return isPointerSerializationValid() && isPointerDeserializationValid();
            }
        };

    }
}

#endif //BITSERY_POINTER_UTILS_H
