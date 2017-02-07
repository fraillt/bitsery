//
// Created by fraillt on 17.1.10.
//

#ifndef TMP_DELTASERIALIZER_H
#define TMP_DELTASERIALIZER_H

#include <array>
#include <stack>
#include <algorithm>
#include "Common.h"

template<typename Writter, typename TObj>
class DeltaSerializer {
public:
    DeltaSerializer(Writter& w, const TObj& oldObj, const TObj& newObj)
            :_writter{w},
             _oldObj{oldObj},
             _newObj{newObj},
             _objMemPos(std::deque<ObjectMemoryPosition>(1, ObjectMemoryPosition{oldObj, newObj})),
             _isNewElement{false}
    {

    };

    template<size_t SIZE = 0, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    DeltaSerializer& value(const T& v) {
        if (setChangedState(v)) {
            constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
            _writter.template WriteBytes<ValueSize>(v);
        }
        return *this;
    }

    template <typename T>
    DeltaSerializer& object(T&& obj) {
        if (setChangedState(obj)) {
            serialize(*this, std::forward<T>(obj));
        }
        return *this;
    }

    template<typename T>
    DeltaSerializer& text(T&& str) {
        return withContainer(str, [this](auto& v) { value<1>(v);});
    }

    template<typename T, size_t N, typename Fnc>
    DeltaSerializer & withArray(const std::array<T,N> &arr, Fnc&& fnc) {
        if (setChangedState(arr)) {
            if (!_isNewElement) {
                const auto& old = *_objMemPos.top().getOldObjectField(arr);
                processContainer(std::begin(old), std::end(old), std::begin(arr), std::end(arr), fnc);
            } else {
                for (auto& v:arr)
                    fnc(v);
            }
        }
        return *this;
    }


    template<typename T, size_t N, typename Fnc>
    DeltaSerializer& withArray(const T (&arr)[N], Fnc&& fnc) {
        if (setChangedState(arr)) {
            if (!_isNewElement) {
                auto old = *_objMemPos.top().getOldObjectField(arr);
                const T* tmp = arr;
                processContainer(old, old + N, tmp, tmp + N, fnc);
            } else {
                const T* tmp = arr;
                for (auto i=0u; i < N; ++i, ++tmp)
                    fnc(*tmp);
            }
        }
        return *this;
    }

    template <typename T, typename Fnc>
    DeltaSerializer& withContainer(T&& obj, Fnc&& fnc) {
        if(setChangedState(obj)) {
            _writter.template WriteBits<32>(obj.size());
            if (!_isNewElement) {
                auto old = *_objMemPos.top().getOldObjectField(obj);
                processContainer(std::begin(old), std::end(old), std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
            } else {
                for (auto& v:obj)
                    fnc(v);
            }
        }
        return *this;
    }

private:
    Writter& _writter;
    const TObj& _oldObj;
    const TObj& _newObj;
    std::stack<ObjectMemoryPosition> _objMemPos;
    bool _isNewElement;

    template <typename T>
    bool setChangedState(const T& obj) {
        if (!_isNewElement) {
            auto res = !_objMemPos.top().isFieldsEquals(obj);
            writeChangedState(res);
            return res;
        }
        return true;
    }

    template <typename T, size_t N>
    bool setChangedState(const T (&arr)[N]) {
        if (!_isNewElement) {
            auto old = *_objMemPos.top().getOldObjectField(arr);
            auto end = arr + N;
            bool changed{};
            for (auto p=arr, pOld=old; p != end; ++p, ++pOld) {
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


    template <typename T, typename Fnc>
    void processContainer(const T oldBegin, const T oldEnd, const T begin, const T end, Fnc&& fnc) {
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
        _writter.template WriteBits<1>(state ? 1u : 0u);
    }

    void writeIndexOffset(const size_t offset) {
        //special case, if items are updated sequentialy
        if (offset == 0) {
            _writter.template WriteBits<1>(1u);
        } else {
            _writter.template WriteBits<1>(0u);
            auto smallOffset = offset < 16;
            _writter.template WriteBits<1>(smallOffset ? 1u : 0u);
            if (smallOffset)
                _writter.template WriteBits<4>(offset);
            else
                _writter.template WriteBits<32>(offset);
        }
    }

};

#endif //TMP_DELTASERIALIZER_H
