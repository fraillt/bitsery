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

#ifndef BITSERY_EXT_STD_SMART_PTR_H
#define BITSERY_EXT_STD_SMART_PTR_H

#include <cassert>
#include "../traits/core/traits.h"
#include "utils/pointer_utils.h"
#include "utils/polymorphism_utils.h"
#include "utils/rtti_utils.h"
#include <memory>

namespace bitsery {
    namespace ext {

        namespace smart_ptr_details {

            //further code is for managing shared ownership
            //do not nest this type in pointer manager class itself, because it will be different type for different T
            struct SharedPtrSharedState : pointer_utils::PointerSharedStateBase {
                std::shared_ptr<void> obj{};
            };

            template<typename T>
            struct SmartPtrOwnerManager {

                using TElement = typename T::element_type;

                template<typename TDeleter>
                static TElement* getPtr(std::unique_ptr<TElement, TDeleter>& obj) {
                    return obj.get();
                }

                static TElement* getPtr(std::shared_ptr<TElement>& obj) {
                    return obj.get();
                }

                static TElement* getPtr(std::weak_ptr<TElement>& obj) {
                    if (auto ptr = obj.lock())
                        return ptr.get();
                    return nullptr;
                }

                static constexpr PointerOwnershipType getOwnership() {
                    return ::bitsery::details::IsSpecializationOf<T, std::unique_ptr>::value
                           ? PointerOwnershipType::Owner
                           : std::is_same<std::shared_ptr<TElement>, T>::value
                             ? PointerOwnershipType::SharedOwner
                             : PointerOwnershipType::SharedObserver;
                }

                template<typename TDeleter>
                static void create(std::unique_ptr<TElement, TDeleter>& obj, PolymorphicAllocator& alloc,
                                   size_t typeId) {
                    obj.reset(alloc.allocate<TElement>(typeId));
                }

                template<typename TDeleter>
                static void createPolymorphic(std::unique_ptr<TElement, TDeleter>& obj, PolymorphicAllocator& alloc,
                                              const std::shared_ptr<PolymorphicHandlerBase>& handler) {
                    obj.reset(static_cast<TElement*>(handler->create(alloc)));
                }

                template<typename TDel>
                static void destroy(std::unique_ptr<TElement, TDel>& obj, PolymorphicAllocator& alloc, size_t typeId) {
                    uniquePtrDestroy(obj, alloc, typeId, std::is_same<std::unique_ptr<TElement>, T>{});
                }

                template<typename TDel>
                static void destroyPolymorphic(std::unique_ptr<TElement, TDel>& obj, PolymorphicAllocator& alloc,
                                               const std::shared_ptr<PolymorphicHandlerBase>& handler) {
                    uniquePtrDestroyPolymorphic(obj, alloc, handler, std::is_same<std::unique_ptr<TElement>, T>{});
                }

                static void destroy(std::shared_ptr<TElement>& obj, PolymorphicAllocator&, size_t) {
                    obj.reset();
                }

                static void destroyPolymorphic(std::shared_ptr<TElement>& obj, PolymorphicAllocator&,
                                               const std::shared_ptr<PolymorphicHandlerBase>&) {
                    obj.reset();
                }

                static void destroy(std::weak_ptr<TElement>& obj, PolymorphicAllocator&, size_t) {
                    obj.reset();
                }

                static void destroyPolymorphic(std::weak_ptr<TElement>& obj, PolymorphicAllocator&,
                                               const std::shared_ptr<PolymorphicHandlerBase>&) {
                    obj.reset();
                }

                static std::unique_ptr<pointer_utils::PointerSharedStateBase> createShared(
                    std::shared_ptr<TElement>& obj, PolymorphicAllocator& alloc, size_t typeId) {
                    // capture deleter parameters by value
                    obj = std::shared_ptr<TElement>(alloc.allocate<TElement>(typeId),
                                                    [&alloc, typeId](TElement* data) {
                                                        alloc.deallocate(data, typeId);
                                                    });
                    auto state = new SharedPtrSharedState{};
                    state->obj = obj;
                    return std::unique_ptr<pointer_utils::PointerSharedStateBase>{state};
                }

