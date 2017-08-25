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
#include <string>

namespace bitsery {


    template<typename Reader>
    class Deserializer {
    public:
        Deserializer(Reader &r) : _reader{r}, _isValid{true} {};

        /*
         * object function
         */

        template<typename T>
        void object(T &&obj) {
            if (_isValid)
                details::SerializeFunction<Deserializer, T>::invoke(*this, std::forward<T>(obj));
        }

        /*
         * value overloads
         */

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        void value(T &v) {
            static_assert(std::numeric_limits<T>::is_iec559, "");
            if (_isValid)
                _isValid = _reader.template readBytes<VSIZE>(reinterpret_cast<details::SAME_SIZE_UNSIGNED<T> &>(v));
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
        void value(T &v) {
            using UT = std::underlying_type_t<T>;
            if (_isValid)
                _isValid = _reader.template readBytes<VSIZE>(reinterpret_cast<UT &>(v));
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        void value(T &v) {
            if (_isValid)
                _isValid = _reader.template readBytes<VSIZE>(v);
        }

        /*
         * custom function
         */

        template<typename T, typename Fnc>
        void custom(T &&obj, Fnc &&fnc) {
            if (_isValid)
                fnc(*this, std::forward<T>(obj));
        };

        /*
         * extension functions
         */

        template<typename T, typename Ext, typename Fnc>
        void extension(T &obj, Ext &&ext, Fnc &&fnc) {
            if (_isValid)
                ext.deserialize(obj, *this, std::forward<Fnc>(fnc));
        };

        template<size_t VSIZE, typename T, typename Ext>
        void extension(T &obj, Ext &&ext) {
            if (_isValid)
                ext.deserialize(obj, *this, [](auto &s, auto &v) { s.template value<VSIZE>(v); });
        };

        template<typename T, typename Ext>
        void extension(T &obj, Ext &&ext) {
            if (_isValid)
                ext.deserialize(obj, *this, [](auto &s, auto &v) { s.object(v); });
        };

        /*
         * bool
         */

        void boolBit(bool &v) {
            if (_isValid) {
                unsigned char tmp;
                _isValid = _reader.readBits(tmp, 1);
                v = tmp == 1;
            }
        }

        void boolByte(bool &v) {
            if (_isValid) {
                unsigned char tmp;
                _isValid = _reader.template readBytes<1>(tmp);
                if (_isValid)
                    _isValid = tmp < 2;
                v = tmp == 1;
            }
        }

        /*
         * range
         */

        template<typename T>
        void range(T &v, const RangeSpec<T> &range) {
            if (_isValid) {
                _isValid = _reader.readBits(reinterpret_cast<details::SAME_SIZE_UNSIGNED<T> &>(v), range.bitsRequired);
                details::setRangeValue(v, range);
                if (_isValid)
                    _isValid = details::isRangeValid(v, range);
            }
        }

        /*
         * substitution overloads
         */
        template<typename T, size_t N, typename Fnc>
        void substitution(T &v, const std::array<T, N> &expectedValues, Fnc &&fnc) {
            size_t index;
            range(index, {{}, N + 1});
            if (_isValid) {
                if (index)
                    v = expectedValues[index - 1];
                else
                    fnc(*this, v);
            }
        };

        template<size_t VSIZE, typename T, size_t N>
        void substitution(T &v, const std::array<T, N> &expectedValues) {
            size_t index;
            range(index, {{}, N + 1});
            if (_isValid) {
                if (index)
                    v = expectedValues[index - 1];
                else
                    value<VSIZE>(v);
            }
        };

        template<typename T, size_t N>
        void substitution(T &v, const std::array<T, N> &expectedValues) {
            size_t index;
            range(index, {{}, N + 1});
            if (_isValid) {
                if (index)
                    v = expectedValues[index - 1];
                else
                    object(v);
            }
        };

        /*
         * text overloads
         */

        template<size_t VSIZE, typename T>
        void text(std::basic_string<T> &str, size_t maxSize) {
            size_t size;
            readSize(size, maxSize);
            if (_isValid) {
                str.resize(size);
                procContainer<VSIZE>(std::begin(str), std::end(str), std::true_type{});
            }
        }

        template<size_t VSIZE, typename T, size_t N>
        void text(T (&str)[N]) {
            size_t size;
            readSize(size, N - 1);
            if (_isValid) {
                auto first = std::begin(str);
                procContainer<VSIZE>(first, std::next(first, size), std::true_type{});
                //null-terminated string
                str[size] = {};
            }
        }

        /*
         * container overloads
         */

        template<typename T, typename Fnc>
        void container(T &&obj, size_t maxSize, Fnc &&fnc) {
            decltype(obj.size()) size{};
            readSize(size, maxSize);
            if (_isValid) {
                obj.resize(size);
                procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
            }
        }

        template<size_t VSIZE, typename T>
        void container(T &obj, size_t maxSize) {
            decltype(obj.size()) size{};
            readSize(size, maxSize);
            if (_isValid) {
                obj.resize(size);
                procContainer<VSIZE>(std::begin(obj), std::end(obj), std::false_type{});
            }
        }

        template<typename T>
        void container(T &obj, size_t maxSize) {
            decltype(obj.size()) size{};
            readSize(size, maxSize);
            if (_isValid) {
                obj.resize(size);
                procContainer(std::begin(obj), std::end(obj));
            }
        }

        /*
         * array overloads (fixed size array (std::array, and c-style array))
         */

        //std::array overloads

        template<typename T, size_t N, typename Fnc>
        void array(std::array<T, N> &arr, Fnc &&fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T, size_t N>
        void array(std::array<T, N> &arr) {
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
        }

        template<typename T, size_t N>
        void array(std::array<T, N> &arr) {
            procContainer(std::begin(arr), std::end(arr));
        }

        //c-style array overloads

        template<typename T, size_t N, typename Fnc>
        void array(T (&arr)[N], Fnc &&fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T, size_t N>
        void array(T (&arr)[N]) {
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
        }

        template<typename T, size_t N>
        void array(T (&arr)[N]) {
            procContainer(std::begin(arr), std::end(arr));
        }

        void align() {
            _reader.align();
        }

        bool isValid() const {
            return _isValid;
        }

        //overloads for functions with explicit type size

        template<typename T>
        void value1(T &&v) { value<1>(std::forward<T>(v)); }

        template<typename T>
        void value2(T &&v) { value<2>(std::forward<T>(v)); }

        template<typename T>
        void value4(T &&v) { value<4>(std::forward<T>(v)); }

        template<typename T>
        void value8(T &&v) { value<8>(std::forward<T>(v)); }

        template<typename T, typename Ext>
        void extension1(T &v, Ext &&ext) { extension<1>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extension2(T &v, Ext &&ext) { extension<2>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extension4(T &v, Ext &&ext) { extension<4>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extension8(T &v, Ext &&ext) { extension<8>(v, std::forward<Ext>(ext)); };

        template<typename T, size_t N>
        void substitution1(T &v, const std::array<T, N> &expectedValues) { substitution<1>(v, expectedValues); };

        template<typename T, size_t N>
        void substitution2(T &v, const std::array<T, N> &expectedValues) { substitution<2>(v, expectedValues); };

        template<typename T, size_t N>
        void substitution4(T &v, const std::array<T, N> &expectedValues) { substitution<4>(v, expectedValues); };

        template<typename T, size_t N>
        void substitution8(T &v, const std::array<T, N> &expectedValues) { substitution<8>(v, expectedValues); };

        template<typename T>
        void text1(std::basic_string<T> &str, size_t maxSize) { text<1>(str, maxSize); }

        template<typename T>
        void text2(std::basic_string<T> &str, size_t maxSize) { text<2>(str, maxSize); }

        template<typename T>
        void text4(std::basic_string<T> &str, size_t maxSize) { text<4>(str, maxSize); }

        template<typename T, size_t N>
        void text1(T (&str)[N]) { text<1>(str); }

        template<typename T, size_t N>
        void text2(T (&str)[N]) { text<2>(str); }

        template<typename T, size_t N>
        void text4(T (&str)[N]) { text<4>(str); }

        template<typename T>
        void container1(T &&obj, size_t maxSize) { container<1>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container2(T &&obj, size_t maxSize) { container<2>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container4(T &&obj, size_t maxSize) { container<4>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container8(T &&obj, size_t maxSize) { container<8>(std::forward<T>(obj), maxSize); }

        template<typename T, size_t N>
        void array1(std::array<T, N> &arr) { array<1>(arr); }

        template<typename T, size_t N>
        void array2(std::array<T, N> &arr) { array<2>(arr); }

        template<typename T, size_t N>
        void array4(std::array<T, N> &arr) { array<4>(arr); }

        template<typename T, size_t N>
        void array8(std::array<T, N> &arr) { array<8>(arr); }

        template<typename T, size_t N>
        void array1(T (&arr)[N]) { array<1>(arr); }

        template<typename T, size_t N>
        void array2(T (&arr)[N]) { array<2>(arr); }

        template<typename T, size_t N>
        void array4(T (&arr)[N]) { array<4>(arr); }

        template<typename T, size_t N>
        void array8(T (&arr)[N]) { array<8>(arr); }

    private:
        Reader &_reader;
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
                                _isValid = _reader.readBits(size, 14);
                            } else {
                                _isValid = _reader.readBits(size, 30);
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
            for (; _isValid && first != last; ++first)
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
            for (; _isValid && first != last; ++first)
                fnc(*this, *first);
        };

        //process object types
        template<typename It>
        void procContainer(It first, It last) {
            for (; _isValid && first != last; ++first)
                object(*first);
        };

    };


}

#endif //BITSERY_DESERIALIZER_H
