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

#ifndef BITSERY_EXT_STD_MAP_H
#define BITSERY_EXT_STD_MAP_H

#include "../traits/core/traits.h"
#include "../details/adapter_utils.h"

namespace bitsery {
    namespace ext {

        class StdMap {
        public:

            constexpr explicit StdMap(size_t maxSize):_maxSize{maxSize} {}

            template<typename Ser, typename Writer, typename T, typename Fnc>
            void serialize(Ser &, Writer &writer, const T &obj, Fnc &&fnc) const {
                using TKey = typename T::key_type;
                using TValue = typename T::mapped_type;
                auto size = obj.size();
                assert(size <= _maxSize);
                details::writeSize(writer, size);

                for (auto &v:obj)
                    fnc(const_cast<TKey &>(v.first), const_cast<TValue &>(v.second));
            }

            template<typename Des, typename Reader, typename T, typename Fnc>
            void deserialize(Des &, Reader &reader, T &obj, Fnc &&fnc) const {
                using TKey = typename T::key_type;
                using TValue = typename T::mapped_type;

                size_t size{};
                details::readSize(reader, size, _maxSize);
                auto hint = obj.begin();
                obj.clear();

                for (auto i = 0u; i < size; ++i) {
                    TKey key;
                    TValue value;
                    fnc(key, value);
                    hint = obj.emplace_hint(hint, std::move(key), std::move(value));
                }
            }
        private:
            size_t _maxSize;
        };
    }

    namespace traits {
        template<typename T>
        struct ExtensionTraits<ext::StdMap, T> {
            using TValue = void;
            static constexpr bool SupportValueOverload = false;
            static constexpr bool SupportObjectOverload = false;
            static constexpr bool SupportLambdaOverload = true;
        };
    }

}


#endif //BITSERY_EXT_STD_MAP_H
