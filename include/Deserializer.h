//
// Created by Mindaugas Vinkelis on 17.1.9.
//

#ifndef TMP_DESERIALIZER_H
#define TMP_DESERIALIZER_H

#include <array>

template<typename Reader>
class Deserializer {
public:
    Deserializer(Reader& r):_reader{r} {};

    template <typename T>
    Deserializer& object(T&& obj) {
        return serialize(*this, std::forward<T>(obj));
    }

    template<size_t SIZE = 0, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    Deserializer& value(T& v) {
        static_assert(std::numeric_limits<float>::is_iec559, "");
        static_assert(std::numeric_limits<double>::is_iec559, "");

        constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
        using CT = std::conditional_t<std::is_same<T,float>::value, uint32_t, uint64_t>;
        static_assert(sizeof(CT) == ValueSize, "");
        _reader.template ReadBytes<ValueSize>(reinterpret_cast<CT&>(v));
        return *this;
    }

    template<size_t SIZE = 0, typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
    Deserializer& value(T& v) {
        constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
        using UT = std::underlying_type_t<T>;
        _reader.template ReadBytes<ValueSize>(reinterpret_cast<UT&>(v));
        return *this;
    }

    template<size_t SIZE = 0, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    Deserializer& value(T& v) {
        constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
        _reader.template ReadBytes<ValueSize>(v);
        return *this;
    }

    template <size_t SIZE = 1, typename T>
    Deserializer& text(std::basic_string<T>& str) {
        size_t size;
        readLength(size);
		std::vector<T> buf(size);
		_reader.template ReadBuffer<SIZE>(buf.data(), size);
		str.assign(buf.data(), size);
//		str.resize(size);
//		if (size)
//			_reader.template ReadBuffer<SIZE>(str.data(), size);
        return *this;
    }

    template<typename T, size_t N, typename Fnc>
    Deserializer & withArray(std::array<T,N> &arr, Fnc && fnc) {
        for (auto& v: arr)
            fnc(v);
        return *this;
    }

    template<typename T, size_t N, typename Fnc>
    Deserializer& withArray(T (&arr)[N], Fnc&& fnc) {
        T* tmp = arr;
        for (auto i = 0u; i < N; ++i, ++tmp)
            fnc(*tmp);
        return *this;
    }

    template <typename T, typename Fnc>
    Deserializer& withContainer(T&& obj, Fnc&& fnc) {
        decltype(obj.size()) size{};
        readLength(size);
        obj.resize(size);
        for (auto& v:obj)
            fnc(v);
        return *this;
    }



    template <typename T>
    Deserializer& withContainer(T&& obj) {
        decltype(obj.size()) size{};
        readLength(size);
        obj.resize(size);
        for (auto& v: obj)
            object(v);
        return *this;
    }

    template <size_t SIZE, typename T>
    Deserializer& withContainer(T&& obj) {
        constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
        writeLength(obj.size());
        for (auto& v: obj)
            value<ValueSize>(v);
        return *this;
    }



private:
    Reader& _reader;
    void readLength(size_t& size) {
		_reader.template ReadBits<32>(size);
    }
};


#endif //TMP_DESERIALIZER_H