                static std::unique_ptr<pointer_utils::PointerSharedStateBase> createSharedPolymorphic(
                    std::shared_ptr<TElement>& obj, PolymorphicAllocator& alloc,
                    const std::shared_ptr<PolymorphicHandlerBase>& handler) {
                    // capture deleter parameters by value
                    obj = std::shared_ptr<TElement>(static_cast<TElement*>(handler->create(alloc)),
                                                    [alloc, handler](TElement* data) {
                                                        handler->destroy(alloc, data);
                                                    });
                    auto state = new SharedPtrSharedState{};
                    state->obj = obj;
                    return std::unique_ptr<pointer_utils::PointerSharedStateBase>{state};
                }

                static std::unique_ptr<pointer_utils::PointerSharedStateBase> createShared(
                    std::weak_ptr<TElement>& obj, PolymorphicAllocator& alloc, size_t typeId) {
                    auto res = std::shared_ptr<TElement>(alloc.allocate<TElement>(typeId),
                                                         [alloc, typeId](TElement* data) {
                                                             alloc.deallocate(data, typeId);
                                                         });
                    obj = res;
                    auto state = new SharedPtrSharedState{};
                    state->obj = res;
                    return std::unique_ptr<pointer_utils::PointerSharedStateBase>{state};
                }

                static std::unique_ptr<pointer_utils::PointerSharedStateBase> createSharedPolymorphic(
                    std::weak_ptr<TElement>& obj, PolymorphicAllocator& alloc,
                    const std::shared_ptr<PolymorphicHandlerBase>& handler) {
                    auto res = std::shared_ptr<TElement>(static_cast<TElement*>(handler->create(alloc)),
                                                         [alloc, handler](TElement* data) {
                                                             handler->destroy(alloc, data);
                                                         });
                    obj = res;
                    auto state = new SharedPtrSharedState{};
                    state->obj = res;
                    return std::unique_ptr<pointer_utils::PointerSharedStateBase>{state};
                }

                static std::unique_ptr<pointer_utils::PointerSharedStateBase> getSharedState(T& obj) {
                    auto state = new SharedPtrSharedState{};
                    //to work with weak_ptr and shared_ptr create new std::shared_ptr
                    state->obj = std::shared_ptr<TElement>(obj);
                    return std::unique_ptr<pointer_utils::PointerSharedStateBase>{state};
                }

                static void loadFromSharedState(pointer_utils::PointerSharedStateBase* ctx, T& obj) {
                    auto state = dynamic_cast<SharedPtrSharedState*>(ctx);
                    //reinterpret_pointer_cast is only since c++17
                    auto p = reinterpret_cast<TElement*>(state->obj.get());
                    obj = std::shared_ptr<TElement>(state->obj, p);
                }

            private:
                template<typename TDel>
                static void
                uniquePtrDestroy(std::unique_ptr<TElement, TDel>& obj, PolymorphicAllocator& alloc, size_t typeId,
                                 std::true_type) {
                    auto ptr = obj.release();
                    alloc.deallocate(ptr, typeId);
                }

                template<typename TDel>
                static void
                uniquePtrDestroyPolymorphic(std::unique_ptr<TElement, TDel>& obj, PolymorphicAllocator& alloc,
                                            const std::shared_ptr<PolymorphicHandlerBase>& handler, std::true_type) {
                    auto ptr = obj.release();
                    handler->destroy(alloc, ptr);
                }

                template<typename TDel>
                static void
                uniquePtrDestroy(std::unique_ptr<TElement, TDel>& obj, PolymorphicAllocator&, size_t,
                                 std::false_type) {
                    obj.reset();
                }

                template<typename TDel>
                static void
                uniquePtrDestroyPolymorphic(std::unique_ptr<TElement, TDel>& obj, PolymorphicAllocator&,
                                            const std::shared_ptr<PolymorphicHandlerBase>&, std::false_type) {
                    obj.reset();
                }

            };
        }

        template<typename RTTI>
        using StdSmartPtrBase = pointer_utils::PointerObjectExtensionBase<
            smart_ptr_details::SmartPtrOwnerManager, PolymorphicContext, RTTI>;

        //helper type for convienience
        using StdSmartPtr = StdSmartPtrBase<StandardRTTI>;

    }

    namespace traits {

        template<typename T, typename RTTI>
        struct ExtensionTraits<ext::StdSmartPtrBase<RTTI>, T> {
            using TValue = typename T::element_type;
            static constexpr bool SupportValueOverload = true;
            static constexpr bool SupportObjectOverload = true;
            //if underlying type is not polymorphic, then we can enable lambda syntax
            static constexpr bool SupportLambdaOverload = !RTTI::template isPolymorphic<TValue>();
        };

    }

}

#endif //BITSERY_EXT_STD_SMART_PTR_H
