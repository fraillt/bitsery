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


#ifndef BITSERY_DESERIALIZER_H
#define BITSERY_DESERIALIZER_H

#include "common.h"
#include "details/serialization_common.h"
#include <utility>

namespace bitsery {


    template<typename Reader>
    class Deserializer {
    public:
        Deserializer(Reader& r):_reader{r}, _isValid{true} {};

        template <typename T>
        Deserializer& object(T&& obj) {
            return serialize(*this, std::forward<T>(obj));
        }

        //in c++17 change "class" to typename
        template <template <typename> class Extension, typename TValue, typename Fnc>
        Deserializer& ext(TValue& v, Fnc&& fnc) {
            static_assert(!std::is_const<TValue>(), "");
            Extension<TValue> ext{v};
            ext.deserialize(*this, std::forward<Fnc>(fnc));
            return *this;
        };

        /*
         * value overloads
         */

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
        Deserializer& value(T& v) {
            static_assert(std::numeric_limits<T>::is_iec559, "");
            if (_isValid) {
                _isValid = _reader.template readBytes<VSIZE>(reinterpret_cast<details::SAME_SIZE_UNSIGNED<T>&>(v));
            }
            return *this;
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
        Deserializer& value(T& v) {
            using UT = std::underlying_type_t<T>;
            if (_isValid) {
                _isValid = _reader.template readBytes<VSIZE>(reinterpret_cast<UT&>(v));
            }
            return *this;
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
        Deserializer& value(T& v) {
            if (_isValid) {
                _isValid = _reader.template readBytes<VSIZE>(v);
            }
            return *this;
        }

        /*
         * bool
         */

        Deserializer& boolBit(bool& v) {
            if (_isValid) {
                unsigned char tmp;
                _isValid = _reader.readBits(tmp, 1);
                v = tmp == 1;
            }
            return *this;
        }

        Deserializer& boolByte(bool& v) {
            if (_isValid) {
                unsigned char tmp;
                _isValid = _reader.template readBytes<1>(tmp);
                if (_isValid)
                    _isValid = tmp < 2;
                v = tmp == 1;
            }
            return *this;
        }

        /*
         * range
         */

        template <typename T>
        Deserializer& range(T& v, const RangeSpec<T>& range) {
            if (_isValid) {
                _isValid = _reader.readBits(reinterpret_cast<details::SAME_SIZE_UNSIGNED<T>&>(v), range.bitsRequired);
                details::setRangeValue(v, range);
                if (_isValid)
                    _isValid = details::isRangeValid(v, range);
            }
            return *this;
        }

        /*
         * substitution overloads
         */
        template<typename T, size_t N, typename Fnc>
        Deserializer& substitution(T& v, const std::array<T,N>& expectedValues, Fnc&& fnc) {
            size_t index;
            range(index, {{}, N + 1});
            if (_isValid) {
                if (index)
                    v = expectedValues[index-1];
                else
                    fnc(*this, v);
            }
            return *this;
        };

        template<size_t VSIZE, typename T, size_t N>
        Deserializer& substitution(T& v, const std::array<T,N>& expectedValues) {
            size_t index;
            range(index, {{}, N + 1});
            if (_isValid) {
                if (index)
                    v = expectedValues[index-1];
                else
                    value<VSIZE>(v);
            }
            return *this;
        };

        template<typename T, size_t N>
        Deserializer& substitution(T& v, const std::array<T,N>& expectedValues) {
            size_t index;
            range(index, {{}, N + 1});
            if (_isValid) {
                if (index)
                    v = expectedValues[index-1];
                else
                    object(v);
            }
            return *this;
        };

        /*
         * text overloads
         */

        template <size_t VSIZE, typename T>
        Deserializer& text(std::basic_string<T>& str, size_t maxSize) {
            size_t size;
            readSize(size, maxSize);
            if (_isValid) {
                str.resize(size);
                procContainer<VSIZE>(std::begin(str), std::end(str), std::true_type{});
            }
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Deserializer& text(T (&str)[N]) {
            size_t size;
            readSize(size, N-1);
            if (_isValid) {
                auto first = std::begin(str);
                procContainer<VSIZE>(first, std::next(first, size), std::true_type{});
                //null-terminated string
                str[size] = {};
            }
            return *this;
        }

        /*
         * container overloads
         */

        template <typename T, typename Fnc>
        Deserializer& container(T&& obj, size_t maxSize, Fnc&& fnc) {
            decltype(obj.size()) size{};
            readSize(size, maxSize);
            if (_isValid) {
                obj.resize(size);
                procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
            }
            return *this;
        }

        template <size_t VSIZE, typename T>
        Deserializer& container(T& obj, size_t maxSize) {
            decltype(obj.size()) size{};
            readSize(size, maxSize);
            if (_isValid) {
                obj.resize(size);
                procContainer<VSIZE>(std::begin(obj), std::end(obj), std::false_type{});
            }
            return *this;
        }

        template <typename T>
        Deserializer& container(T& obj, size_t maxSize) {
            decltype(obj.size()) size{};
            readSize(size, maxSize);
            if (_isValid) {
                obj.resize(size);
                procContainer(std::begin(obj), std::end(obj));
            }
            return *this;
        }

        /*
         * array overloads (fixed size array (std::array, and c-style array))
         */

        //std::array overloads

        template<typename T, size_t N, typename Fnc>
        Deserializer&  array(std::array<T,N> &arr, Fnc && fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Deserializer&  array(std::array<T,N> &arr) {
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
            return *this;
        }

        template<typename T, size_t N>
        Deserializer&  array(std::array<T,N> &arr) {
            procContainer(std::begin(arr), std::end(arr));
            return *this;
        }

        //c-style array overloads

        template<typename T, size_t N, typename Fnc>
        Deserializer& array(T (&arr)[N], Fnc&& fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Deserializer& array(T (&arr)[N]) {
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
            return *this;
        }

        template<typename T, size_t N>
        Deserializer& array(T (&arr)[N]) {
            procContainer(std::begin(arr), std::end(arr));
            return *this;
        }
        bool isValid() const {
            return _isValid;
        }

        Deserializer& align() {
            _reader.align();
            return *this;
        }

        //overloads for functions with explicit type size
        template<typename T> Deserializer& value1(T &&v) { return value<1>(std::forward<T>(v)); }
        template<typename T> Deserializer& value2(T &&v) { return value<2>(std::forward<T>(v)); }
        template<typename T> Deserializer& value4(T &&v) { return value<4>(std::forward<T>(v)); }
        template<typename T> Deserializer& value8(T &&v) { return value<8>(std::forward<T>(v)); }

        template<typename T, size_t N> Deserializer& substitution1
                (T &v, const std::array<T, N> &expectedValues) { return substitution<1>(v, expectedValues); };
        template<typename T, size_t N> Deserializer& substitution2
                (T &v, const std::array<T, N> &expectedValues) { return substitution<2>(v, expectedValues); };
        template<typename T, size_t N> Deserializer& substitution4
                (T &v, const std::array<T, N> &expectedValues) { return substitution<4>(v, expectedValues); };
        template<typename T, size_t N> Deserializer& substitution8
                (T &v, const std::array<T, N> &expectedValues) { return substitution<8>(v, expectedValues); };

        template<typename T> Deserializer& text1(std::basic_string<T> &str, size_t maxSize) {
            return text<1>(str, maxSize); }
        template<typename T> Deserializer& text2(std::basic_string<T> &str, size_t maxSize) {
            return text<2>(str, maxSize); }
        template<typename T> Deserializer& text4(std::basic_string<T> &str, size_t maxSize) {
            return text<4>(str, maxSize); }

        template<typename T, size_t N> Deserializer& text1(T (&str)[N]) { return text<1>(str); }
        template<typename T, size_t N> Deserializer& text2(T (&str)[N]) { return text<2>(str); }
        template<typename T, size_t N> Deserializer& text4(T (&str)[N]) { return text<4>(str); }

        template<typename T> Deserializer& container1(T &&obj, size_t maxSize) {
            return container<1>(std::forward<T>(obj), maxSize); }
        template<typename T> Deserializer& container2(T &&obj, size_t maxSize) {
            return container<2>(std::forward<T>(obj), maxSize); }
        template<typename T> Deserializer& container4(T &&obj, size_t maxSize) {
            return container<4>(std::forward<T>(obj), maxSize); }
        template<typename T> Deserializer& container8(T &&obj, size_t maxSize) {
            return container<8>(std::forward<T>(obj), maxSize); }

    private:
        Reader& _reader;
        bool _isValid;
        void readSize(size_t &size, size_t maxSize) {
            size = {};
            if (_isValid) {
                unsigned char firstBit;
                _isValid = _reader.readBits(firstBit, 1);
                if (_isValid) {
                    if (firstBit) {
                        _isValid = _reader.readBits(size, 7);
                    } else {
                        unsigned char secondBit;
                        _isValid = _reader.readBits(secondBit, 1);
                        if (_isValid) {
                            if (secondBit) {
                                _isValid = _reader.readBits(size,14);
                            } else {
                                _isValid = _reader.readBits(size,30);
                            }
                        }
                    }
                }
                if (_isValid)
                    _isValid = size <= maxSize;
            }
        }

        //process value types
        //false_type means that we must process all elements individually
        template<size_t VSIZE, typename It>
        void procContainer(It first, It last, std::false_type) {
            for (;_isValid && first != last; ++first)
                value<VSIZE>(*first);
        };

        //process value types
        //true_type means, that we can copy whole buffer
        template<size_t VSIZE, typename It>
        void procContainer(It first, It last, std::true_type) {
            if (_isValid && first != last)
                _isValid = _reader.template readBuffer<VSIZE>(&(*first), std::distance(first, last));
        };

        //process by calling functions
        template<typename It, typename Fnc>
        void procContainer(It first, It last, Fnc fnc) {
            for (;_isValid && first != last; ++first)
                fnc(*this, *first);
        };

        //process object types
        template<typename It>
        void procContainer(It first, It last) {
            for (;_isValid && first != last; ++first)
                object(*first);
        };

    };


}

#endif //BITSERY_DESERIALIZER_H
