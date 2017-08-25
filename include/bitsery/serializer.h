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



#ifndef BITSERY_SERIALIZER_H
#define BITSERY_SERIALIZER_H

#include "common.h"
#include "details/serialization_common.h"
#include <cassert>
#include <array>
#include <string>

namespace bitsery {

    template<typename Writter>
    class Serializer {
    public:
        Serializer(Writter &w) : _writter{w} {};

        /*
         * object function
         */
        template<typename T>
        void object(T &&obj) {
            details::SerializeFunction<Serializer, T>::invoke(*this, std::forward<T>(obj));
        }

        /*
         * value overloads
         */

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        void value(const T &v) {
            static_assert(std::numeric_limits<T>::is_iec559, "");
            _writter.template writeBytes<VSIZE>(reinterpret_cast<const details::SAME_SIZE_UNSIGNED<T> &>(v));
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
        void value(const T &v) {
            _writter.template writeBytes<VSIZE>(reinterpret_cast<const std::underlying_type_t<T> &>(v));
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        void value(const T &v) {
            _writter.template writeBytes<VSIZE>(v);
        }

        /*
         * custom function
         */

        template<typename T, typename Fnc>
        void custom(T &&obj, Fnc &&fnc) {
            fnc(*this, std::forward<T>(obj));
        };

        /*
         * extension functions
         */

        template<typename T, typename Ext, typename Fnc>
        void extension(const T &obj, Ext &&ext, Fnc &&fnc) {
            ext.serialize(obj, *this, std::forward<Fnc>(fnc));

        };

        template<size_t VSIZE, typename T, typename Ext>
        void extension(const T &obj, Ext &&ext) {
            ext.serialize(obj, *this, [](auto &s, auto &v) { s.template value<VSIZE>(v); });

        };

        template<typename T, typename Ext>
        void extension(const T &obj, Ext &&ext) {
            ext.serialize(obj, *this, [](auto &s, auto &v) { s.object(v); });
        };

        /*
         * bool
         */

        void boolBit(bool v) {
            _writter.writeBits(static_cast<unsigned char>(v ? 1 : 0), 1);
        }

        void boolByte(bool v) {
            _writter.template writeBytes<1>(static_cast<unsigned char>(v ? 1 : 0));
        }

        /*
         * range
         */

        template<typename T>
        void range(const T &v, const RangeSpec<T> &range) {
            assert(details::isRangeValid(v, range));
            using BT = decltype(details::getRangeValue(v, range));
            _writter.template writeBits<BT>(details::getRangeValue(v, range), range.bitsRequired);
        }

        /*
         * substitution overloads
         */
        template<typename T, size_t N, typename Fnc>
        void substitution(const T &v, const std::array<T, N> &expectedValues, Fnc &&fnc) {
            auto index = details::findSubstitutionIndex(v, expectedValues);
            range(index, {{}, N + 1});
            if (!index)
                fnc(*this, v);
        };

        template<size_t VSIZE, typename T, size_t N>
        void substitution(const T &v, const std::array<T, N> &expectedValues) {
            auto index = details::findSubstitutionIndex(v, expectedValues);
            range(index, {{}, N + 1});
            if (!index)
                value<VSIZE>(v);
        };

        template<typename T, size_t N>
        void substitution(const T &v, const std::array<T, N> &expectedValues) {
            auto index = details::findSubstitutionIndex(v, expectedValues);
            range(index, {{}, N + 1});
            if (!index)
                object(v);
        };

        /*
         * text overloads
         */

        template<size_t VSIZE, typename T>
        void text(const std::basic_string<T> &str, size_t maxSize) {
            assert(str.size() <= maxSize);
            auto first = std::begin(str);
            auto last = std::end(str);
            writeSize(std::distance(first, last));
            procContainer<VSIZE>(first, last, std::true_type{});
        }

        template<size_t VSIZE, typename T, size_t N>
        void text(const T (&str)[N]) {
            auto first = std::begin(str);
            auto last = std::next(first, std::min(std::char_traits<T>::length(str), N - 1));
            writeSize(std::distance(first, last));
            procContainer<VSIZE>(first, last, std::true_type{});
        }

        /*
         * container overloads
         */

        template<typename T, typename Fnc>
        void container(const T &obj, size_t maxSize, Fnc &&fnc) {
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T>
        void container(const T &obj, size_t maxSize) {
            static_assert(VSIZE > 0, "");
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            //todo optimisation is possible for contigous containers, but currently there is no compile-time check for this
            procContainer<VSIZE>(std::begin(obj), std::end(obj), std::false_type{});
        }

        template<typename T>
        void container(const T &obj, size_t maxSize) {
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            procContainer(std::begin(obj), std::end(obj));
        }

        /*
         * array overloads (fixed size array (std::array, and c-style array))
         */

        //std::array overloads

        template<typename T, size_t N, typename Fnc>
        void array(const std::array<T, N> &arr, Fnc &&fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T, size_t N>
        void array(const std::array<T, N> &arr) {
            static_assert(VSIZE > 0, "");
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
        }

        template<typename T, size_t N>
        void array(const std::array<T, N> &arr) {
            procContainer(std::begin(arr), std::end(arr));
        }

        //c-style array overloads

        template<typename T, size_t N, typename Fnc>
        void array(const T (&arr)[N], Fnc &&fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T, size_t N>
        void array(const T (&arr)[N]) {
            static_assert(VSIZE > 0, "");
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
        }

        template<typename T, size_t N>
        void array(const T (&arr)[N]) {
            procContainer(std::begin(arr), std::end(arr));
        }

        void align() {
            _writter.align();
        }

        bool isValid() const {
            //serialization cannot fail, it doesn't handle out of memory exception
            return true;
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
        void extension1(const T &v, Ext &&ext) { extension<1>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extension2(const T &v, Ext &&ext) { extension<2>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extension4(const T &v, Ext &&ext) { extension<4>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extension8(const T &v, Ext &&ext) { extension<8>(v, std::forward<Ext>(ext)); };

        template<typename T, size_t N>
        void substitution1(const T &v, const std::array<T, N> &expectedValues) {
            substitution<1>(v, expectedValues);
        };

        template<typename T, size_t N>
        void substitution2(const T &v, const std::array<T, N> &expectedValues) {
            substitution<2>(v, expectedValues);
        };

        template<typename T, size_t N>
        void substitution4(const T &v, const std::array<T, N> &expectedValues) {
            substitution<4>(v, expectedValues);
        };

        template<typename T, size_t N>
        void substitution8(const T &v, const std::array<T, N> &expectedValues) {
            substitution<8>(v, expectedValues);
        };

        template<typename T>
        void text1(const std::basic_string<T> &str, size_t maxSize) { text<1>(str, maxSize); }

        template<typename T>
        void text2(const std::basic_string<T> &str, size_t maxSize) { text<2>(str, maxSize); }

        template<typename T>
        void text4(const std::basic_string<T> &str, size_t maxSize) { text<4>(str, maxSize); }

        template<typename T, size_t N>
        void text1(const T (&str)[N]) { text<1>(str); }

        template<typename T, size_t N>
        void text2(const T (&str)[N]) { text<2>(str); }

        template<typename T, size_t N>
        void text4(const T (&str)[N]) { text<4>(str); }

        template<typename T>
        void container1(T &&obj, size_t maxSize) { container<1>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container2(T &&obj, size_t maxSize) { container<2>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container4(T &&obj, size_t maxSize) { container<4>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container8(T &&obj, size_t maxSize) { container<8>(std::forward<T>(obj), maxSize); }

        template<typename T, size_t N>
        void array1(const std::array<T, N> &arr) { array<1>(arr); }

        template<typename T, size_t N>
        void array2(const std::array<T, N> &arr) { array<2>(arr); }

        template<typename T, size_t N>
        void array4(const std::array<T, N> &arr) { array<4>(arr); }

        template<typename T, size_t N>
        void array8(const std::array<T, N> &arr) { array<8>(arr); }

        template<typename T, size_t N>
        void array1(const T (&arr)[N]) { array<1>(arr); }

        template<typename T, size_t N>
        void array2(const T (&arr)[N]) { array<2>(arr); }

        template<typename T, size_t N>
        void array4(const T (&arr)[N]) { array<4>(arr); }

        template<typename T, size_t N>
        void array8(const T (&arr)[N]) { array<8>(arr); }

    private:
        Writter &_writter;

        void writeSize(const size_t size) {
            if (size < 0x80u) {
                _writter.writeBits(1u, 1);
                _writter.writeBits(size, 7);
            } else if (size < 0x4000u) {
                _writter.writeBits(2u, 2);
                _writter.writeBits(size, 14);
            } else {
                assert(size < 0x40000000u);
                _writter.writeBits(0u, 2);
                _writter.writeBits(size, 30);
            }
        }

        //process value types
        //false_type means that we must process all elements individually
        template<size_t VSIZE, typename It>
        void procContainer(It first, It last, std::false_type) {
            for (; first != last; ++first)
                value<VSIZE>(*first);
        };

        //process value types
        //true_type means, that we can copy whole buffer
        template<size_t VSIZE, typename It>
        void procContainer(It first, It last, std::true_type) {
            _writter.template writeBuffer<VSIZE>(&(*first), std::distance(first, last));
        };

        //process by calling functions
        template<typename It, typename Fnc>
        void procContainer(It first, It last, Fnc fnc) {
            for (; first != last; ++first)
                fnc(*this, *first);
        };

        //process object types
        template<typename It>
        void procContainer(It first, It last) {
            for (; first != last; ++first)
                object(*first);
        };

    };

}
#endif //BITSERY_SERIALIZER_H
