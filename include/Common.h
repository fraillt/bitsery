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

template <size_t SIZE, typename T>
constexpr size_t DEFAULT_OR_SIZE =SIZE == 0 ? sizeof(T) : SIZE;

template <size_t SIZE>
struct ProcessAnyType {
    template <typename S, typename T>
    static void serialize(S& s, T&& v) {
        s.template value<SIZE>(v);
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
