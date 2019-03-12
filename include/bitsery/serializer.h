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

#include "details/serialization_common.h"
#include "adapter_writer.h"
#include <cassert>

namespace bitsery {

    template<typename TAdapterWriter, typename TContext = void>
    class BasicSerializer {
    public:
        //this is used by AdapterAccess class
        using TWriter = TAdapterWriter;
        //helper type, that always returns bit-packing enabled type, useful inside serialize function when enabling bitpacking
        using BPEnabledType = BasicSerializer<typename std::conditional<TAdapterWriter::BitPackingEnabled,
                TAdapterWriter, AdapterWriterBitPackingWrapper<TAdapterWriter>>::type, TContext>;

        static_assert(details::IsSpecializationOf<typename TWriter::TConfig::InternalContext, std::tuple>::value,
                      "Config::InternalContext must be std::tuple");

        template <typename WriterParam>
        explicit BasicSerializer(WriterParam&& w, TContext* context = nullptr)
                : _writer{std::forward<WriterParam>(w)},
                  _context{context},
                  _internalContext{}
        {
        }

        //copying disabled
        BasicSerializer(const BasicSerializer&) = delete;
        BasicSerializer& operator = (const BasicSerializer&) = delete;

        //move enabled
        BasicSerializer(BasicSerializer&& ) = default;
        BasicSerializer& operator = (BasicSerializer&& ) = default;

        /*
         * get serialization context.
         * this is optional, but might be required for some specific serialization flows.
         */
        TContext* context() {
            return _context;
        }

        template <typename T>
        T* context() {
            return details::getContext<T>(_context, _internalContext);
        }

        template <typename T>
        T* contextOrNull() {
            return details::getContextIfTypeExists<T>(_context, _internalContext);
        }

        /*
         * object function
         */
        template<typename T>
        void object(const T &obj) {
            details::SerializeFunction<BasicSerializer, T>::invoke(*this, const_cast<T& >(obj));
        }

        template<typename T, typename Fnc>
        void object(const T &obj, Fnc &&fnc) {
            fnc(const_cast<T& >(obj));
        }

        /*
         * functionality, that enables simpler serialization syntax, by including additional header
         */
        template<typename T, typename ... TArgs>
        void archive(T &&head, TArgs &&... tail) {
            //serialize object
            details::ArchiveFunction<BasicSerializer, T>::invoke(*this, std::forward<T>(head));
            //expand other elements
            archive(std::forward<TArgs>(tail)...);
        }

        template <typename T, typename... TArgs>
        BasicSerializer &operator()(T &&head, TArgs &&... tail) {
            details::ArchiveFunction<BasicSerializer, T>::invoke(*this, std::forward<T>(head));
            archive(std::forward<TArgs>(tail)...);
            return *this;
        }

        /*
         * value overloads
         */

        template<size_t VSIZE, typename T, typename std::enable_if<details::IsFundamentalType<T>::value>::type * = nullptr>
        void value(const T &v) {
            using TValue = typename details::IntegralFromFundamental<T>::TValue;
            _writer.template writeBytes<VSIZE>(reinterpret_cast<const TValue &>(v));
        }

        /*
         * enable bit-packing
         */
        template <typename Fnc>
        void enableBitPacking(Fnc&& fnc) {
            procEnableBitPacking(std::forward<Fnc>(fnc), std::integral_constant<bool, TAdapterWriter::BitPackingEnabled>{});
        }

        /*
         * extension functions
         */

