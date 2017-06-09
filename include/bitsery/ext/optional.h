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

namespace bitsery {
    namespace ext {

        template <typename T>
        using std_optional = ::std::optional<T>;

        template <typename T>
        class optional {
        public:
            explicit optional(T& v):_value{v} {
                using TOpt = typename std::remove_cv<T>::type;
                using TVal = typename TOpt::value_type;
                static_assert(std::is_same<TOpt, std_optional<TVal>>(), "");
            };
            template <typename TSerializer, typename Fnc>
            void serialize(TSerializer& ser, const Fnc& fnc) {
                ser.boolByte(static_cast<bool>(_value));
                if (_value)
                    fnc(ser, *_value);
            }
            template <typename TSerializer, typename Fnc>
            void deserialize(TSerializer& ser, const Fnc& fnc) {
                bool exists{};
                ser.boolByte(exists);
                if (exists) {
                    typename T::value_type tmp{};
                    fnc(ser, tmp);
                    _value = tmp;
                } else {
                    _value = T{};
                }
            }
        private:
            T& _value;
        };

    }
}


#endif //BITSERY_EXT_OPTIONAL_H
