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

#ifndef BITSERY_EXT_GROWABLE_H
#define BITSERY_EXT_GROWABLE_H

#include "../traits/core/traits.h"

namespace bitsery {

    namespace ext {

        /*
         * enables to add additional serialization methods at the end of method, without breaking existing older code
         */
        class Growable {
        public:

            template<typename Ser, typename Writer, typename T, typename Fnc>
            void serialize(Ser &, Writer &writer, const T &obj, Fnc &&fnc) const {
                writer.beginSession();
                fnc(const_cast<T&>(obj));
                writer.endSession();
            }

            template<typename Des, typename Reader, typename T, typename Fnc>
            void deserialize(Des &, Reader &reader, T &obj, Fnc &&fnc) const {
                reader.beginSession();
                fnc(obj);
                reader.endSession();
            }
        };
    }

    namespace traits {
        template<typename T>
        struct ExtensionTraits<ext::Growable, T> {
            using TValue = T;
            static constexpr bool SupportValueOverload = false;
            static constexpr bool SupportObjectOverload = true;
            static constexpr bool SupportLambdaOverload = true;
        };
    }

}


#endif //BITSERY_EXT_GROWABLE_H
