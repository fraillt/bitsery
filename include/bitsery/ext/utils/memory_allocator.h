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

#ifndef BITSERY_EXT_MEMORY_ALLOCATOR_H
#define BITSERY_EXT_MEMORY_ALLOCATOR_H

#include "../../details/serialization_common.h"
#include <new>

namespace bitsery {
    namespace ext {
        // these are very similar to c++17 polymorphic allocator and memory resource classes
        // but i don't want to enforce users to use c++17 if they want to use pointers
        // plus this has additional information from RTTI about runtime type information,
        // might be useful working with polymorphic types

        class MemResourceBase {
        public:
            virtual void* allocate(size_t bytes, size_t alignment, size_t typeId) = 0;

            virtual void deallocate(void* ptr, size_t bytes, size_t alignment, size_t typeId) = 0;

            virtual ~MemResourceBase() noexcept = default;
        };

        class MemResourceNewDelete : public MemResourceBase {
        public:
            inline void* allocate(size_t bytes, size_t /*alignment*/, size_t /*typeId*/) final {
                return (::operator new(bytes));
            }

            inline void deallocate(void* ptr, size_t /*bytes*/, size_t /*alignment*/, size_t /*typeId*/) final {
                (::operator delete(ptr));
            }

            ~MemResourceNewDelete() noexcept final = default;
        };

        class PolymorphicAllocator {
        public:

            template<typename T>
            T* allocate(size_t typeId) const {
                constexpr auto bytes = sizeof(T);
                constexpr auto alignment = std::alignment_of<T>::value;
                void* ptr = _resource
                            ? _resource->allocate(bytes, alignment, typeId)
                            : MemResourceNewDelete{}.allocate(bytes, alignment, typeId);
                return ::bitsery::Access::create<T>(ptr);
            }

            template<typename T>
            void deallocate(T* ptr, size_t typeId) const {
                constexpr auto bytes = sizeof(T);
                constexpr auto alignment = std::alignment_of<T>::value;
                ptr->~T();
                _resource
                ? _resource->deallocate(ptr, bytes, alignment, typeId)
                : MemResourceNewDelete{}.deallocate(ptr, bytes, alignment, typeId);
            }

            void setMemResource(MemResourceBase* resource) {
                _resource = resource;
            }

            MemResourceBase* getMemResource() const {
                return _resource;
            }

        private:
            MemResourceBase* _resource{nullptr};
        };

    }
}
#endif //BITSERY_EXT_MEMORY_ALLOCATOR_H
