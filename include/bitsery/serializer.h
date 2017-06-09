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

namespace bitsery {

/*
 * functions for range
 */

    template<typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
    auto getRangeValue(const T &v, const RangeSpec<T> &r) {
        return static_cast<SAME_SIZE_UNSIGNED<T>>(v - r.min);
    };

    template<typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
    auto getRangeValue(const T &v, const RangeSpec<T> &r) {
        return static_cast<SAME_SIZE_UNSIGNED<T>>(v) - static_cast<SAME_SIZE_UNSIGNED<T>>(r.min);
    };

    template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
    auto getRangeValue(const T &v, const RangeSpec<T> &r) {
        using VT = SAME_SIZE_UNSIGNED<T>;
        const VT maxUint = (static_cast<VT>(1) << r.bitsRequired) - 1;
        const auto ratio = (v - r.min) / (r.max - r.min);
        return static_cast<VT>(ratio * maxUint);
    };

/*
 * functions for substitution
 */

    template<typename T, size_t N>
    size_t findSubstitutionIndex(const T &v, const std::array<T, N> &defValues) {
        auto index{1u};
        for (auto &d:defValues) {
            if (d == v)
                return index;
            ++index;
        }
        return 0u;
    };




    template<typename Writter>
    class Serializer {
    public:
        Serializer(Writter &w) : _writter{w} {};

        template<typename T>
        Serializer& object(const T &obj) {
            return serialize(*this, obj);
        }

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
            _writter.template writeBytes<VSIZE>(reinterpret_cast<const SAME_SIZE_UNSIGNED<T> &>(v));
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
            _writter.template writeBits(static_cast<unsigned char>(v ? 1 : 0), 1);
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
            assert(isRangeValid(v, range));
            _writter.template writeBits(getRangeValue(v, range), range.bitsRequired);
            return *this;
        }

        /*
         * substitution overloads
         */
        template<typename T, size_t N, typename Fnc>
        Serializer& substitution(const T &v, const std::array<T, N> &expectedValues, Fnc &&fnc) {
            auto index = findSubstitutionIndex(v, expectedValues);
            range(index, {{}, N +1});
            if (!index)
                fnc(*this, v);
            return *this;
        };

        template<size_t VSIZE, typename T, size_t N>
        Serializer& substitution(const T &v, const std::array<T, N> &expectedValues) {
            auto index = findSubstitutionIndex(v, expectedValues);
            range(index, {{}, N +1});
            if (!index)
                value<VSIZE>(v);
            return *this;
        };

        template<typename T, size_t N>
        Serializer& substitution(const T &v, const std::array<T, N> &expectedValues) {
            auto index = findSubstitutionIndex(v, expectedValues);
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
            procText<VSIZE>(str.data(), str.size());
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Serializer& text(const T (&str)[N]) {
            procText<VSIZE>(str, std::min(std::char_traits<T>::length(str), N - 1));
            return *this;
        }

        /*
         * container overloads
         */

        template<typename T, typename Fnc>
        Serializer& container(const T &obj, size_t maxSize, Fnc &&fnc) {
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            for (auto &v: obj)
                fnc(*this, v);
            return *this;
        }

        template<size_t VSIZE, typename T>
        Serializer& container(const T &obj, size_t maxSize) {
            static_assert(VSIZE > 0, "");
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            procContainer<VSIZE>(obj);
            return *this;
        }

        template<typename T>
        Serializer& container(const T &obj, size_t maxSize) {
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            procContainer<0>(obj);
            return *this;
        }

        /*
         * array overloads (fixed size array (std::array, and c-style array))
         */

        //std::array overloads

        template<typename T, size_t N, typename Fnc>
        Serializer& array(const std::array<T, N> &arr, Fnc &&fnc) {
            for (auto &v: arr)
                fnc(*this, v);
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Serializer& array(const std::array<T, N> &arr) {
            static_assert(VSIZE > 0, "");
            procContainer<VSIZE>(arr);
            return *this;
        }

        template<typename T, size_t N>
        Serializer& array(const std::array<T, N> &arr) {
            procContainer<0>(arr);
            return *this;
        }

        //c-style array overloads

        template<typename T, size_t N, typename Fnc>
        Serializer& array(const T (&arr)[N], Fnc &&fnc) {
            const T *end = arr + N;
            for (const T *tmp = arr; tmp != end; ++tmp)
                fnc(*this, *tmp);
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Serializer& array(const T (&arr)[N]) {
            static_assert(VSIZE > 0, "");
            procCArray<VSIZE>(arr);
            return *this;
        }

        template<typename T, size_t N>
        Serializer& array(const T (&arr)[N]) {
            procCArray<0>(arr);
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

        template<size_t VSIZE, typename T>
        void procContainer(T &&obj) {
            //todo could be improved for arithmetic types in contiguous containers (std::vector, std::array) (keep in mind std::vector<bool> specialization)
            for (auto &v: obj)
                ProcessAnyType<VSIZE>::serialize(*this, v);
        };

        template<size_t VSIZE, typename T, size_t N>
        void procCArray(T (&arr)[N]) {
            //todo could be improved for arithmetic types
            const T *end = arr + N;
            for (const T *it = arr; it != end; ++it)
                ProcessAnyType<VSIZE>::serialize(*this, *it);
        };

        template<size_t VSIZE, typename T>
        void procText(const T *str, size_t size) {
            writeSize(size);
            if (size)
                _writter.template writeBuffer<VSIZE>(str, size);
        }
    };

}
#endif //BITSERY_SERIALIZER_H
