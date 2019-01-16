//MIT License
//
//Copyright (c) 2018 Mindaugas Vinkelis
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

#ifndef BITSERY_EXT_COMPACT_VALUE_H
#define BITSERY_EXT_COMPACT_VALUE_H

#include "../details/serialization_common.h"
#include "../details/adapter_common.h"
#include <cassert>

namespace bitsery {

    namespace details {

        template <bool CheckOverflow>
        class CompactValueImpl {
        public:

            template<typename Ser, typename Writer, typename T, typename Fnc>
            void serialize(Ser &s, Writer &writer, const T &v, Fnc &&) const {
                static_assert(std::is_integral<T>::value || std::is_enum<T>::value, "");
                using TValue = typename IntegralFromFundamental<T>::TValue;
                serializeImpl(s, writer, reinterpret_cast<const TValue&>(v), std::integral_constant<bool, sizeof(T) != 1>{});
            }

            template<typename Des, typename Reader, typename T, typename Fnc>
            void deserialize(Des &d, Reader &reader, T &v, Fnc &&) const {
                static_assert(std::is_integral<T>::value || std::is_enum<T>::value, "");
                using TValue = typename IntegralFromFundamental<T>::TValue;
                deserializeImpl(d, reader, reinterpret_cast<TValue &>(v), std::integral_constant<bool, sizeof(T) != 1>{});
            }

        private:

            // if value is 1byte size, just serialize/ deserialize whole value
            template<typename Ser, typename Writer, typename T>
            void serializeImpl(Ser &s, Writer &, const T &v, std::false_type) const {
                s.value1b(v);
            }

            template<typename Des, typename Reader, typename T>
            void deserializeImpl(Des &d, Reader &, T &v, std::false_type) const {
                d.value1b(v);
            }

            // when value is bigger than 1byte size,
            template<typename Ser, typename Writer, typename T>
            void serializeImpl(Ser &, Writer &writer, const T &v, std::true_type) const {
                auto val = zigZagEncode(v, std::is_signed<typename IntegralFromFundamental<T>::TValue>{});
                writeBytes(writer, val);
            }


            template<typename Des, typename Reader, typename T>
            void deserializeImpl(Des &, Reader &reader, T &v, std::true_type) const {
                using TUnsigned = SameSizeUnsigned<T>;
                TUnsigned res{};
                readBytes(reader, res);
                v = zigZagDecode<T>(res, std::is_signed<typename IntegralFromFundamental<T>::TValue>{});
            }

            // zigzag encode signed types
            template<typename T>
            const SameSizeUnsigned<T> &zigZagEncode(const T &v, std::false_type) const {
                return v;
            }

            template<typename TResult, typename TUnsigned>
            const TResult &zigZagDecode(const TUnsigned &v, std::false_type) const{
                return v;
            }

            template<typename T>
            SameSizeUnsigned<T> zigZagEncode(const T &v, std::true_type) const {
                return (v << 1) ^ (v >> (BitsSize<T>::value - 1));
            }

            template<typename TResult, typename TUnsigned>
            TResult zigZagDecode(TUnsigned v, std::true_type) const {
                return (v >> 1) ^ (~(v & 1) + 1); // same as -(v & 1), but no warning on VisualStudio
            }

            // write/read bytes one by one
            template<typename Writer, typename T>
            void writeBytes(Writer &w, const T &v) const {
                auto val = v;
                while(val > 0x7Fu) {
                    w.template writeBytes<1>(static_cast<uint8_t>(val | 0x80u));
                    val >>=7u;
                }
                w.template writeBytes<1>(static_cast<uint8_t>(val));
            }

            template<typename Reader, typename T>
            void readBytes(Reader &r, T &v) const {
                constexpr auto TBITS = sizeof(T)*8;
                uint8_t b1{0x80u};
                auto i = 0u;
                for (;i < TBITS && b1 > 0x7Fu; i +=7u) {
                    r.template readBytes<1>(b1);
                    v += static_cast<T>(b1 & 0x7Fu) << i;
                }
                checkReadOverflow<Reader, T>(r, i, b1, std::integral_constant<bool, CheckOverflow>{});
            }
            template <typename Reader, typename T>
            void checkReadOverflow(Reader &r, unsigned shiftedBy, uint8_t remainder, std::true_type) const {
                constexpr auto TBITS = sizeof(T)*8;
                if (shiftedBy > TBITS && remainder >> (TBITS + 7 - shiftedBy)) {
                    r.setError(bitsery::ReaderError::DataOverflow);
                }
            }

            template <typename Reader, typename T>
            void checkReadOverflow(Reader &, unsigned , uint8_t , std::false_type) const {

            }

        };

    }

    namespace ext {

        // this type will use value overload, and do not check if type is sufficiently large during deserialization
        class CompactValue: public details::CompactValueImpl<false> {};

        // this type will enable object overload, and set DataOverflow if value doesn't fit in type, during deserialization
        class CompactValueAsObject: public details::CompactValueImpl<true> {};

    }

    namespace traits {
        template<typename T>
        struct ExtensionTraits<ext::CompactValue, T> {
            using TValue = T;
            static constexpr bool SupportValueOverload = true;
            // disable object overload, because we don't have implemented serialization function for fundamental types
            static constexpr bool SupportObjectOverload = false;
            static constexpr bool SupportLambdaOverload = false;
        };

        template<typename T>
        struct ExtensionTraits<ext::CompactValueAsObject, T> {
            // use dummy implemenations for value and object overload
            using TValue = void;
            // only enable object overload
            static constexpr bool SupportValueOverload = false;
            static constexpr bool SupportObjectOverload = true;
            static constexpr bool SupportLambdaOverload = false;
        };

    }

}


#endif //BITSERY_EXT_COMPACT_VALUE_H
