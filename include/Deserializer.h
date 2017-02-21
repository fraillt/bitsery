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


#ifndef BITSERY_DESERIALIZER_H
#define BITSERY_DESERIALIZER_H

#include "Common.h"
#include <array>
#include <utility>

namespace bitsery {

    /*
     * functions for range
     */
    template<typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    void setRangeValue(T& v, const RangeSpec<T>& r) {
        v += r.min;
    };

    template<typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
    void setRangeValue(T& v, const RangeSpec<T>& r) {
        using VT = std::underlying_type_t<T>;
        reinterpret_cast<VT&>(v) += static_cast<VT>(r.min);
    };

    template<typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
    void setRangeValue(T& v, const RangeSpec<T>& r) {
        using UIT = SAME_SIZE_UNSIGNED<T>;
        const auto intRep = reinterpret_cast<UIT&>(v);
        const UIT maxUint = (static_cast<UIT>(1) << r.bitsRequired) - 1;
        v = r.min + (static_cast<T>(intRep) / maxUint) * (r.max - r.min);
    };


    template<typename Reader>
    class Deserializer {
    public:
        Deserializer(Reader& r):_reader{r}, _isValid{true} {};

        template <typename T>
        Deserializer& object(T&& obj) {
            return serialize(*this, std::forward<T>(obj));
        }

        /*
         * value overloads
         */

        template<size_t VSIZE = 0, typename T, typename std::enable_if<std::is_floating_point<T>::value>::type* = nullptr>
        Deserializer& value(T& v) {
            static_assert(std::numeric_limits<float>::is_iec559, "");
            static_assert(std::numeric_limits<double>::is_iec559, "");
            if (_isValid) {
                constexpr size_t ValueSize = VSIZE == 0 ? sizeof(T) : VSIZE;
                _isValid = _reader.template readBytes<ValueSize>(reinterpret_cast<SAME_SIZE_UNSIGNED<T>&>(v));
            }
            return *this;
        }

        template<size_t VSIZE = 0, typename T, typename std::enable_if<std::is_enum<T>::value>::type* = nullptr>
        Deserializer& value(T& v) {
            constexpr size_t ValueSize = VSIZE == 0 ? sizeof(T) : VSIZE;
            using UT = std::underlying_type_t<T>;
            if (_isValid) {
                _isValid = _reader.template readBytes<ValueSize>(reinterpret_cast<UT&>(v));
            }
            return *this;
        }

        template<size_t VSIZE = 0, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
        Deserializer& value(T& v) {
            constexpr size_t ValueSize = VSIZE == 0 ? sizeof(T) : VSIZE;
            if (_isValid) {
                _isValid = _reader.template readBytes<ValueSize>(v);
            }
            return *this;
        }

        /*
         * bool
         */

        Deserializer& boolBit(bool& v) {
            if (_isValid) {
                unsigned char tmp;
                _isValid = _reader.readBits(tmp, 1);
                v = tmp == 1;
            }
            return *this;
        }

        Deserializer& boolByte(bool& v) {
            if (_isValid) {
                unsigned char tmp;
                _isValid = _reader.template readBytes<1>(tmp);
                if (_isValid)
                    _isValid = tmp < 2;
                v = tmp == 1;
            }
            return *this;
        }

        /*
         * range
         */

        template <typename T>
        Deserializer& range(T& v, const RangeSpec<T>& range) {
            if (_isValid) {
                _isValid = _reader.template readBits(reinterpret_cast<SAME_SIZE_UNSIGNED<T>&>(v), range.bitsRequired);
                setRangeValue(v, range);
                if (_isValid)
                    _isValid = isRangeValid(v, range);
            }
            return *this;
        }

        /*
         * substitution overloads
         */
        template<typename T, size_t N, typename Fnc>
        Deserializer& substitution(T& v, const std::array<T,N>& expectedValues, Fnc&& fnc) {
            size_t index;
            range(index, {{}, N + 1});
            if (_isValid) {
                if (index)
                    v = expectedValues[index-1];
                else
                    fnc(v);
            }
            return *this;
        };

        template<size_t VSIZE, typename T, size_t N>
        Deserializer& substitution(T& v, const std::array<T,N>& expectedValues) {
            size_t index;
            range(index, {{}, N + 1});
            if (_isValid) {
                if (index)
                    v = expectedValues[index-1];
                else
                    ProcessAnyType<VSIZE>::serialize(*this, v);
            }
            return *this;
        };

        template<typename T, size_t N>
        Deserializer& substitution(T& v, const std::array<T,N>& expectedValues) {
            size_t index;
            range(index, {{}, N + 1});
            if (_isValid) {
                if (index)
                    v = expectedValues[index-1];
                else
                    ProcessAnyType<ARITHMETIC_OR_ENUM_SIZE<T>>::serialize(*this, v);
            }
            return *this;
        };

        /*
         * text overloads
         */

