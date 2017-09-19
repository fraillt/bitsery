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
#include "both_common.h"

namespace bitsery {

    namespace details {

        template<typename T, typename Enable = void>
        struct SAME_SIZE_UNSIGNED_TYPE {
            using type = typename std::make_unsigned<T>::type;
        };

        template<typename T>
        struct SAME_SIZE_UNSIGNED_TYPE<T, typename std::enable_if<std::is_enum<T>::value>::type> {
            using type = typename std::make_unsigned<typename std::underlying_type<T>::type>::type;
        };

        template<typename T>
        struct SAME_SIZE_UNSIGNED_TYPE<T, typename std::enable_if<std::is_floating_point<T>::value>::type> {
            using type = typename std::conditional<std::is_same<T, float>::value, uint32_t, uint64_t>::type;
        };

        template<typename T>
        using SAME_SIZE_UNSIGNED = typename SAME_SIZE_UNSIGNED_TYPE<T>::type;

/*
 * functions for object serialization
 */

        template<typename S, typename T, typename Enabled = void>
        struct SerializeFunction {
            static void invoke(S &s, T &v) {
                static_assert(!std::is_void<Enabled>::value,
                                      "\nPlease define 'serialize' function for your type:\n"
                                      "  template<typename S>\n"
                                      "  void serialize(S& s, <YourType>& o)\n"
                                      "  {\n"
                                      "    ...\n"
                                      "  }\n");
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

        //used for extensions, when extension TValue = void
        struct DummyType {
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