        template<typename T, typename Ext, typename Fnc>
        void ext(const T &obj, const Ext &extension, Fnc &&fnc) {
            static_assert(details::IsExtensionTraitsDefined<Ext, T>::value, "Please define ExtensionTraits");
            static_assert(traits::ExtensionTraits<Ext,T>::SupportLambdaOverload,
                          "extension doesn't support overload with lambda");
            extension.serialize(*this, _writer, obj, std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T, typename Ext>
        void ext(const T &obj, const Ext &extension) {
            static_assert(details::IsExtensionTraitsDefined<Ext, T>::value, "Please define ExtensionTraits");
            static_assert(traits::ExtensionTraits<Ext,T>::SupportValueOverload,
                          "extension doesn't support overload with `value<N>`");
            using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
            using VType = typename std::conditional<std::is_void<ExtVType>::value, details::DummyType, ExtVType>::type;
            extension.serialize(*this, _writer, obj, [this](VType &v) { value<VSIZE>(v); });
        }

        template<typename T, typename Ext>
        void ext(const T &obj, const Ext &extension) {
            static_assert(details::IsExtensionTraitsDefined<Ext, T>::value, "Please define ExtensionTraits");
            static_assert(traits::ExtensionTraits<Ext,T>::SupportObjectOverload,
                          "extension doesn't support overload with `object`");
            using ExtVType = typename traits::ExtensionTraits<Ext, T>::TValue;
            using VType = typename std::conditional<std::is_void<ExtVType>::value, details::DummyType, ExtVType>::type;
            extension.serialize(*this, _writer, obj, [this](VType &v) { object(v); });
        }

        /*
         * boolValue
         */

        void boolValue(bool v) {
            procBoolValue(v, std::integral_constant<bool, TAdapterWriter::BitPackingEnabled>{});
        }

        /*
         * text overloads
         */

        template<size_t VSIZE, typename T>
        void text(const T &str, size_t maxSize) {
            static_assert(details::IsTextTraitsDefined<T>::value,
                          "Please define TextTraits or include from <bitsery/traits/...>");
            static_assert(traits::ContainerTraits<T>::isResizable,
                          "use text(const T&) overload without `maxSize` for static container");
            procText<VSIZE>(str, maxSize);
        }

        template<size_t VSIZE, typename T>
        void text(const T &str) {
            static_assert(details::IsTextTraitsDefined<T>::value,
                          "Please define TextTraits or include from <bitsery/traits/...>");
            static_assert(!traits::ContainerTraits<T>::isResizable,
                          "use text(const T&, size_t) overload with `maxSize` for dynamic containers");
            procText<VSIZE>(str, traits::ContainerTraits<T>::size(str));
        }

        /*
         * container overloads
         */

        //dynamic size containers

        template<typename T, typename Fnc>
        void container(const T &obj, size_t maxSize, Fnc &&fnc) {
            static_assert(details::IsContainerTraitsDefined<T>::value,
                          "Please define ContainerTraits or include from <bitsery/traits/...>");
            static_assert(traits::ContainerTraits<T>::isResizable,
                          "use container(const T&, Fnc) overload without `maxSize` for static containers");
            auto size = traits::ContainerTraits<T>::size(obj);
            assert(size <= maxSize);
            details::writeSize(_writer, size);
            procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T>
        void container(const T &obj, size_t maxSize) {
            static_assert(details::IsContainerTraitsDefined<T>::value,
                          "Please define ContainerTraits or include from <bitsery/traits/...>");
            static_assert(traits::ContainerTraits<T>::isResizable,
                          "use container(const T&) overload without `maxSize` for static containers");
            static_assert(VSIZE > 0, "");
            auto size = traits::ContainerTraits<T>::size(obj);
            assert(size <= maxSize);
            details::writeSize(_writer, size);

            procContainer<VSIZE>(std::begin(obj), std::end(obj), std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
        }

        template<typename T>
        void container(const T &obj, size_t maxSize) {
            static_assert(details::IsContainerTraitsDefined<T>::value,
                          "Please define ContainerTraits or include from <bitsery/traits/...>");
            static_assert(traits::ContainerTraits<T>::isResizable,
                          "use container(const T&) overload without `maxSize` for static containers");
            auto size = traits::ContainerTraits<T>::size(obj);
            assert(size <= maxSize);
            details::writeSize(_writer, size);
            procContainer(std::begin(obj), std::end(obj));
        }

        //fixed size containers

        template<typename T, typename Fnc, typename std::enable_if<!std::is_integral<Fnc>::value>::type * = nullptr>
        void container(const T &obj, Fnc &&fnc) {
            static_assert(details::IsContainerTraitsDefined<T>::value,
                          "Please define ContainerTraits or include from <bitsery/traits/...>");
            static_assert(!traits::ContainerTraits<T>::isResizable,
                          "use container(const T&, size_t, Fnc) overload with `maxSize` for dynamic containers");
            procContainer(std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
        }

        template<size_t VSIZE, typename T>
        void container(const T &obj) {
            static_assert(details::IsContainerTraitsDefined<T>::value,
                          "Please define ContainerTraits or include from <bitsery/traits/...>");
            static_assert(!traits::ContainerTraits<T>::isResizable,
                          "use container(const T&, size_t) overload with `maxSize` for dynamic containers");
            static_assert(VSIZE > 0, "");
            procContainer<VSIZE>(std::begin(obj), std::end(obj), std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
        }

        template<typename T>
        void container(const T &obj) {
            static_assert(details::IsContainerTraitsDefined<T>::value,
                          "Please define ContainerTraits or include from <bitsery/traits/...>");
            static_assert(!traits::ContainerTraits<T>::isResizable,
                          "use container(const T&, size_t) overload with `maxSize` for dynamic containers");
            procContainer(std::begin(obj), std::end(obj));
        }

        void align() {
            _writer.align();
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
        void ext1b(const T &v, Ext &&extension) { ext<1, T, Ext>(v, std::forward<Ext>(extension)); }

        template<typename T, typename Ext>
        void ext2b(const T &v, Ext &&extension) { ext<2, T, Ext>(v, std::forward<Ext>(extension)); }

        template<typename T, typename Ext>
        void ext4b(const T &v, Ext &&extension) { ext<4, T, Ext>(v, std::forward<Ext>(extension)); }

        template<typename T, typename Ext>
        void ext8b(const T &v, Ext &&extension) { ext<8, T, Ext>(v, std::forward<Ext>(extension)); }

        template<typename T>
        void text1b(const T &str, size_t maxSize) { text<1>(str, maxSize); }

        template<typename T>
        void text2b(const T &str, size_t maxSize) { text<2>(str, maxSize); }

        template<typename T>
        void text4b(const T &str, size_t maxSize) { text<4>(str, maxSize); }

        template<typename T>
        void text1b(const T &str) { text<1>(str); }

        template<typename T>
        void text2b(const T &str) { text<2>(str); }

        template<typename T>
        void text4b(const T &str) { text<4>(str); }

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

    private:
        friend AdapterAccess;

        TAdapterWriter _writer;
        TContext* _context;
        typename TWriter::TConfig::InternalContext _internalContext;

        //process value types
        //false_type means that we must process all elements individually
        template<size_t VSIZE, typename It>
        void procContainer(It first, It last, std::false_type) {
            for (; first != last; ++first)
                value<VSIZE>(*first);
        }

        //process value types
        //true_type means, that we can copy whole buffer
        template<size_t VSIZE, typename It>
        void procContainer(It first, It last, std::true_type) {
            using TValue = typename std::decay<decltype(*first)>::type;
            using TIntegral = typename details::IntegralFromFundamental<TValue>::TValue;
			if (first != last)
				_writer.template writeBuffer<VSIZE>(reinterpret_cast<const TIntegral*>(&(*first)),
                                                    static_cast<size_t>(std::distance(first, last)));
        }

        //process by calling functions
        template<typename It, typename Fnc>
        void procContainer(It first, It last, Fnc fnc) {
            using TValue = typename std::decay<decltype(*first)>::type;
            for (; first != last; ++first) {
                fnc(const_cast<TValue&>(*first));
            }
        }

        //process text,
        template<size_t VSIZE, typename T>
        void procText(const T& str, size_t maxSize) {
            auto length = traits::TextTraits<T>::length(str);
            assert((length + (traits::TextTraits<T>::addNUL ? 1u : 0u)) <= maxSize);
            details::writeSize(_writer, length);
            auto begin = std::begin(str);
            procContainer<VSIZE>(begin, std::next(begin, length), std::integral_constant<bool, traits::ContainerTraits<T>::isContiguous>{});
        }

        //process object types
        template<typename It>
        void procContainer(It first, It last) {
            for (; first != last; ++first)
                object(*first);
        }

        //proc bool writing bit or byte, depending on if BitPackingEnabled or not
        void procBoolValue(bool v, std::true_type) {
            _writer.writeBits(static_cast<unsigned char>(v ? 1 : 0), 1);
        }

        void procBoolValue(bool v, std::false_type) {
            _writer.template writeBytes<1>(static_cast<unsigned char>(v ? 1 : 0));
        }

        //enable bit-packing or do nothing if it is already enabled
        template <typename Fnc>
        void procEnableBitPacking(const Fnc& fnc, std::true_type) {
            fnc(*this);
        }

        template <typename Fnc>
        void procEnableBitPacking(const Fnc& fnc, std::false_type) {
            //create serializer using bitpacking wrapper
            BPEnabledType tmp(_writer, _context);
            fnc(tmp);
        }

        //these are dummy functions for extensions that have TValue = void
        void object(const details::DummyType&) {

        }

        template <size_t VSIZE>
        void value(const details::DummyType&) {

        }

        //dummy function, that stops archive variadic arguments expansion
        void archive() {
        }

    };

    //helper type
    template <typename Adapter>
    using Serializer = BasicSerializer<AdapterWriter<Adapter, DefaultConfig>>;

    //helper function that set ups all the basic steps and after serialziation returns serialized bytes count
    template <typename Adapter, typename T>
    size_t quickSerialization(Adapter adapter, const T& value) {
        Serializer<Adapter> ser{std::move(adapter)};
        ser.object(value);
        auto& w = AdapterAccess::getWriter(ser);
        w.flush();
        return w.writtenBytesCount();
    }

    template <typename T>
    size_t quickMeasureSize(const T& value) {
        BasicSerializer<MeasureSize> ser{MeasureSize{}};
        ser.object(value);
        auto& w = AdapterAccess::getWriter(ser);
        w.flush();
        return w.writtenBytesCount();
    }

}
#endif //BITSERY_SERIALIZER_H