        template <size_t VSIZE = 1, typename T>
        Deserializer& text(std::basic_string<T>& str, size_t maxSize) {
            size_t size;
            readSize(size, maxSize);
            if (_isValid) {
                str.resize(size);
                if (size) {
                    //if (std::is_const<decltype(std::declval<std::basic_string<T>>().data())>::value) {
                        std::vector<T> buf(size);
                        _isValid = _reader.template readBuffer<VSIZE>(buf.data(), size);
                        str.assign(buf.data(), size);
                    //} else {
                        //_isValid = _reader.template readBuffer<VSIZE>(str.data(), size);
                    //}
                }
            }
            return *this;
        }

        template<size_t VSIZE=1, typename T, size_t N>
        Deserializer& text(T (&str)[N]) {
            size_t size;
            readSize(size, N-1);
            if (_isValid) {
                _isValid = _reader.template readBuffer<VSIZE>(str, size);
                str[size] = {};
            }
            return *this;
        }

        /*
         * container overloads
         */

        template <typename T, typename Fnc>
        Deserializer& container(T&& obj, Fnc&& fnc, size_t maxSize) {
            decltype(obj.size()) size{};
            readSize(size, maxSize);
            if (_isValid) {
                obj.resize(size);
                for (auto& v:obj) {
                    if (_isValid)
                        fnc(v);
                }

            }
            return *this;
        }

        template <size_t VSIZE, typename T>
        Deserializer& container(T& obj, size_t maxSize) {
            decltype(obj.size()) size{};
            readSize(size, maxSize);
            if (_isValid) {
                obj.resize(size);
                procContainer<VSIZE>(obj);
            }
            return *this;
        }

        template <typename T>
        Deserializer& container(T& obj, size_t maxSize) {
            decltype(obj.size()) size{};
            readSize(size, maxSize);
            if (_isValid) {
                obj.resize(size);
                procContainer<ARITHMETIC_OR_ENUM_SIZE<typename T::value_type>>(obj);
            }
            return *this;
        }

        /*
         * array overloads (fixed size array (std::array, and c-style array))
         */

        //std::array overloads

        template<typename T, size_t N, typename Fnc>
        Deserializer&  array(std::array<T,N> &arr, Fnc && fnc) {
            for (auto& v: arr)
                if (_isValid)
                    fnc(v);
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Deserializer&  array(std::array<T,N> &arr) {
            procContainer<VSIZE>(arr);
            return *this;
        }

        template<typename T, size_t N>
        Deserializer&  array(std::array<T,N> &arr) {
            procContainer<ARITHMETIC_OR_ENUM_SIZE<T>>(arr);
            return *this;
        }

        //c-style array overloads

        template<typename T, size_t N, typename Fnc>
        Deserializer& array(T (&arr)[N], Fnc&& fnc) {
            T* tmp = arr;
            for (auto i = 0u; i < N; ++i, ++tmp)
                if (_isValid)
                    fnc(*tmp);
            return *this;
        }

        template<size_t VSIZE, typename T, size_t N>
        Deserializer& array(T (&arr)[N]) {
            procCArray<VSIZE>(arr);
            return *this;
        }

        template<typename T, size_t N>
        Deserializer& array(T (&arr)[N]) {
            procCArray<ARITHMETIC_OR_ENUM_SIZE<T>>(arr);
            return *this;
        }
        bool isValid() const {
            return _isValid;
        }

    private:
        Reader& _reader;
        bool _isValid;
        void readSize(size_t &size, size_t maxSize) {
            size = {};
            if (_isValid) {
                unsigned char firstBit;
                _isValid = _reader.readBits(firstBit, 1);
                if (_isValid) {
                    if (firstBit) {
                        _isValid = _reader.readBits(size, 7);
                    } else {
                        unsigned char secondBit;
                        _isValid = _reader.readBits(secondBit, 1);
                        if (_isValid) {
                            if (secondBit) {
                                _isValid = _reader.readBits(size,14);
                            } else {
                                _isValid = _reader.readBits(size,30);
                            }
                        }
                    }
                }
                if (_isValid)
                    _isValid = size <= maxSize;
            }
        }
        template <size_t VSIZE, typename T>
        void procContainer(T&& obj) {
            //todo could be improved for arithmetic types in contiguous containers (std::vector, std::array) (keep in mind std::vector<bool> specialization)
            for (auto& v: obj)
                if (_isValid)
                    ProcessAnyType<VSIZE>::serialize(*this, v);
        };

        template <size_t VSIZE, typename T, size_t N>
        void procCArray(T (&arr)[N]) {
            //todo could be improved for arithmetic types
            T* end = arr + N;
            for (T* it = arr; it != end; ++it)
                if (_isValid)
                    ProcessAnyType<VSIZE>::serialize(*this, *it);
        };

    };


}

#endif //BITSERY_DESERIALIZER_H
