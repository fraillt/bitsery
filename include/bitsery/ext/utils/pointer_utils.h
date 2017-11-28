//
// Created by fraillt on 17.11.30.
//

#ifndef BITSERY_POINTER_UTILS_H
#define BITSERY_POINTER_UTILS_H

#include <unordered_map>
#include <vector>
#include <memory>

#include "polymorphism_utils.h"

namespace bitsery {
    namespace ext {

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

        namespace utils {
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
            struct PointerSharedContextBase {
                virtual ~PointerSharedContextBase() = default;
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
                              sharedContext{} {};

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
                    std::unique_ptr<PointerSharedContextBase> sharedContext;
                };

                PointerInfo &getInfoById(size_t id, PointerOwnershipType ptrType) {
                    auto res = _idMap.emplace(id, PointerInfo{id, nullptr, ptrType});
                    auto &ptrInfo = res.first->second;
                    if (!res.second) {
                        assert(ptrType != PointerOwnershipType::Owner ||
                               ptrInfo.ownershipType == PointerOwnershipType::Observer);
                        if (ptrInfo.ownershipType == PointerOwnershipType::Observer)
                            ptrInfo.ownershipType = ptrType;
                    }
                    return ptrInfo;
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

            template<template<typename> class Config>
            class PointerOwnerManager {

                template <typename TObject>
                using Handler = typename Config<TObject>::Handler;

                template<typename TObject>
                struct HelperTypes {
                    using RTTI = typename Config<TObject>::RTTI;
                    using THandler = Handler<TObject>;
                    using TValue = typename std::remove_pointer<typename THandler::TPointer>::type;
                };

                template<typename Ser, typename T, typename Fnc>
                void serializeImpl(PointerLinkingContextSerialization &, Ser &ser, const T &obj, Fnc &&,
                                   std::true_type) const {
                    InheritanceTreeSerialize<Ser, T, HelperTypes> tree{};
                    auto handler = tree.getHandler(obj);

                    assert(handler.second);
                    ser.object(handler.first);
                    handler.second->process(ser, const_cast<T &>(obj));
                }

                template<typename Ser, typename T, typename Fnc>
                void serializeImpl(PointerLinkingContextSerialization &, Ser &, const T &obj, Fnc &&fnc,
                                   std::false_type) const {
                    auto handler = Handler<T>{};
                    fnc(*handler.getPtr(const_cast<T &>(obj)));
                }

                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContextDeserialization &, Des &des, T &obj, Fnc &&,
                                     Reader &r, std::true_type) const {
                    InheritanceTreeTypeId id{};
                    des.object(id);

                    InheritanceTreeDeserialize<Des, T, HelperTypes> tree{};
                    auto handler = tree.getHandler(id);
                    if (handler) {
                        handler->create(obj);
                        handler->process(des, obj);
                    } else {
                        r.setError(ReaderError::InvalidPointer);
                    }
                }

                template<typename Des, typename T, typename Fnc, typename Reader>
                void deserializeImpl(PointerLinkingContextDeserialization &, Des &, T &obj, Fnc &&fnc,
                                     Reader &, std::false_type) const {
                    using TValue = typename HelperTypes<T>::TValue;
                    auto handler = Handler<T>{};
                    if (auto ptr = handler.getPtr(obj)) {
                        fnc(*ptr);
                    } else {
                        handler.template create<TValue>(obj);
                        fnc(*handler.getPtr(obj));
                    }
                }

                PointerType _ptrType;
            public:

                explicit PointerOwnerManager(PointerType ptrType = PointerType::Nullable) : _ptrType{ptrType} {}

                template<typename Ser, typename Writer, typename T, typename Fnc>
                void serialize(Ser &ser, Writer &w, const T &obj, Fnc &&fnc) const {
                    auto handler = Handler<T>{};
                    auto ptr = handler.getPtr(obj);
                    if (ptr) {
                        auto ctx = ser.template context<PointerLinkingContext>();
                        assert(ctx != nullptr);
                        auto &ptrInfo = ctx->getInfoByPtr(ptr, Config<T>::OwnershipType);
                        details::writeSize(w, ptrInfo.id);
                        if (ptrInfo.sharedCount == 0) {
                            serializeImpl(*ctx, ser, obj, std::forward<Fnc>(fnc),
                                          std::is_polymorphic<typename HelperTypes<T>::TValue>{});
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
                    auto handler = Handler<T>{};
                    if (id) {
                        auto ctx = des.template context<PointerLinkingContext>();
                        assert(ctx != nullptr);
                        auto &ptrInfo = ctx->getInfoById(id, Config<T>::OwnershipType);
                        //todo add deserialization checking
                        if (ptrInfo.ownershipType == PointerOwnershipType::Owner) {
                            deserializeImpl(*ctx, des, obj, std::forward<Fnc>(fnc), r,
                                            std::is_polymorphic<typename HelperTypes<T>::TValue>{});
                            ptrInfo.processOwner(handler.getPtr(obj));
                        } else {
                            if (!ptrInfo.sharedContext) {
                                deserializeImpl(*ctx, des, obj, std::forward<Fnc>(fnc), r,
                                                std::is_polymorphic<typename HelperTypes<T>::TValue>{});
                                ptrInfo.processOwner(handler.getPtr(obj));
                                ptrInfo.sharedContext = Config<T>::createSharedContext(obj);
                            } else {
                                Config<T>::restoreFromSharedContext(obj, ptrInfo.sharedContext.get());
                            }
                        }

                    } else {
                        if (_ptrType == PointerType::Nullable && handler.getPtr(obj)) {
                            handler.destroy(obj);
                        } else
                            r.setError(ReaderError::InvalidPointer);
                    }
                }

            };

        }

        //this class is for convenience
        class PointerLinkingContext :
                public utils::PointerLinkingContextSerialization,
                public utils::PointerLinkingContextDeserialization {
        public:
            bool isValid() {
                return isPointerSerializationValid() && isPointerDeserializationValid();
            }
        };

    }
}

#endif //BITSERY_POINTER_UTILS_H
