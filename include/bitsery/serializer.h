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

namespace bitsery {

    template<typename Writter>
    class Serializer {
    public:
        Serializer(Writter &w, void* context = nullptr) : _writter{w}, _context{context} {};

        /*
         * get serialization context.
         * this is optional, but might be required for some specific serialization flows.
         */
        void* getContext() {
            return _context;
        }

        /*
         * object function
         */
        template<typename T>
        void object(T &&obj) {
            details::SerializeFunction<Serializer, T>::invoke(*this, std::forward<T>(obj));
        }

        template<typename T, typename Fnc>
        void object(T &&obj, Fnc &&fnc) {
            fnc(*this, std::forward<T>(obj));
        };

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
         * growable function
         */

        template<typename T, typename Fnc>
        void growable(const T &obj, Fnc &&fnc) {
            _writter.beginSession();
            fnc(*this, obj);
            _writter.endSession();
        };

        /*
         * extend functions
         */

        template<typename T, typename Ext, typename Fnc>
        void extend(const T &obj, Ext &&ext, Fnc &&fnc) {
            ext.serialize(*this, _writter, obj, std::forward<Fnc>(fnc));

        };

        template<size_t VSIZE, typename T, typename Ext>
        void extend(const T &obj, Ext &&ext) {
            ext.serialize(*this, _writter, obj, [](auto &s, auto &v) { s.template value<VSIZE>(v); });

        };

