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


#ifndef BITSERY_EXT_STD_OPTIONAL_H
#define BITSERY_EXT_STD_OPTIONAL_H

#include "../traits/core/traits.h"
#include "../details/serialization_common.h"
#include <optional>

namespace bitsery {
    namespace ext {

        class StdOptional {
        public:

            /**
             * Works with std::optional types
             * @param alignBeforeData only makes sense when bit-packing enabled, by default aligns after writing/reading bool state of optional
             */
            explicit StdOptional(bool alignBeforeData=true):_alignBeforeData{alignBeforeData} {}

            template<typename Ser, typename T, typename Fnc>
            void serialize(Ser &ser, const std::optional<T> &obj, Fnc &&fnc) const {
                ser.boolValue(static_cast<bool>(obj));
                if (_alignBeforeData)
                    ser.adapter().align();
                if (obj)
                    fnc(ser, const_cast<T&>(*obj));
            }

            template<typename Des, typename T, typename Fnc>
            void deserialize(Des &des, std::optional<T> &obj, Fnc &&fnc) const {
                bool exists{};
                des.boolValue(exists);
                if (_alignBeforeData)
                    des.adapter().align();
                if (exists) {
                    // Reinitializing nontrivial types may be expensive
                    // especially when they reference heap data, so if `obj`
                    // already holds a value then we'll deserialize into the
                    // existing object
                    if constexpr (!std::is_trivial_v<T>) {
                        if (obj) {
                            fnc(des, *obj);
                            return;
                        }
                    }

                    obj = ::bitsery::Access::create<T>();
                    fnc(des, *obj);
                } else {
                    obj = std::nullopt;
                }
            }
        private:
            bool _alignBeforeData;
        };
    }

    namespace traits {
        template<typename T>
        struct ExtensionTraits<ext::StdOptional, std::optional<T>> {
            using TValue = T;
            static constexpr bool SupportValueOverload = true;
            static constexpr bool SupportObjectOverload = true;
            static constexpr bool SupportLambdaOverload = true;
        };
    }

}


#endif //BITSERY_EXT_STD_OPTIONAL_H
