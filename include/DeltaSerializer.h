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



#ifndef BITSERY_DELTASERIALIZER_H
#define BITSERY_DELTASERIALIZER_H

#include <array>
#include <stack>
#include <algorithm>
#include "Serializer.h"

namespace bitsery {

    template<typename Writter, typename TObj>
    class DeltaSerializer {
    public:
        DeltaSerializer(Writter &w, const TObj &oldObj, const TObj &newObj)
                : _serializer{w},
                  _writter{w},
                  _oldObj{oldObj},
                  _newObj{newObj},
                  _objMemPos(std::deque<ObjectMemoryPosition>(1, ObjectMemoryPosition{oldObj, newObj})),
                  _isNewElement{false} {

        };

        template<size_t SIZE = 0, typename T, typename std::enable_if<std::is_integral<T>::value>::type * = nullptr>
        DeltaSerializer &value(const T &v) {
            if (setChangedState(v)) {
                constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
                _writter.template writeBytes<ValueSize>(v);
            }
            return *this;
        }

        template<typename T>
        DeltaSerializer &object(T &&obj) {
            if (setChangedState(obj)) {
                serialize(*this, std::forward<T>(obj));
            }
            return *this;
        }

        template<size_t VSIZE = 1, typename T>
        DeltaSerializer &text(const std::basic_string<T> &str, size_t maxSize) {
            if (setChangedState(str)) {
                _serializer.template text<VSIZE>(str, maxSize);
            }
            return *this;

        }

        template<size_t VSIZE = 1, typename T, size_t N>
        DeltaSerializer &text(const T (&str)[N]) {
            if (setChangedState(str)) {
                _serializer.template text<VSIZE>(str);
            }
            return *this;

        }

        template<typename T, size_t N, typename Fnc>
        DeltaSerializer &array(const std::array<T, N> &arr, Fnc &&fnc) {
            if (setChangedState(arr)) {
                if (!_isNewElement) {
                    const auto &old = *_objMemPos.top().getOldObjectField(arr);
                    processContainer(std::begin(old), std::end(old), std::begin(arr), std::end(arr), fnc);
                } else {
                    for (auto &v:arr)
                        fnc(v);
                }
            }
            return *this;
        }


        template<typename T, size_t N, typename Fnc>
        DeltaSerializer &array(const T (&arr)[N], Fnc &&fnc) {
            if (setChangedState(arr)) {
                if (!_isNewElement) {
                    auto old = *_objMemPos.top().getOldObjectField(arr);
                    const T *tmp = arr;
                    processContainer(old, old + N, tmp, tmp + N, fnc);
                } else {
                    const T *tmp = arr;
                    for (auto i = 0u; i < N; ++i, ++tmp)
                        fnc(*tmp);
                }
            }
            return *this;
        }

        template<typename T, typename Fnc>
        DeltaSerializer &container(T &&obj, Fnc &&fnc, size_t maxSize) {
            if (setChangedState(obj)) {
                _writter.writeBits(obj.size(), 32);
                if (!_isNewElement) {
                    auto old = *_objMemPos.top().getOldObjectField(obj);
                    processContainer(std::begin(old), std::end(old), std::begin(obj), std::end(obj),
                                     std::forward<Fnc>(fnc));
                } else {
                    for (auto &v:obj)
                        fnc(v);
                }
            }
            return *this;
        }

    private:
        Serializer <Writter> _serializer;
        Writter &_writter;
        const TObj &_oldObj;
        const TObj &_newObj;
        std::stack<ObjectMemoryPosition> _objMemPos;
        bool _isNewElement;

        template<typename T>
        bool setChangedState(const T &obj) {
            if (!_isNewElement) {
                auto res = !_objMemPos.top().isFieldsEquals(obj);
                writeChangedState(res);
                return res;
            }
            return true;
        }

        template<typename T, size_t N>
        bool setChangedState(const T (&arr)[N]) {
            if (!_isNewElement) {
                auto old = *_objMemPos.top().getOldObjectField(arr);
                auto end = arr + N;
                bool changed{};
                for (auto p = arr, pOld = old; p != end; ++p, ++pOld) {
                    if (!(*p == *pOld)) {
                        changed = true;
                        break;
                    }
                }
                writeChangedState(changed);
                return changed;
            }
            return true;
        }


        template<typename T, typename Fnc>
        void processContainer(const T oldBegin, const T oldEnd, const T begin, const T end, Fnc &&fnc) {
            auto misMatch = std::mismatch(oldBegin, oldEnd, begin, end);
            auto lastChanged = begin;
            while (misMatch.first != oldEnd && misMatch.second != end) {
                writeIndexOffset(std::distance(lastChanged, misMatch.second));
                _objMemPos.emplace(ObjectMemoryPosition{*misMatch.first, *misMatch.second});
                fnc(*misMatch.second);
                _objMemPos.pop();
                ++misMatch.first;
                ++misMatch.second;
                lastChanged = misMatch.second;
                misMatch = std::mismatch(misMatch.first, oldEnd, misMatch.second, end);
            }
            auto p = misMatch.second;
            //write items left
            writeIndexOffset(std::distance(lastChanged, end));
            //write old elements
            for (auto pOld = misMatch.first; p != end && pOld != oldEnd; ++p, ++pOld) {
                _objMemPos.emplace(ObjectMemoryPosition{*pOld, *p});
                fnc(*p);
                _objMemPos.pop();
            }

            //write new elements
            _isNewElement = true;
            for (; p != end; ++p)
                fnc(*p);
            _isNewElement = false;
        }

        void writeChangedState(bool state) {
            _writter.writeBits(state ? 1u : 0u, 1);
        }

        void writeIndexOffset(const size_t offset) {
            //special case, if items are updated sequentialy
            if (offset == 0) {
                _writter.writeBits(1u, 1);
            } else {
                _writter.writeBits(0u, 1);
                auto smallOffset = offset < 16;
                _writter.writeBits(smallOffset ? 1u : 0u, 1);
                if (smallOffset)
                    _writter.writeBits(offset, 4);
                else
                    _writter.writeBits(offset, 32);
            }
        }

    };

}
#endif //BITSERY_DELTASERIALIZER_H
