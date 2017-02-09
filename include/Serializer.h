//
// Created by Mindaugas Vinkelis on 17.1.3.
//

#ifndef TMP_SERIALIZER_H
#define TMP_SERIALIZER_H

#include "Common.h"
#include <array>

template<typename Writter>
class Serializer {
public:
    Serializer(Writter& w):_writter{w} {};

    template <typename T>
    Serializer& object(const T& obj) {
        return serialize(*this, obj);
    }

    /*
     * value overloads
     */

    template<size_t VSIZE = 0, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    Serializer& value(const T& v) {
        static_assert(std::numeric_limits<float>::is_iec559, "");
        static_assert(std::numeric_limits<double>::is_iec559, "");

        constexpr size_t ValueSize = VSIZE == 0 ? sizeof(T) : VSIZE;
        using CT = std::conditional_t<std::is_same<T,float>::value, uint32_t, uint64_t>;
        _writter.template writeBytes<ValueSize>(reinterpret_cast<const CT&>(v));
        return *this;
    }

    template<size_t VSIZE = 0, typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
    Serializer& value(const T& v) {
        constexpr size_t ValueSize = VSIZE == 0 ? sizeof(T) : VSIZE;
        _writter.template writeBytes<ValueSize>(reinterpret_cast<const std::underlying_type_t<T>&>(v));
        return *this;
    }

    template<size_t VSIZE = 0, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    Serializer& value(const T& v) {
        constexpr size_t ValueSize = VSIZE == 0 ? sizeof(T) : VSIZE;
        _writter.template writeBytes<ValueSize>(v);
        return *this;
    }

    /*
     * text overloads
     */

    template <size_t VSIZE = 1, typename T>
    Serializer& text(const std::basic_string<T>& str) {
        procText<VSIZE>(str.data(), str.size());
        return *this;
    }

    template<size_t VSIZE=1, typename T, size_t N>
    Serializer& text(const T (&str)[N]) {
        procText<VSIZE>(str, std::min(std::char_traits<T>::length(str), N-1));
        return *this;
    }

    /*
     * container overloads
     */

    template <typename T, typename Fnc>
    Serializer& container(const T& obj, Fnc&& fnc) {
        writeLength(obj.size());
        for (auto& v: obj)
            fnc(v);
        return *this;
    }

    template <size_t VSIZE, typename T>
    Serializer& container(const T& obj) {
        writeLength(obj.size());
        procContainer<VSIZE>(obj);
        return *this;
    }

    template <typename T>
    Serializer& container(const T& obj) {
        writeLength(obj.size());
        using VType = typename T::value_type;
        constexpr auto VSIZE = std::is_arithmetic<VType>::value || std::is_enum<VType>::value ? sizeof(VType) : 0;
        procContainer<VSIZE>(obj);
        return *this;
    }

    /*
     * array overloads (fixed size array (std::array, and c-style array))
     */

    //std::array overloads

    template<typename T, size_t N, typename Fnc>
    Serializer & array(const std::array<T,N> &arr, Fnc&& fnc) {
        for (auto& v: arr)
            fnc(v);
        return *this;
    }

    template<size_t VSIZE, typename T, size_t N>
    Serializer & array(const std::array<T,N> &arr) {
        procContainer<VSIZE>(arr);
        return *this;
    }

    template<typename T, size_t N>
    Serializer & array(const std::array<T,N> &arr) {
        constexpr auto VSIZE = std::is_arithmetic<T>::value || std::is_enum<T>::value ? sizeof(T) : 0;
        procContainer<VSIZE>(arr);
        return *this;
    }

    //c-style array overloads

    template<typename T, size_t N, typename Fnc>
    Serializer& array(const T (&arr)[N], Fnc&& fnc) {
        const T* end = arr + N;
        for (const T* tmp = arr; tmp != end; ++tmp)
            fnc(*tmp);
        return *this;
    }

    template<typename T, size_t N>
    Serializer& array(const T (&arr)[N]) {
//        const T* end = arr + N;
//        for (const T* tmp = arr; tmp != end; ++tmp)
//            fnc(*tmp);
        return *this;
    }

private:
    Writter& _writter;
    void writeLength(const size_t size) {
        _writter.template writeBits<32>(size);
    }

    template <size_t VSIZE, typename T>
    void procContainer(T&& obj) {
        for (auto& v: obj)
            ProcessAnyType<VSIZE>::serialize(*this, v);
    };

    template <size_t VSIZE, typename T>
    void procText(const T* str, size_t size) {
        writeLength(size);
        if (size)
            _writter.template writeBuffer<VSIZE>(str, size);
    }

};


#endif //TMP_SERIALIZER_H
