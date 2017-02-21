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



#ifndef BITSERY_COMMON_H
#define BITSERY_COMMON_H

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

/*
 * range functions
 */

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

    template<typename T, typename std::enable_if<std::is_arithmetic<T>::value>::type * = nullptr>
    bool isRangeValid(const T &v, const RangeSpec<T> &r) {
        return !(r.min > v || v > r.max);
    }

    template<typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
    bool isRangeValid(const T &v, const RangeSpec<T> &r) {
        using VT = std::underlying_type_t<T>;
        return !(static_cast<VT>(r.min) > static_cast<VT>(v)
                 || static_cast<VT>(v) > static_cast<VT>(r.max));
    }

/*
 * delta functions
 */

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
#endif //BITSERY_COMMON_H
