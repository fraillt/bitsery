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

#ifndef BITSERY_DETAILS_SERIALIZATION_COMMON_H
#define BITSERY_DETAILS_SERIALIZATION_COMMON_H

#include <type_traits>
#include <array>
#include "both_common.h"

namespace bitsery {

    namespace details {

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

        template<typename T>
        constexpr size_t getSize(T v, size_t s) {
            return v > 0 ? getSize(v / 2, s + 1) : s;
        }

        template<typename T>
        constexpr size_t calcRequiredBits(T min, T max) {
            //call recursive function, because some compilers only support constexpr functions with return-only body
            return getSize(max - min, 0);
        }

        template <typename T, typename = int>
        struct IsResizable : std::false_type {};

        template <typename T>
        struct IsResizable <T, decltype((void)std::declval<T>().resize(1u), 0)> : std::true_type {};
    }

    /*
     * serialization/deserialization context
     */
    struct Context {
        void* getCustomPtr();
    };

/*
 * range functions in bitsery namespace because these are used by user
 */

    template<typename T, typename Enable = void>
    struct RangeSpec {

        constexpr RangeSpec(T minValue, T maxValue)
                : min{minValue},
                  max{maxValue},
                  bitsRequired{details::calcRequiredBits(min, max)} {
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
                bitsRequired{details::calcRequiredBits(
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
                bitsRequired{details::calcRequiredBits<details::SAME_SIZE_UNSIGNED<T>>({}, ((max - min) / precision))} {

        }

        const T min;
        const T max;
        const size_t bitsRequired;
    };

    namespace details {

/*
 * functions for range
 */

        template<typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        auto getRangeValue(const T &v, const RangeSpec<T> &r) {
            return static_cast<SAME_SIZE_UNSIGNED<T>>(v - r.min);
        };

        template<typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
        auto getRangeValue(const T &v, const RangeSpec<T> &r) {
            using VT = SAME_SIZE_UNSIGNED<T>;
            return static_cast<VT>(static_cast<VT>(v) - static_cast<VT>(r.min));
        };

        template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        auto getRangeValue(const T &v, const RangeSpec<T> &r) {
            using VT = SAME_SIZE_UNSIGNED<T>;
            const VT maxUint = (static_cast<VT>(1) << r.bitsRequired) - 1;
            const auto ratio = (v - r.min) / (r.max - r.min);
            return static_cast<VT>(ratio * maxUint);
        };

        template<typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        void setRangeValue(T &v, const RangeSpec<T> &r) {
            v += r.min;
        };

        template<typename T, typename std::enable_if<std::is_enum<T>::value>::type * = nullptr>
        void setRangeValue(T &v, const RangeSpec<T> &r) {
            using VT = std::underlying_type_t<T>;
            reinterpret_cast<VT &>(v) += static_cast<VT>(r.min);
        };

        template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type * = nullptr>
        void setRangeValue(T &v, const RangeSpec<T> &r) {
            using UIT = SAME_SIZE_UNSIGNED<T>;
            const auto intRep = reinterpret_cast<UIT &>(v);
            const UIT maxUint = (static_cast<UIT>(1) << r.bitsRequired) - 1;
            v = r.min + (static_cast<T>(intRep) / maxUint) * (r.max - r.min);
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
 * functions for entropy encoding
 */

        template<typename T, size_t N>
        size_t findEntropyIndex(const T &v, const T (&defValues)[N]) {
            auto index{1u};
            for (auto &d:defValues) {
                if (d == v)
                    return index;
                ++index;
            }
            return 0u;
        };

/*
 * functions for object serialization
 */

        template<typename S, typename T, typename Enabled = void>
        struct SerializeFunction {
            static void invoke(S &s, T &v) {
                static_assert(!std::is_void<Enabled>::value, "please define 'serialize' function.");
            }
        };

        template<typename S, typename T>
        struct SerializeFunction<S, T, typename std::enable_if<
                std::is_same<void, decltype(serialize(std::declval<S &>(), std::declval<T &>()))>::value
        >::type> {
            static void invoke(S &s, T &v) {
                serialize(s, v);
            }
        };


/*
 * delta functions
 */

        class ObjectMemoryPosition {
        public:

            template<typename T>
            ObjectMemoryPosition(const T &oldObj, const T &newObj)
                    :ObjectMemoryPosition{reinterpret_cast<const char *>(&oldObj),
                                          reinterpret_cast<const char *>(&newObj),
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

}

#endif //BITSERY_DETAILS_SERIALIZATION_COMMON_H
