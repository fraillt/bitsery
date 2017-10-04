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

#ifndef BITSERY_DETAILS_FLEXIBLE_COMMON_H
#define BITSERY_DETAILS_FLEXIBLE_COMMON_H

#include "traits.h"
#include <limits>

namespace bitsery {
    namespace flexible {

        //these function overloads is required to apply maxSize, and optimize for fundamental types
        //for contigous arrays of fundamenal types, memcpy will be applied

        template<typename S, typename T, typename std::enable_if<
                details::IsFundamentalType<typename details::ContainerTraits<T>::TValue>::value
                && details::ContainerTraits<T>::isResizable
        >::type * = nullptr>
        void processContainer(S &s, T &c, size_t maxSize = std::numeric_limits<size_t>::max()) {
            using TValue = typename details::ContainerTraits<T>::TValue;
            s.template container<sizeof(TValue)>(c, maxSize);
        }

        template<typename S, typename T, typename std::enable_if<
                !details::IsFundamentalType<typename details::ContainerTraits<T>::TValue>::value
                && details::ContainerTraits<T>::isResizable
        >::type * = nullptr>
        void processContainer(S &s, T &c, size_t maxSize = std::numeric_limits<size_t>::max()) {
            s.container(c, maxSize);
        }

        template<typename S, typename T, typename std::enable_if<
                details::IsFundamentalType<typename details::ContainerTraits<T>::TValue>::value
                && !details::ContainerTraits<T>::isResizable
        >::type * = nullptr>
        void processContainer(S &s, T &c) {
            using TValue = typename details::ContainerTraits<T>::TValue;
            s.template container<sizeof(TValue)>(c);
        }

        template<typename S, typename T, typename std::enable_if<
                !details::IsFundamentalType<typename details::ContainerTraits<T>::TValue>::value
                && !details::ContainerTraits<T>::isResizable
        >::type * = nullptr>
        void processContainer(S &s, T &c) {
            s.container(c);
        }

        //overloads for text processing to apply maxSize

        template<typename S, typename T, typename std::enable_if<
                details::ContainerTraits<T>::isResizable>::type * = nullptr>
        void processText(S &s, T &c, size_t maxSize = std::numeric_limits<size_t>::max()) {
            using TValue = typename details::ContainerTraits<T>::TValue;
            s.template text<sizeof(TValue)>(c, maxSize);
        }

        template<typename S, typename T, typename std::enable_if<
                !details::ContainerTraits<T>::isResizable>::type * = nullptr>
        void processText(S &s, T &c) {
            using TValue = typename details::ContainerTraits<T>::TValue;
            s.template text<sizeof(TValue)>(c);
        }


        //all wrapper functions, that modify behaviour, should inherit from this
        struct ArchiveWrapperFnc {

        };

        //this type is used to differentiate between container and text behaviour
        template<typename T, size_t N, bool isText>
        struct CArray : public ArchiveWrapperFnc {
            CArray(T (&data_)[N]) : data{data_} {};
            T (&data)[N];
        };

        template<typename S, typename T, size_t N>
        void serialize(S &s, CArray<T, N, true> &str) {
            processText(s, str.data);
        }

        template<typename S, typename T, size_t N>
        void serialize(S &s, CArray<T, N, false> &obj) {
            processContainer(s, obj.data);
        }

        //used to set max container size
        template<typename T>
        struct MaxSize : public ArchiveWrapperFnc {
            MaxSize(T &data_, size_t maxSize_) : data{data_}, maxSize{maxSize_} {};
            T &data;
            size_t maxSize;
        };

        template<typename S, typename T>
        void serialize(S &s, const MaxSize<T> &ms) {
            processContainer(s, ms.data, ms.maxSize);
        };

    }
}

#endif //BITSERY_DETAILS_FLEXIBLE_COMMON_H
