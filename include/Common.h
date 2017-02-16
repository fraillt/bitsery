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

template <typename T>
using UINT_FOR_FLOATING_POINT = std::conditional_t<std::is_same<T,float>::value, uint32_t, uint64_t>; 

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

template <typename T>
constexpr size_t calcRequiredBits(T min, T max) {    
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

private:
    const T _min;
    const T _max;
    const size_t _bitsRequired;
};


template <typename T>
class RangeSpec<T, typename std::enable_if<std::is_enum<T>::value>::type> {
public:
    
    constexpr RangeSpec(T min, T max):
            _min{min},
            _max{max},
            _bitsRequired{calcRequiredBits(
                static_cast<std::underlying_type_t<T>>(_min), 
                static_cast<std::underlying_type_t<T>>(_max))}
    {                
    }
    constexpr size_t bitsRequired() const {
        return _bitsRequired;
    }

private:
    const T _min;
    const T _max;
    const size_t _bitsRequired;
};

template <typename T>
class RangeSpec<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
public:
    
    //todo bits should be separate size not implicitly convertable from floating point types, so these constructors would be ambiguous
    constexpr RangeSpec(T min, T max, size_t bits, int tmp):
            _min{min},
            _max{max},
            _bitsRequired{bits}
    {                
    }

    constexpr RangeSpec(T min, T max, T precision):
            _min{min},
            _max{max},
            _bitsRequired{calcRequiredBits<UINT_FOR_FLOATING_POINT<T>>({}, ((max - min) / precision))}
    {            
        
    }

    constexpr size_t bitsRequired() const {
        return _bitsRequired;
    }

private:
    const T _min;
    const T _max;
    const size_t _bitsRequired;
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
