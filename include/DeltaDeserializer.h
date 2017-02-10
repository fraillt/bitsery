//
// Created by fraillt on 17.1.10.
//

#ifndef TMP_DELTADESERIALIZER_H
#define TMP_DELTADESERIALIZER_H

#include <array>
#include <stack>
#include <algorithm>
#include "Deserializer.h"

template<typename Reader, typename TObj>
class DeltaDeserializer {
public:
    DeltaDeserializer(Reader& r, const TObj& oldObj, const TObj& newObj)
            :_deserializer{r},
             _reader{r},
             _oldObj{oldObj},
             _newObj{newObj},
             _objMemPos(std::deque<ObjectMemoryPosition>(1, ObjectMemoryPosition{oldObj, newObj})),
             _isNewElement{false}
    {
    };

    template<size_t SIZE = 0, typename T, typename std::enable_if<std::is_integral<T>::value>::type* = nullptr>
    DeltaDeserializer& value(T& v) {
        if (getChangedState(v)) {
            constexpr size_t ValueSize = SIZE == 0 ? sizeof(T) : SIZE;
            _reader.template readBytes<ValueSize>(v);
        }
        return *this;
    }

    template <typename T>
    DeltaDeserializer& object(T&& obj) {
        if (getChangedState(obj))
            return serialize(*this, std::forward<T>(obj));
        return *this;
    }

    template<typename T>
    DeltaDeserializer& text(T&& str) {
        if (getChangedState(str)) {
            _deserializer.text(str);
        }
        return *this;
    }



    template<typename T, size_t N, typename Fnc>
    DeltaDeserializer & array(std::array<T,N> &arr, Fnc&& fnc) {
        if (getChangedState(arr)) {
            if (!_isNewElement) {
                const auto old = *_objMemPos.top().getOldObjectField(arr);
                processContainer(std::begin(old), std::end(old), std::begin(arr), std::end(arr), fnc);
            } else {
                for (auto& v:arr)
                    fnc(v);
            }
        }
        return *this;
    }


    template<typename T, size_t N, typename Fnc>
    DeltaDeserializer& array(T (&arr)[N], Fnc&& fnc) {
        if (getChangedState(arr)) {
            if (!_isNewElement) {
                const auto old = *_objMemPos.top().getOldObjectField(arr);
                T* tmp = arr;
                processContainer(old, old + N, tmp, tmp + N, fnc);
            } else {
                T* tmp = arr;
                for (auto i=0u; i < N; ++i, ++tmp)
                    fnc(*tmp);
            }
        }
        return *this;
    }

    template <typename T, typename Fnc>
    DeltaDeserializer& container(T& obj, Fnc&& fnc) {
        if(getChangedState(obj)) {
            size_t newSize{};
            _reader.readBits(newSize, 32);
            if (!_isNewElement) {
                auto old = *_objMemPos.top().getOldObjectField(obj);
                if (old.size() != newSize)
                    obj.resize(newSize);
                processContainer(std::begin(old), std::end(old), std::begin(obj), std::end(obj), std::forward<Fnc>(fnc));
            } else {
                obj.resize(newSize);
                for (auto& v:obj)
                    fnc(v);
            }
        }
        return *this;
    }

private:
    Deserializer<Reader> _deserializer;
    Reader& _reader;

    const TObj& _oldObj;
    const TObj& _newObj;
    std::stack<ObjectMemoryPosition> _objMemPos;
    bool _isNewElement;

    template <typename T>
    bool getChangedState(T& obj) {
        if (!_isNewElement) {
            if (!readChangedState()) {
                obj = *_objMemPos.top().getOldObjectField(obj);
                return false;
            }
        }
        return true;
    }

    template <typename T, size_t N>
    bool getChangedState(T (&arr)[N]) {
        if (!_isNewElement) {
            if (!readChangedState()) {
                auto old = *_objMemPos.top().getOldObjectField(arr);
                auto end = arr + N;
                auto pOld = old;
                for (auto p=arr; p != end; ++p, ++pOld)
                    *p = *pOld;
                return false;
            }
        }
        return true;
    }

    template <typename TConstIt, typename TIt, typename Fnc>
    bool processContainer(TConstIt oldBegin, TConstIt oldEnd, TIt begin, TIt end, Fnc&& fnc) {
        auto offset = readIndexOffset();
        auto p = begin;
        auto pOld = oldBegin;
        for (; p != end && pOld != oldEnd; ++p, ++pOld) {
            if (offset) {
                *p = *pOld;
                --offset;
            } else {
                _objMemPos.emplace(ObjectMemoryPosition{*pOld, *p});
                fnc(*p);
                _objMemPos.pop();
                offset = readIndexOffset();
            }
        }
        if (offset != 0 && pOld != oldEnd)
            return false;
        _isNewElement = true;
        for (; p != end; ++p, --offset)
            fnc(*p);
        _isNewElement = false;
        return offset == 0;

    }

    bool readChangedState() {
        unsigned char res{};
        _reader.readBits(res, 1);
        return res;
    }



    size_t readIndexOffset() {
        //special case, if items are updated sequentialy
        unsigned char tmp{};
        _reader.readBits(tmp, 1);
        if (tmp) {
            return 0u;
        }
        else {
            size_t res{};
            _reader.readBits(tmp, 1);
            if (tmp > 0)
                _reader.readBits(res, 4);
            else
                _reader.readBits(res, 32);
            return res;
        }

    }

};

#endif //TMP_DELTADESERIALIZER_H