        template<typename T, typename Ext>
        void extend(const T &obj, Ext &&ext) {
            ext.serialize(*this, _writter, obj, [](auto &s, auto &v) { s.object(v); });
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
         * entropy overloads
         */
        template<typename T, size_t N, typename Fnc>
        void entropy(const T &v, const T (&expectedValues)[N], Fnc &&fnc) {
            auto index = details::findEntropyIndex(v, expectedValues);
            range(index, {{}, N + 1});
            if (!index)
                fnc(*this, v);
        };

        template<size_t VSIZE, typename T, size_t N>
        void entropy(const T &v, const T (&expectedValues)[N]) {
            auto index = details::findEntropyIndex(v, expectedValues);
            range(index, {{}, N + 1});
            if (!index)
                value<VSIZE>(v);
        };

        template<typename T, size_t N>
        void entropy(const T &v, const T (&expectedValues)[N]) {
            auto index = details::findEntropyIndex(v, expectedValues);
            range(index, {{}, N + 1});
            if (!index)
                object(v);
        };

        /*
         * text overloads
         */

        template<size_t VSIZE, typename T>
        void text(const T &str, size_t maxSize) {
            auto first = std::begin(str);
            auto last = std::end(str);
            auto size = static_cast<size_t>(std::distance(first, last));
            assert(size <= maxSize);
            writeSize(size);
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

        //dynamic size containers

        template<typename T, typename Fnc>
        void container(const T &obj, size_t maxSize, Fnc &&fnc) {
            static_assert(details::IsResizable<T>::value,
                          "use container(const T&, Fnc) overload without `maxSize` for static containers");
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T>
        void container(const T &obj, size_t maxSize) {
            static_assert(details::IsResizable<T>::value,
                          "use container(const T&) overload without `maxSize` for static containers");
            static_assert(VSIZE > 0, "");
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            //todo optimisation is possible for contigous containers, but currently there is no compile-time check for this
            procContainer<VSIZE>(std::begin(obj), std::end(obj), std::false_type{});
        }

        template<typename T>
        void container(const T &obj, size_t maxSize) {
            static_assert(details::IsResizable<T>::value,
                          "use container(const T&) overload without `maxSize` for static containers");
            assert(obj.size() <= maxSize);
            writeSize(obj.size());
            procContainer(std::begin(obj), std::end(obj));
        }

        //fixed size containers

        template<typename T, typename Fnc, typename std::enable_if<!std::is_integral<Fnc>::value>::type * = nullptr>
        void container(const T &obj, Fnc &&fnc) {
            static_assert(!details::IsResizable<T>::value,
                          "use container(const T&, size_t, Fnc) overload with `maxSize` for dynamic containers");
            procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T>
        void container(const T &obj) {
            static_assert(!details::IsResizable<T>::value,
                          "use container(const T&, size_t) overload with `maxSize` for dynamic containers");
            static_assert(VSIZE > 0, "");
            //todo optimisation is possible for contigous containers, but currently there is no compile-time check for this
            procContainer<VSIZE>(std::begin(obj), std::end(obj), std::false_type{});
        }

        template<typename T>
        void container(const T &obj) {
            static_assert(!details::IsResizable<T>::value,
                          "use container(const T&, size_t) overload with `maxSize` for dynamic containers");
            procContainer(std::begin(obj), std::end(obj));
        }

        //c-style array overloads

        template<typename T, size_t N, typename Fnc>
        void container(const T (&arr)[N], Fnc &&fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T, size_t N>
        void container(const T (&arr)[N]) {
            static_assert(VSIZE > 0, "");
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
        }

        template<typename T, size_t N>
        void container(const T (&arr)[N]) {
            procContainer(std::begin(arr), std::end(arr));
        }

        void align() {
            _writter.align();
        }

        //overloads for functions with explicit type size

        template<typename T>
        void value1b(T &&v) { value<1>(std::forward<T>(v)); }

        template<typename T>
        void value2b(T &&v) { value<2>(std::forward<T>(v)); }

        template<typename T>
        void value4b(T &&v) { value<4>(std::forward<T>(v)); }

        template<typename T>
        void value8b(T &&v) { value<8>(std::forward<T>(v)); }

        template<typename T, typename Ext>
        void extend1b(const T &v, Ext &&ext) { extend<1>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extend2b(const T &v, Ext &&ext) { extend<2>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extend4b(const T &v, Ext &&ext) { extend<4>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extend8b(const T &v, Ext &&ext) { extend<8>(v, std::forward<Ext>(ext)); };

        template<typename T, size_t N>
        void entropy1b(const T &v, const T (&expectedValues)[N]) {
            entropy<1>(v, expectedValues);
        };

        template<typename T, size_t N>
        void entropy2b(const T &v, const T (&expectedValues)[N]) {
            entropy<2>(v, expectedValues);
        };

        template<typename T, size_t N>
        void entropy4b(const T &v, const T (&expectedValues)[N]) {
            entropy<4>(v, expectedValues);
        };

        template<typename T, size_t N>
        void entropy8b(const T &v, const T (&expectedValues)[N]) {
            entropy<8>(v, expectedValues);
        };

        template<typename T>
        void text1b(const T &str, size_t maxSize) { text<1>(str, maxSize); }

        template<typename T>
        void text2b(const T &str, size_t maxSize) { text<2>(str, maxSize); }

        template<typename T>
        void text4b(const T &str, size_t maxSize) { text<4>(str, maxSize); }

        template<typename T, size_t N>
        void text1b(const T (&str)[N]) { text<1>(str); }

        template<typename T, size_t N>
        void text2b(const T (&str)[N]) { text<2>(str); }

        template<typename T, size_t N>
        void text4b(const T (&str)[N]) { text<4>(str); }

        template<typename T>
        void container1b(T &&obj, size_t maxSize) { container<1>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container2b(T &&obj, size_t maxSize) { container<2>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container4b(T &&obj, size_t maxSize) { container<4>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container8b(T &&obj, size_t maxSize) { container<8>(std::forward<T>(obj), maxSize); }

        template<typename T>
        void container1b(T &&obj) { container<1>(std::forward<T>(obj)); }

        template<typename T>
        void container2b(T &&obj) { container<2>(std::forward<T>(obj)); }

        template<typename T>
        void container4b(T &&obj) { container<4>(std::forward<T>(obj)); }

        template<typename T>
        void container8b(T &&obj) { container<8>(std::forward<T>(obj)); }

        template<typename T, size_t N>
        void container1b(const T (&arr)[N]) { container<1>(arr); }

        template<typename T, size_t N>
        void container2b(const T (&arr)[N]) { container<2>(arr); }

        template<typename T, size_t N>
        void container4b(const T (&arr)[N]) { container<4>(arr); }

        template<typename T, size_t N>
        void container8b(const T (&arr)[N]) { container<8>(arr); }

    private:
        Writter &_writter;
        void* _context;

        void writeSize(const size_t size) {
            details::writeSize(_writter, size);
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
