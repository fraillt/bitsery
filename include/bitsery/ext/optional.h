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


#ifndef BITSERY_EXT_OPTIONAL_H
#define BITSERY_EXT_OPTIONAL_H


//this module do not include optional, but expects it to be declared in std::optional
//if you're using experimental optional from <experimental/optional>
//add it in std namespace like this:
//namespace std {
//    template <typename T>
//    using optional = experimental::optional<T>;
//}
#include <type_traits>

namespace bitsery {
    namespace ext {

        template<typename T>
        using std_optional = ::std::optional<T>;

        class optional {
        public:

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
                ser.boolByte(static_cast<bool>(obj));
                if (obj)
                    fnc(const_cast<typename T::value_type& >(*obj));
            }

            template<typename Des, typename Reader, typename T, typename Fnc>
            void deserialize(Des &des, Reader& , T &obj, Fnc &&fnc) const {
                assertType<T>();
                bool exists{};
                des.boolByte(exists);
                if (exists) {
                    typename T::value_type tmp{};
                    fnc(tmp);
                    obj = tmp;
                } else {
                    //experimental optional doesnt have .reset method
                    obj = T{};
                }
            }
        };
    }

    namespace details {
        template <typename T>
        struct ExtensionTraits<ext::optional, T> {
            using TValue = typename T::value_type;
        };
    }

}


#endif //BITSERY_EXT_OPTIONAL_H
