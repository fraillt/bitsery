//
// Created by fraillt on 17.1.5.
//

#ifndef TMP_COMMON_H
#define TMP_COMMON_H

#include <stdint.h>

namespace bitsery {

    template<typename T>
    constexpr size_t BITS_SIZE = sizeof(T) << 3;

    template<typename T>
    struct BIGGER_TYPE {
    };

    template<>
    struct BIGGER_TYPE<uint8_t> {
        typedef uint16_t type;
    };

    template<>
    struct BIGGER_TYPE<uint16_t> {
        typedef uint32_t type;
    };

    template<>
    struct BIGGER_TYPE<uint32_t> {
        typedef uint64_t type;
    };

    template<>
    struct BIGGER_TYPE<int8_t> {
        typedef int16_t type;
    };

    template<>
    struct BIGGER_TYPE<int16_t> {
        typedef int32_t type;
    };

    template<>
    struct BIGGER_TYPE<int32_t> {
        typedef int64_t type;
    };

    template<>
    struct BIGGER_TYPE<char> {
        typedef int16_t type;
    };

    template<typename T>
    constexpr size_t ARITHMETIC_OR_ENUM_SIZE = std::is_arithmetic<T>::value || std::is_enum<T>::value ? sizeof(T) : 0;


    template<typename T, typename Enable = void>
    struct SAME_SIZE_UNSIGNED_TYPE {
        typedef std::make_unsigned_t<T> type;
    };

    template<typename T>
    struct SAME_SIZE_UNSIGNED_TYPE<T, typename std::enable_if<std::is_enum<T>::value>::type> {
        typedef std::make_unsigned_t<std::underlying_type_t<T>> type;
    };

    template<typename T>
    struct SAME_SIZE_UNSIGNED_TYPE<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
        typedef std::conditional_t<std::is_same<T, float>::value, uint32_t, uint64_t> type;
    };

    template<typename T>
    using SAME_SIZE_UNSIGNED = typename SAME_SIZE_UNSIGNED_TYPE<T>::type;


    template<size_t SIZE>
    struct ProcessAnyType {
        template<typename S, typename T>
        static void serialize(S &s, T &&v) {
            s.template value<SIZE>(std::forward<T>(v));
        }
    };

    template<>
    struct ProcessAnyType<0> {
        template<typename S, typename T>
        static void serialize(S &s, T &&v) {
            s.object(std::forward<T>(v));
        }
    };


#define SERIALIZE(ObjectType) \
template <typename S, typename T, typename std::enable_if<std::is_same<T, ObjectType>::value || std::is_same<T, const ObjectType>::value>::type* = nullptr> \
S& serialize(S& s, T& o)

    template<typename T>
    constexpr size_t calcRequiredBits(T min, T max) {
        size_t res{};
        for (auto diff = max - min; diff > 0; diff >>= 1)
            ++res;
        return res;
    }


    template<typename T, typename Enable = void>
    struct RangeSpec {

        constexpr RangeSpec(T minValue, T maxValue)
                : min{minValue},
                  max{maxValue},
                  bitsRequired{calcRequiredBits(min, max)} {
        }

        const T min;
        const T max;
        const size_t bitsRequired;
    };


    template<typename T>
    struct RangeSpec<T, typename std::enable_if<std::is_enum<T>::value>::type> {

        constexpr RangeSpec(T minValue, T maxValue) :
                min{minValue},
                max{maxValue},
                bitsRequired{calcRequiredBits(
                        static_cast<std::underlying_type_t<T>>(min),
                        static_cast<std::underlying_type_t<T>>(max))} {
        }

        const T min;
        const T max;
        const size_t bitsRequired;
    };

//this class is used to make default RangeSpec float specialization always prefer constructor with precision
    struct BitsConstraint {
        explicit constexpr BitsConstraint(size_t bits) : value{bits} {}

        const size_t value;
    };

    template<typename T>
    struct RangeSpec<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {

        constexpr RangeSpec(T minValue, T maxValue, BitsConstraint bits) :
                min{minValue},
                max{maxValue},
                bitsRequired{bits.value} {
        }

        constexpr RangeSpec(T minValue, T maxValue, T precision) :
                min{minValue},
                max{maxValue},
                bitsRequired{calcRequiredBits<SAME_SIZE_UNSIGNED<T>>({}, ((max - min) / precision))} {

        }

        const T min;
        const T max;
        const size_t bitsRequired;
    };


    class ObjectMemoryPosition {
    public:

        template<typename T>
        ObjectMemoryPosition(const T &oldObj, const T &newObj)
                :ObjectMemoryPosition{reinterpret_cast<const char *>(&oldObj), reinterpret_cast<const char *>(&newObj),
                                      sizeof(T)} {
        }

        template<typename T>
        bool isFieldsEquals(const T &newObjField) {
            return *getOldObjectField(newObjField) == newObjField;
        }

        template<typename T>
        const T *getOldObjectField(const T &field) {
            auto offset = reinterpret_cast<const char *>(&field) - newObj;
            return reinterpret_cast<const T *>(oldObj + offset);
        }

    private:

        ObjectMemoryPosition(const char *objOld, const char *objNew, size_t)
                : oldObj{objOld},
                  newObj{objNew} {
        }

        const char *oldObj;
        const char *newObj;
    };

}
#endif //TMP_COMMON_H
