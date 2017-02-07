//
// Created by Mindaugas Vinkelis on 17.1.3.
//

#ifndef TMP_SERIALIZER_H
#define TMP_SERIALIZER_H

#include <array>

template<typename Writter>
class Serializer {
public:
    Serializer(Writter& w):_writter{w} {};

    template <typename T>
    Serializer& object(T&& obj) {
        return serialize(*this, std::forward<T>(obj));
    }

    template<size_t SIZE = 0, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    Serializer& value(const T& v) {
        static_assert(std::numeric_limits<float>::is_iec559, "");
        static_assert(std::numeric_limits<double>::is_iec559, "");

        constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
        using CT = std::conditional_t<std::is_same<T,float>::value, uint32_t, uint64_t>;
        _writter.template WriteBytes<ValueSize>(reinterpret_cast<const CT&>(v));
        return *this;
    }

    template<size_t SIZE = 0, typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
    Serializer& value(const T& v) {
        constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
        _writter.template WriteBytes<ValueSize>(reinterpret_cast<const std::underlying_type_t<T>&>(v));
        return *this;
    }

    template<size_t SIZE = 0, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    Serializer& value(const T& v) {
        constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
        _writter.template WriteBytes<ValueSize>(v);
        return *this;
    }

    template <size_t SIZE = 1, typename T>
    Serializer& text(const std::basic_string<T>& str) {
        procText<SIZE>(str.data());
        return *this;
    }

    template<size_t SIZE=1, typename T, size_t N>
    Serializer& text(const T (&str)[N]) {
        procText<SIZE>(str);
        return *this;
    }


    template<typename T, size_t N, typename Fnc>
    Serializer & withArray(const std::array<T,N> &arr, Fnc&& fnc) {
        for (auto& v: arr)
            fnc(v);
        return *this;
    }

    template<typename T, size_t N, typename Fnc>
    Serializer& withArray(const T (&arr)[N], Fnc&& fnc) {
        const T* end = arr + N;
        for (const T* tmp = arr; tmp != end; ++tmp)
            fnc(*tmp);
        return *this;
    }

    template <typename T, typename Fnc>
    Serializer& withContainer(T&& obj, Fnc&& fnc) {
        writeLength(obj.size());
        for (auto& v: obj)
            fnc(v);
        return *this;
    }

    template <typename T>
    Serializer& withContainer(T&& obj) {
        writeLength(obj.size());
        procContainer<0>(std::forward<T>(obj));
        return *this;
    }

    template <size_t SIZE=0, typename T, typename std::enable_if<std::is_arithmetic<T>::value && std::is_enum<T>::value>::type>
    Serializer& withContainer(T&& obj) {
        writeLength(obj.size());
        //procContainer<SIZE == 0 ? sizeof(T) : SIZE>(std::forward<T>(obj));
        procContainer<DEFAULT_OR_SIZE<SIZE,T>>(std::forward<T>(obj));
        return *this;
    }


private:
    Writter& _writter;
    void writeLength(const size_t size) {
        _writter.template WriteBits<32>(size);
    }

    template <size_t SIZE, typename T>
    void procContainer(T&& obj) {
        for (auto& v: obj)
            ProcessAnyType<SIZE>::serialize(*this, v);
    };

    template <size_t SIZE, typename T>
    void procText(const T* str) {
        const auto size = std::char_traits<T>::length(str);
        writeLength(size);
        if (size)
            _writter.template WriteBuffer<SIZE>(str, size);
    }

};


#endif //TMP_SERIALIZER_H
