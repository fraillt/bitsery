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


//this module do not include optional, but expects it to be declared in std::optional
//if you're using experimental optional from <experimental/optional>
//add it in std namespace like this:
//namespace std {
//    template <typename T>
//    using optional = experimental::optional<T>;
//}
#include <type_traits>
#include "../traits/core/traits.h"

namespace bitsery {
    namespace ext {

        template<typename T>
        using std_optional = ::std::optional<T>;

        class StdOptional {
        public:

            /**
             * Works with std::optional types
             * @param alignBeforeData only makes sense when bit-packing enabled, by default aligns after writing/reading bool state of optional
             */
            explicit StdOptional(bool alignBeforeData=true):_alignBeforeData{alignBeforeData} {}
            template<typename T>
            constexpr void assertType() const {
                using TOpt = typename std::remove_cv<T>::type;
                using TVal = typename TOpt::value_type;
                static_assert(std::is_same<TOpt, std_optional<TVal>>(), "");
                static_assert(std::is_default_constructible<TVal>::value, "");
            };

            template<typename Ser, typename Writer, typename T, typename Fnc>
            void serialize(Ser &ser, Writer &, const T &obj, Fnc &&fnc) const {
                assertType<T>();
                ser.boolValue(static_cast<bool>(obj));
                if (_alignBeforeData)
                    ser.align();
                if (obj)
                    fnc(const_cast<typename T::value_type & >(*obj));
            }

            template<typename Des, typename Reader, typename T, typename Fnc>
            void deserialize(Des &des, Reader &, T &obj, Fnc &&fnc) const {
                assertType<T>();
                bool exists{};
                des.boolValue(exists);
                if (_alignBeforeData)
                    des.align();
                if (exists) {
                    typename T::value_type tmp{};
                    fnc(tmp);
                    obj = tmp;
                } else {
                    //experimental optional doesnt have .reset method
                    obj = T{};
                }
            }
        private:
            bool _alignBeforeData;
        };
    }

    namespace traits {
        template<typename T>
        struct ExtensionTraits<ext::StdOptional, T> {
            using TValue = typename T::value_type;
            static constexpr bool SupportValueOverload = true;
            static constexpr bool SupportObjectOverload = true;
            static constexpr bool SupportLambdaOverload = true;
        };
    }

}


#endif //BITSERY_EXT_STD_OPTIONAL_H
