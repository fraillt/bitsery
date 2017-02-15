//
// Created by fraillt on 17.1.5.
//

#ifndef TMP_COMMON_H
#define TMP_COMMON_H

#include <stdint.h>

template <typename T>
constexpr size_t BITS_SIZE = sizeof(T) << 3;

template <typename T>
struct BIGGER_TYPE {};

template <>
struct BIGGER_TYPE<uint8_t> {
    typedef uint16_t type;
};

template <>
struct BIGGER_TYPE<uint16_t> {
    typedef uint32_t type;
};

template <>
struct BIGGER_TYPE<uint32_t> {
    typedef uint64_t type;
};

template <>
struct BIGGER_TYPE<int8_t> {
    typedef int16_t type;
};

template <>
struct BIGGER_TYPE<int16_t> {
    typedef int32_t type;
};

template <>
struct BIGGER_TYPE<int32_t> {
    typedef int64_t type;
};

template <>
struct BIGGER_TYPE<char> {
    typedef int16_t type;
};

template <typename T>
constexpr size_t ARITHMETIC_OR_ENUM_SIZE = std::is_arithmetic<T>::value || std::is_enum<T>::value ? sizeof(T) : 0;

template <size_t SIZE>
struct ProcessAnyType {
    template <typename S, typename T>
    static void serialize(S& s, T&& v) {
        s.template value<SIZE>(std::forward<T>(v));
    }
};

template <>
struct ProcessAnyType<0> {
    template <typename S, typename T>
    static void serialize(S& s, T&& v) {
        s.object(std::forward<T>(v));
    }
};


#define SERIALIZE(ObjectType) \
template <typename S, typename T, typename std::enable_if<std::is_same<T, ObjectType>::value || std::is_same<T, const ObjectType>::value>::type* = nullptr> \
S& serialize(S& s, T& o)

extern int no_symbol;

template <typename T>
constexpr size_t calcRequiredBits(T min, T max) {
    (T)min != min ? throw (no_symbol) : 0;
    assert(min < max);
    size_t res{};
    for (auto diff = max - min; diff > 0; diff >>= 1)
        ++res;
    return res;
}


template <typename T, class Enable = void>
class RangeSpec {
public:

    constexpr RangeSpec(T min, T max)
            :_min{min},
             _max{max},
             _bitsRequired{calcRequiredBits(_min, _max)}
    {

    }

    constexpr size_t bitsRequired() const {
        return _bitsRequired;
    }

    constexpr bool isValid(const T& v) const {
        return !(_max < v || v < _min);
    }
    

private:
    T _min;
    T _max;
    size_t _bitsRequired;
};


template <typename T>
class RangeSpec<T, typename std::enable_if<std::is_enum<T>::value>::type> {
public:
    using value_type = typename std::underlying_type<T>::type;
    constexpr RangeSpec(T min, T max):
            _min{static_cast<value_type>(min)},
            _max{static_cast<value_type>(max)},
            _bitsRequired{calcRequiredBits(_min, _max)}
    {

    }
    constexpr size_t bitsRequired() const {
        return _bitsRequired;
    }
    constexpr bool isValid(const T& v) const {
        return !(_max < static_cast<value_type>(v) || static_cast<value_type>(v) < _min);
    }

    T getValue(T v) const {
        //return v - _min;
        return v;
    }
private:
    value_type _min;
    value_type _max;
    size_t _bitsRequired;
};



class ObjectMemoryPosition {
public:

    template <typename T>
    ObjectMemoryPosition(const T& oldObj, const T& newObj)
            :ObjectMemoryPosition{reinterpret_cast<const char*>(&oldObj), reinterpret_cast<const char*>(&newObj), sizeof(T)}
    {
    }

    template <typename T>
    bool isFieldsEquals(const T& newObjField) {
        return *getOldObjectField(newObjField) == newObjField;
    }
    template <typename T>
    const T* getOldObjectField(const T& field) {
        auto offset = reinterpret_cast<const char*>(&field) - newObj;
        return reinterpret_cast<const T*>(oldObj + offset);
    }
private:

    ObjectMemoryPosition(const char* objOld, const char* objNew, size_t )
            :oldObj{objOld},
             newObj{objNew}            
    {
    }

    const char* oldObj;
    const char* newObj;
};

#endif //TMP_COMMON_H
