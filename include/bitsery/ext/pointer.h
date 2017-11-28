//MIT License
//
//Copyright (c) 2017 Mindaugas Vinkelis
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

#ifndef BITSERY_EXT_POINTER_H
#define BITSERY_EXT_POINTER_H

#include "../traits/core/traits.h"
#include "utils/pointer_utils.h"
#include "utils/rtti_utils.h"

namespace bitsery {

    namespace ext {

        namespace details_pointer {

            template<typename S>
            PointerLinkingContext &getLinkingContext(S &s) {
                auto res = s.template context<PointerLinkingContext>();
                assert(res != nullptr);
                return *res;
            }


            template<typename TObject>
            struct RawPointerObjectHandler {

                using TPointer = TObject;

                template<typename T>
                void create(TObject &obj) const {
                    obj = new T{};
                }

                void destroy(TObject &obj) const {
                    delete obj;
                    obj = nullptr;
                }

                const TPointer getPtr(const TObject &obj) const {
                    return obj;
                }

                TPointer getPtr(TObject &obj) const {
                    return obj;
                }

            };

            template <typename TObject>
            struct RawPointerManagerConfig {
                using RTTI = bitsery::ext::utils::StandardRTTI;
                static constexpr PointerOwnershipType OwnershipType = PointerOwnershipType::Owner;

                using Handler = RawPointerObjectHandler<TObject>;

                static std::unique_ptr<utils::PointerSharedContextBase> createSharedContext(TObject &) {
                    return {};
                }

                static void restoreFromSharedContext(TObject &, utils::PointerSharedContextBase *) {

                }
            };
        }

        class PointerOwner : public utils::PointerOwnerManager<details_pointer::RawPointerManagerConfig> {
        public:
            explicit PointerOwner(PointerType ptrType = PointerType::Nullable) : PointerOwnerManager(ptrType) {}
        };

        class PointerObserver {
        public:

            explicit PointerObserver(PointerType ptrType = PointerType::Nullable) : _ptrType{ptrType} {}

            template<typename Ser, typename Writer, typename T, typename Fnc>
            void serialize(Ser &ser, Writer &w, const T &obj, Fnc &&) const {
                auto &ctx = details_pointer::getLinkingContext(ser);
                if (obj) {
                    details::writeSize(w, ctx.getInfoByPtr(obj, PointerOwnershipType::Observer).id);
                } else {
                    assert(_ptrType == PointerType::Nullable);
                    details::writeSize(w, 0);
                }
            }

            template<typename Des, typename Reader, typename T, typename Fnc>
            void deserialize(Des &des, Reader &r, T &obj, Fnc &&) const {
                size_t id{};
                details::readSize(r, id, std::numeric_limits<size_t>::max());
                if (id) {
                    auto &ctx = details_pointer::getLinkingContext(des);
                    ctx.getInfoById(id, PointerOwnershipType::Observer).processObserver(reinterpret_cast<void *&>(obj));
                } else {
                    if (_ptrType == PointerType::Nullable)
                        obj = nullptr;
                    else
                        r.setError(ReaderError::InvalidPointer);
                }
            }

        private:
            PointerType _ptrType;
        };

        class ReferencedByPointer {
        public:
            template<typename Ser, typename Writer, typename T, typename Fnc>
            void serialize(Ser &ser, Writer &w, const T &obj, Fnc &&fnc) const {
                auto &ctx = details_pointer::getLinkingContext(ser);
                details::writeSize(w, ctx.getInfoByPtr(&obj, PointerOwnershipType::Owner).id);
                fnc(const_cast<T &>(obj));
            }

            template<typename Des, typename Reader, typename T, typename Fnc>
            void deserialize(Des &des, Reader &r, T &obj, Fnc &&fnc) const {
                size_t id{};
                details::readSize(r, id, std::numeric_limits<size_t>::max());
                if (id) {
                    auto &ctx = details_pointer::getLinkingContext(des);
                    fnc(obj);
                    ctx.getInfoById(id, PointerOwnershipType::Owner).processOwner(&obj);
                } else {
                    //cannot be null for references
                    r.setError(ReaderError::InvalidPointer);
                }
            }
        };

    }

    namespace traits {

        template<typename T>
        struct ExtensionTraits<ext::PointerOwner, T *> {
            using TValue = T;
            static constexpr bool SupportValueOverload = true;
            static constexpr bool SupportObjectOverload = true;
            //pointers cannot have lamba overload, when polymorphism support will be added
            static constexpr bool SupportLambdaOverload = false;
        };

        template<typename T>
        struct ExtensionTraits<ext::PointerObserver, T *> {
            //although pointer observer doesn't serialize anything, but we still add value overload support to be consistent with pointer owners
            using TValue = T;
            static constexpr bool SupportValueOverload = true;
            static constexpr bool SupportObjectOverload = true;
            //pointers cannot have lamba overload, when polymorphism support will be added
            static constexpr bool SupportLambdaOverload = false;
        };

        template<typename T>
        struct ExtensionTraits<ext::ReferencedByPointer, T> {
            //allow everything, because it is serialized as regular type, except it also creates pointerId that is required by NonOwningPointer to work
            using TValue = T;
            static constexpr bool SupportValueOverload = true;
            static constexpr bool SupportObjectOverload = true;
            static constexpr bool SupportLambdaOverload = true;
        };
    }

}


#endif //BITSERY_EXT_POINTER_H
