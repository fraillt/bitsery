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
        _writter.template writeBytes<ValueSize>(reinterpret_cast<const SAME_SIZE_UNSIGNED<T>&>(v));
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
     * range
     */

    template <typename T>
    Serializer& range(const T& v, const RangeSpec<T>& range) {
        assert(isRangeValid(v, range));
        _writter.template writeBits(getRangeValue(v,range), range.bitsRequired);
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
        procContainer<ARITHMETIC_OR_ENUM_SIZE<typename T::value_type>>(obj);
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
        procContainer<ARITHMETIC_OR_ENUM_SIZE<T>>(arr);
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

    template<size_t VSIZE, typename T, size_t N>
    Serializer& array(const T (&arr)[N]) {
        procCArray<VSIZE>(arr);
        return *this;
    }

    template<typename T, size_t N>
    Serializer& array(const T (&arr)[N]) {
        procCArray<ARITHMETIC_OR_ENUM_SIZE<T>>(arr);
        return *this;
    }


private:
    Writter& _writter;
    void writeLength(const size_t size) {
        _writter.writeBits(size, 32);
    }

    template <size_t VSIZE, typename T>
    void procContainer(T&& obj) {
        //todo could be improved for arithmetic types in contiguous containers (std::vector, std::array) (keep in mind std::vector<bool> specialization)
        for (auto& v: obj)
            ProcessAnyType<VSIZE>::serialize(*this, v);
    };

    template <size_t VSIZE, typename T, size_t N>
    void procCArray(T (&arr)[N]) {
        //todo could be improved for arithmetic types
        const T* end = arr + N;
        for (const T* it = arr; it != end; ++it)
            ProcessAnyType<VSIZE>::serialize(*this, *it);
    };

    template <size_t VSIZE, typename T>
    void procText(const T* str, size_t size) {
        writeLength(size);
        if (size)
            _writter.template writeBuffer<VSIZE>(str, size);
    }
};

/*
 * functions for range
 */

template<typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type* = nullptr>
bool isRangeValid(const T& v, const RangeSpec<T>& r) {
    return !(r.min > v || v > r.max);
}

template<typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
bool isRangeValid(const T& v, const RangeSpec<T>& r) {
    using VT = std::underlying_type_t<T>;
    return !(static_cast<VT>(r.min) > static_cast<VT>(v)
             || static_cast<VT>(v) > static_cast<VT>(r.max));
}


template<typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
auto getRangeValue(const T& v, const RangeSpec<T>& r) {
    return static_cast<SAME_SIZE_UNSIGNED<T>>(v - r.min);
};

template<typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
auto getRangeValue(const T& v, const RangeSpec<T>& r) {
    return static_cast<SAME_SIZE_UNSIGNED<T>>(v) - static_cast<SAME_SIZE_UNSIGNED<T>>(r.min);
};

template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
auto getRangeValue(const T& v, const RangeSpec<T>& r) {
    using VT = SAME_SIZE_UNSIGNED<T>;
    const VT maxUint = (static_cast<VT>(1) << r.bitsRequired) - 1;
    const auto ratio = (v - r.min) / (r.max - r.min);
    return static_cast<VT>(ratio * maxUint);
};


#endif //TMP_SERIALIZER_H
