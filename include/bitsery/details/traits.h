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

#ifndef BITSERY_DETAILS_TRAITS_H
#define BITSERY_DETAILS_TRAITS_H

#include <type_traits>
#include <string>

namespace bitsery {
    namespace details {

        /*
         * helper traits that is used internaly, or by other traits
         */
        template <typename T, typename = int>
        struct IsResizable : std::false_type {};

        template <typename T>
        struct IsResizable <T, decltype((void)std::declval<T>().resize(1u), 0)> : std::true_type {};

        /*
         * core library traits, used to extend library for custom types
         */

        //traits for extension
        template<typename Extension, typename T>
        struct ExtensionTraits {
            //this type is used, when using extesion without custom lambda
            // eg.: extension4b>(obj, myextension{}) will call s.value4b(obj) for TValue
            // or extesion(obj, myextension{})  will call s.object(obj) for TValue
            //if this is not defined, then these functions are disabled
            using TValue = void;
        };

        //traits for containers
        template<typename T>
        struct ContainerTraits {

            //default behaviour is resizable if container has method T::resize(size_t)
            static constexpr bool isResizable = IsResizable<T>::value;

            //resize function, called only if container is resizable
            static void resize(T& container, size_t size) {
                container.resize(size);
            }

            //get container size
            static size_t size(const T& container) {
                return container.size();
            }

        };

        //traits for text
        template<typename T>
        struct TextTraits {

            static constexpr bool isResizable = true;

            //resize is without null-terminated character as with std::string,
            //but null terminated character will always be written
            //if you container doesn't add null-terminated character automaticaly, resize it to size+1;
            static void resize(T& container, size_t size) {
                container.resize(size);
            }

            //used for serialization to get text length
            //length is until null-terminated character, size and length might not be equal
            static size_t length(const T& container) {
                auto begin = std::begin(container);
                using TValue = typename std::decay<decltype(*begin)>::type;
                return std::char_traits<TValue>::length(std::addressof(*begin));
            }
        };

        //text traits specialization for std::string
        //for std::string return length as size(), for faster performance, so we don't need to traverse string to find null-terminated characeter
        //although it is not correct behaviour, meaning that string might have null-terminated characters in the middle,
        //but in this case it your decision if you store buffer in string and serialize it as a text.
        template<typename ... Args>
        struct TextTraits<std::basic_string<Args...>> {

            static constexpr bool isResizable = true;

            //resize is without null-terminated character as with std::string,
            //but null terminated character will always be written
            //if you container doesn't add null-terminated character automaticaly, resize it to size+1;
            static void resize(std::basic_string<Args...>& container, size_t size) {
                container.resize(size);
            }

            //used for serialization to get text length
            //length is until null-terminated character, size and length might not be equal
            static size_t length(const std::basic_string<Args...>& container) {
                return container.size();
            }
        };


        //traits only for buffer reader/writer
        template <typename T>
        struct BufferContainerTraits: public ContainerTraits<T> {
            //this function is only used by BufferWriter, when writing data to buffer,
            //it is called only current buffer size is not enough to write.
            //it is used to dramaticaly improve performance by updating buffer directly
            //instead of using back_insert_iterator to append each byte to buffer.
            //thats why BufferWriter return range iterators
            static void increaseBufferSize(T& container) {
                //use default implementation behaviour;
                //call push_back to use default resize strategy
                container.push_back({});
                //after allocation resize to take all capacity
                container.resize(container.capacity());
            }

            using TValue = typename T::value_type;
            using TDifference = typename T::difference_type;
            using TIterator = typename T::iterator;
        };

    }
}

#endif //BITSERY_DETAILS_TRAITS_H
