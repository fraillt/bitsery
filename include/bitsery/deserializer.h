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


    template<typename Config>
    class BasicDeserializer {
    public:
        explicit BasicDeserializer(BasicBufferReader<Config> &r, void* context = nullptr) : _reader{r}, _context{context} {};

        /*
         * get serialization context.
         * this is optional, but might be required for some specific deserialization flows.
         */
        void* getContext() {
            return _context;
        }

        /*
         * object function
         */

        template<typename T>
        void object(T &&obj) {
            details::SerializeFunction<BasicDeserializer, T>::invoke(*this, std::forward<T>(obj));
        }

        template<typename T, typename Fnc>
        void object(T &&obj, Fnc &&fnc) {
            fnc(std::forward<T>(obj));
        };

        /*
         * value overloads
         */

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        void value(T &v) {
            static_assert(std::numeric_limits<T>::is_iec559, "");
            _reader.template readBytes<VSIZE>(reinterpret_cast<details::SAME_SIZE_UNSIGNED<T> &>(v));
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
        void value(T &v) {
            using UT = typename std::underlying_type<T>::type;
            _reader.template readBytes<VSIZE>(reinterpret_cast<UT &>(v));
        }

        template<size_t VSIZE, typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        void value(T &v) {
            _reader.template readBytes<VSIZE>(v);
        }

        /*
         * growable function
         */

        template <typename T, typename Fnc>
        void growable(T&& obj, Fnc&& fnc) {
            _reader.beginSession();
            fnc(std::forward<T>(obj));
            _reader.endSession();
        };

        /*
         * extend functions
         */

        template<typename T, typename Ext, typename Fnc>
        void extend(T &obj, Ext &&ext, Fnc &&fnc) {
            ext.deserialize(*this, _reader, obj, std::forward<Fnc>(fnc));
        };

        template<size_t VSIZE, typename T, typename Ext>
        void extend(T &obj, Ext &&ext) {
            static_assert(details::HasTValue<details::ExtensionTraits<Ext, T>>::value,
                          "this extension only supports overload with lambda");
            ext.deserialize(*this, _reader, obj, [this](typename details::ExtensionTraits<Ext, T>::TValue &v) { value<VSIZE>(v); });
        };

        template<typename T, typename Ext>
        void extend(T &obj, Ext &&ext) {
            static_assert(details::HasTValue<details::ExtensionTraits<Ext, T>>::value,
                          "this extension only supports overload with lambda");
            ext.deserialize(*this, _reader, obj, [this](typename details::ExtensionTraits<Ext, T>::TValue &v) { object(v); });
        };

        /*
         * bool
         */

        void boolBit(bool &v) {
            uint8_t tmp{};
            _reader.readBits(tmp, 1);
            v = tmp == 1;
        }

        void boolByte(bool &v) {
            unsigned char tmp;
            _reader.template readBytes<1>(tmp);
            if (tmp > 1)
                _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
            v = tmp == 1;
        }

        /*
         * range
         */

        template<typename T>
        void range(T &v, const RangeSpec<T> &range) {
            _reader.readBits(reinterpret_cast<details::SAME_SIZE_UNSIGNED<T> &>(v), range.bitsRequired);
            details::setRangeValue(v, range);
            if (!details::isRangeValid(v, range)) {
                _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                v = range.min;
            }
        }

        /*
         * entropy overloads
         */
        template<typename T, size_t N, typename Fnc>
        void entropy(T &obj, const T (&expectedValues)[N], Fnc &&fnc) {
            size_t index;
            range(index, {{}, N + 1});
            if (index)
                obj = expectedValues[index - 1];
            else
                fnc(obj);
        };

        template<size_t VSIZE, typename T, size_t N>
        void entropy(T &v, const T (&expectedValues)[N]) {
            size_t index;
            range(index, {{}, N + 1});
            if (index)
                v = expectedValues[index - 1];
            else
                value<VSIZE>(v);
        };

        template<typename T, size_t N>
        void entropy(T &obj, const T (&expectedValues)[N]) {
            size_t index;
            range(index, {{}, N + 1});
            if (index)
                obj = expectedValues[index - 1];
            else
                object(obj);
        };

        /*
         * text overloads
         */

        template<size_t VSIZE, typename T>
        void text(T &str, size_t maxSize) {
            static_assert(details::TextTraits<T>::isResizable,
                          "use text(T&) overload without `maxSize` for static containers");
            size_t size;
            readSize(size, maxSize);
            details::TextTraits<T>::resize(str, size);
            auto begin = std::begin(str);
            auto end = std::next(begin, size);
            procContainer<VSIZE>(begin, end, std::true_type{});
            //null terminated character at the end
            *end = {};
        }

        template<size_t VSIZE, typename T>
        void text(T &str) {
            static_assert(!details::TextTraits<T>::isResizable,
                          "use text(T&, size_t) overload with `maxSize` for dynamic containers");
            size_t size;
            auto begin = std::begin(str);
            auto containerEnd = std::end(str);
            assert(begin != containerEnd);
            readSize(size, static_cast<size_t>(std::distance(begin, containerEnd) - 1));
            //end of string, not en
            auto end = std::next(begin, size);
            procContainer<VSIZE>(std::begin(str), std::end(str), std::true_type{});
            //null terminated character at the end
            *end = {};
        }

        template<size_t VSIZE, typename T, size_t N>
        void text(T (&str)[N]) {
            size_t size;
            readSize(size, N - 1);
            auto first = std::begin(str);
            procContainer<VSIZE>(first, std::next(first, size), std::true_type{});
            //null-terminated string
            str[size] = {};
        }

        /*
         * container overloads
         */

        //dynamic size containers

        template<typename T, typename Fnc>
        void container(T &&obj, size_t maxSize, Fnc &&fnc) {
            static_assert(details::ContainerTraits<T>::isResizable,
                          "use container(T&) overload without `maxSize` for static containers");
            size_t size{};
            readSize(size, maxSize);
            details::ContainerTraits<T>::resize(obj, size);
            procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T>
        void container(T &obj, size_t maxSize) {
            static_assert(details::ContainerTraits<T>::isResizable,
                          "use container(T&) overload without `maxSize` for static containers");
            size_t size{};
            readSize(size, maxSize);
            details::ContainerTraits<T>::resize(obj, size);
            procContainer<VSIZE>(std::begin(obj), std::end(obj), std::false_type{});
        }

        template<typename T>
        void container(T &obj, size_t maxSize) {
            static_assert(details::ContainerTraits<T>::isResizable,
                          "use container(T&) overload without `maxSize` for static containers");
            size_t size{};
            readSize(size, maxSize);
            details::ContainerTraits<T>::resize(obj, size);
            procContainer(std::begin(obj), std::end(obj));
        }
        //fixed size containers

        template<typename T, typename Fnc, typename std::enable_if<!std::is_integral<Fnc>::value>::type * = nullptr>
        void container(T &&obj, Fnc &&fnc) {
            static_assert(!details::ContainerTraits<T>::isResizable,
                          "use container(T&, size_t, Fnc) overload with `maxSize` for dynamic containers");
            procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T>
        void container(T &obj) {
            static_assert(!details::ContainerTraits<T>::isResizable,
                          "use container(T&, size_t) overload with `maxSize` for dynamic containers");
            static_assert(VSIZE > 0);
            procContainer<VSIZE>(std::begin(obj), std::end(obj), std::false_type{});
        }

        template<typename T>
        void container(T &obj) {
            static_assert(!details::ContainerTraits<T>::isResizable,
                          "use container(T&, size_t) overload with `maxSize` for dynamic containers");
            procContainer(std::begin(obj), std::end(obj));
        }

        //c-style array overloads

        template<typename T, size_t N, typename Fnc>
        void container(T (&arr)[N], Fnc &&fnc) {
            procContainer(std::begin(arr), std::end(arr), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T, size_t N>
        void container(T (&arr)[N]) {
            procContainer<VSIZE>(std::begin(arr), std::end(arr), std::true_type{});
        }

        template<typename T, size_t N>
        void container(T (&arr)[N]) {
            procContainer(std::begin(arr), std::end(arr));
        }

        void align() {
            _reader.align();
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
        void extend1b(T &v, Ext &&ext) { extend<1>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extend2b(T &v, Ext &&ext) { extend<2>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extend4b(T &v, Ext &&ext) { extend<4>(v, std::forward<Ext>(ext)); };

        template<typename T, typename Ext>
        void extend8b(T &v, Ext &&ext) { extend<8>(v, std::forward<Ext>(ext)); };

        template<typename T, size_t N>
        void entropy1b(T &v, const T (&expectedValues)[N]) { entropy<1, T, N>(v, expectedValues); };

        template<typename T, size_t N>
        void entropy2b(T &v, const T (&expectedValues)[N]) { entropy<2, T, N>(v, expectedValues); };

        template<typename T, size_t N>
        void entropy4b(T &v, const T (&expectedValues)[N]) { entropy<4, T, N>(v, expectedValues); };

        template<typename T, size_t N>
        void entropy8b(T &v, const T (&expectedValues)[N]) { entropy<8, T, N>(v, expectedValues); };

        template<typename T>
        void text1b(T &str, size_t maxSize) { text<1>(str, maxSize); }

        template<typename T>
        void text2b(T &str, size_t maxSize) { text<2>(str, maxSize); }

        template<typename T>
        void text4b(T &str, size_t maxSize) { text<4>(str, maxSize); }

        template<typename T, size_t N>
        void text1b(T (&str)[N]) { text<1>(str); }

        template<typename T, size_t N>
        void text2b(T (&str)[N]) { text<2>(str); }

        template<typename T, size_t N>
        void text4b(T (&str)[N]) { text<4>(str); }

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
        void container1b(T (&arr)[N]) { container<1>(arr); }

        template<typename T, size_t N>
        void container2b(T (&arr)[N]) { container<2>(arr); }

        template<typename T, size_t N>
        void container4b(T (&arr)[N]) { container<4>(arr); }

        template<typename T, size_t N>
        void container8b(T (&arr)[N]) { container<8>(arr); }

    private:
        BasicBufferReader<Config> &_reader;
        void* _context;

        void readSize(size_t &size, size_t maxSize) {
            details::readSize(_reader, size);
            if (size > maxSize) {
                _reader.setError(BufferReaderError::INVALID_BUFFER_DATA);
                size = {};
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
            if (first != last)
                _reader.template readBuffer<VSIZE>(&(*first), std::distance(first, last));
        };

        //process by calling functions
        template<typename It, typename Fnc>
        void procContainer(It first, It last, Fnc fnc) {
            for (; first != last; ++first)
                fnc(*first);
        };

        //process object types
        template<typename It>
        void procContainer(It first, It last) {
            for (; first != last; ++first)
                object(*first);
        };

    };

    //helper type
    using Deserializer = BasicDeserializer<DefaultConfig>;

}

#endif //BITSERY_DESERIALIZER_H
