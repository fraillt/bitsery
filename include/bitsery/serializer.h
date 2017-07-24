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
#include <array>
#include "details/serialization_common.h"

namespace bitsery {

    template<typename Writter>
    class Serializer {
    public:
        Serializer(Writter &w) : _writter{w} {};

        template<typename T>
        Serializer& object(const T &obj) {
            return serialize(*this, obj);
        }

        //in c++17 change "class" to typename
        template <template <typename> class Extension, typename TValue, typename Fnc>
        Serializer& ext(const TValue& v, Fnc&& fnc ) {
            Extension<const TValue> ext{v};
            ext.serialize(*this, std::forward<Fnc>(fnc));
            return *this;
        };


        /*
         * value overloads
         */

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        Serializer& value(const T &v) {
            static_assert(std::numeric_limits<T>::is_iec559, "");
            _writter.template writeBytes<VSIZE>(reinterpret_cast<const details::SAME_SIZE_UNSIGNED<T> &>(v));
            return *this;
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
        Serializer& value(const T &v) {
            _writter.template writeBytes<VSIZE>(reinterpret_cast<const std::underlying_type_t<T> &>(v));
            return *this;
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        Serializer& value(const T &v) {
            _writter.template writeBytes<VSIZE>(v);
            return *this;
        }

        /*
         * bool
         */

        Serializer& boolBit(bool v) {
            _writter.writeBits(static_cast<unsigned char>(v ? 1 : 0), 1);
            return *this;
        }

        Serializer& boolByte(bool v) {
            _writter.template writeBytes<1>(static_cast<unsigned char>(v ? 1 : 0));
            return *this;
        }

        /*
         * range
         */

        template<typename T>
        Serializer& range(const T &v, const RangeSpec<T> &range) {
            assert(details::isRangeValid(v, range));
            _writter.template writeBits<decltype(details::getRangeValue(v, range))>(details::getRangeValue(v, range), range.bitsRequired);
            return *this;
        }

        /*
         * substitution overloads
         */
        template<typename T, size_t N, typename Fnc>
        Serializer& substitution(const T &v, const std::array<T, N> &expectedValues, Fnc &&fnc) {
            auto index = details::findSubstitutionIndex(v, expectedValues);
            range(index, {{}, N +1});
            if (!index)
                fnc(*this, v);
            return *this;
        };

        template<size_t VSIZE, typename T, size_t N>
        Serializer& substitution(const T &v, const std::array<T, N> &expectedValues) {
            auto index = details::findSubstitutionIndex(v, expectedValues);
            range(index, {{}, N +1});
            if (!index)
                value<VSIZE>(v);
            return *this;
        };

        template<typename T, size_t N>
        Serializer& substitution(const T &v, const std::array<T, N> &expectedValues) {
            auto index = details::findSubstitutionIndex(v, expectedValues);
            range(index, {{}, N +1});
            if (!index)
                object(v);
            return *this;
        };

        /*
         * text overloads
         */

        template<size_t VSIZE, typename T>
        Serializer& text(const std::basic_string<T> &str, size_t maxSize) {
            assert(str.size() <= maxSize);
            auto first = std::begin(str);
            auto last = std::end(str);
            writeSize(std::distance(first, last));
            procContainer<VSIZE>(first, last, std::true_type{});
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Serializer& text(const T (&str)[N]) {
            auto first = std::begin(str);
            auto last = std::next(first, std::min(std::char_traits<T>::length(str), N - 1));
            writeSize(std::distance(first, last));
            procContainer<VSIZE>(first, last, std::true_type{});
            return *this;
        }

        /*
         * container overloads
         */

        template<typename T, typename Fnc>
        Serializer& container(const T &obj, size_t maxSize, Fnc &&fnc) {
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
            return *this;
        }

        template<size_t VSIZE, typename T>
        Serializer& container(const T &obj, size_t maxSize) {
            static_assert(VSIZE > 0, "");
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            //todo optimisation is possible for contigous containers, but currently there is no compile-time check for this
            procContainer<VSIZE>(std::begin(obj), std::end(obj), std::false_type{});
            return *this;
        }

        template<typename T>
        Serializer& container(const T &obj, size_t maxSize) {
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            procContainer(std::begin(obj), std::end(obj));
            return *this;
        }

        /*
         * array overloads (fixed size array (std::array, and c-style array))
         */

        //std::array overloads

        template<typename T, size_t N, typename Fnc>
        Serializer& array(const std::array<T, N> &arr, Fnc &&fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Serializer& array(const std::array<T, N> &arr) {
            static_assert(VSIZE > 0, "");
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
            return *this;
        }

        template<typename T, size_t N>
        Serializer& array(const std::array<T, N> &arr) {
            procContainer(std::begin(arr), std::end(arr));
            return *this;
        }

        //c-style array overloads

        template<typename T, size_t N, typename Fnc>
        Serializer& array(const T (&arr)[N], Fnc &&fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Serializer& array(const T (&arr)[N]) {
            static_assert(VSIZE > 0, "");
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
            return *this;
        }

        template<typename T, size_t N>
        Serializer& array(const T (&arr)[N]) {
            procContainer(std::begin(arr), std::end(arr));
            return *this;
        }

        Serializer& align() {
            _writter.align();
            return *this;
        }

        //overloads for functions with explicit type size
        template<typename T> Serializer& value1(T &&v) { return value<1>(std::forward<T>(v)); }
        template<typename T> Serializer& value2(T &&v) { return value<2>(std::forward<T>(v)); }
        template<typename T> Serializer& value4(T &&v) { return value<4>(std::forward<T>(v)); }
        template<typename T> Serializer& value8(T &&v) { return value<8>(std::forward<T>(v)); }

        template<typename T, size_t N> Serializer& substitution1
                (const T &v, const std::array<T, N> &expectedValues) { return substitution<1>(v, expectedValues); };
        template<typename T, size_t N> Serializer& substitution2
                (const T &v, const std::array<T, N> &expectedValues) { return substitution<2>(v, expectedValues); };
        template<typename T, size_t N> Serializer& substitution4
                (const T &v, const std::array<T, N> &expectedValues) { return substitution<4>(v, expectedValues); };
        template<typename T, size_t N> Serializer& substitution8
                (const T &v, const std::array<T, N> &expectedValues) { return substitution<8>(v, expectedValues); };

        template<typename T> Serializer& text1(const std::basic_string<T> &str, size_t maxSize) {
            return text<1>(str, maxSize); }
        template<typename T> Serializer& text2(const std::basic_string<T> &str, size_t maxSize) {
            return text<2>(str, maxSize); }
        template<typename T> Serializer& text4(const std::basic_string<T> &str, size_t maxSize) {
            return text<4>(str, maxSize); }

        template<typename T, size_t N> Serializer& text1(const T (&str)[N]) { return text<1>(str); }
        template<typename T, size_t N> Serializer& text2(const T (&str)[N]) { return text<2>(str); }
        template<typename T, size_t N> Serializer& text4(const T (&str)[N]) { return text<4>(str); }

        template<typename T> Serializer& container1(T &&obj, size_t maxSize) {
            return container<1>(std::forward<T>(obj), maxSize); }
        template<typename T> Serializer& container2(T &&obj, size_t maxSize) {
            return container<2>(std::forward<T>(obj), maxSize); }
        template<typename T> Serializer& container4(T &&obj, size_t maxSize) {
            return container<4>(std::forward<T>(obj), maxSize); }
        template<typename T> Serializer& container8(T &&obj, size_t maxSize) {
            return container<8>(std::forward<T>(obj), maxSize); }

        template<typename T, size_t N> Serializer& array1(const std::array<T, N> &arr) { return array<1>(arr); }
        template<typename T, size_t N> Serializer& array2(const std::array<T, N> &arr) { return array<2>(arr); }
        template<typename T, size_t N> Serializer& array4(const std::array<T, N> &arr) { return array<4>(arr); }
        template<typename T, size_t N> Serializer& array8(const std::array<T, N> &arr) { return array<8>(arr); }

        template<typename T, size_t N> Serializer& array1(const T (&arr)[N]) { return array<1>(arr); }
        template<typename T, size_t N> Serializer& array2(const T (&arr)[N]) { return array<2>(arr); }
        template<typename T, size_t N> Serializer& array4(const T (&arr)[N]) { return array<4>(arr); }
        template<typename T, size_t N> Serializer& array8(const T (&arr)[N]) { return array<8>(arr); }

    private:
        Writter &_writter;

        void writeSize(const size_t size) {
            if (size < 0x80u) {
                _writter.writeBits(1u, 1);
                _writter.writeBits(size, 7);
            } else if (size < 0x4000u) {
                _writter.writeBits(2u,2);
                _writter.writeBits(size, 14);
            } else {
                assert(size < 0x40000000u);
                _writter.writeBits(0u,2);
                _writter.writeBits(size, 30);
            }
        }

        //process value types
        //false_type means that we must process all elements individually
        template<size_t VSIZE, typename It>
        void procContainer(It first, It last, std::false_type) {
            for (;first != last; ++first)
                value<VSIZE>(*first);
        };

        //process value types
        //true_type means, that we can copy whole buffer
        template<size_t VSIZE, typename It>
        void procContainer(It first, It last, std::true_type) {
            if (first != last)
                _writter.template writeBuffer<VSIZE>(&(*first), std::distance(first, last));
        };

        //process by calling functions
        template<typename It, typename Fnc>
        void procContainer(It first, It last, Fnc fnc) {
            for (;first != last; ++first)
                fnc(*this, *first);
        };

        //process object types
        template<typename It>
        void procContainer(It first, It last) {
            for (;first != last; ++first)
                object(*first);
        };

    };

}
#endif //BITSERY_SERIALIZER_H
