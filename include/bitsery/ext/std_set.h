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

#ifndef BITSERY_EXT_STD_SET_H
#define BITSERY_EXT_STD_SET_H

#include <cassert>
#include "../details/adapter_utils.h"
//we need this, so we could reserve for non ordered set
#include <unordered_set>
#include "../traits/core/traits.h"

namespace bitsery {
    namespace ext {

        class StdSet {
        public:

            constexpr explicit StdSet(size_t maxSize):_maxSize{maxSize} {}

            template<typename Ser, typename Writer, typename T, typename Fnc>
            void serialize(Ser &, Writer &writer, const T &obj, Fnc &&fnc) const {
                using TKey = typename T::key_type;
                auto size = obj.size();
                assert(size <= _maxSize);
                details::writeSize(writer, size);

                for (auto &v:obj)
                    fnc(const_cast<TKey &>(v));
            }

            template<typename Des, typename Reader, typename T, typename Fnc>
            void deserialize(Des &, Reader &reader, T &obj, Fnc &&fnc) const {
                using TKey = typename T::key_type;

                size_t size{};
                details::readSize(reader, size, _maxSize);
                auto hint = obj.begin();
                obj.clear();
                reserve(obj, size);

                for (auto i = 0u; i < size; ++i) {
                    TKey key;
                    fnc(key);
                    hint = obj.emplace_hint(hint, std::move(key));
                }
            }
        private:

            template <typename T>
            void reserve(std::unordered_set<T>& obj, size_t size) const {
                obj.reserve(size);
            }
            template <typename T>
            void reserve(std::unordered_multiset<T>& obj, size_t size) const {
                obj.reserve(size);
            }
            template <typename T>
            void reserve(T& , size_t ) const {
                //for ordered container do nothing
            }
            size_t _maxSize;
        };
    }

    namespace traits {
        template<typename T>
        struct ExtensionTraits<ext::StdSet, T> {
            using TValue = typename T::key_type;
            static constexpr bool SupportValueOverload = true;
            static constexpr bool SupportObjectOverload = true;
            static constexpr bool SupportLambdaOverload = true;
        };
    }

}


#endif //BITSERY_EXT_STD_SET_H
