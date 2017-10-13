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


#ifndef BITSERY_FLEXIBLE_TYPE_STD_UNORDERED_MAP_H
#define BITSERY_FLEXIBLE_TYPE_STD_UNORDERED_MAP_H

#include <unordered_map>
#include "../ext/std_map.h"

namespace bitsery {
    template<typename S, typename ... TArgs>
    void serialize(S &s, std::unordered_map<TArgs ... > &obj, size_t maxSize = std::numeric_limits<size_t>::max()) {
        using TKey = typename std::unordered_map<TArgs...>::key_type;
        using TValue = typename std::unordered_map<TArgs...>::mapped_type;
        s.ext(obj, ext::StdMap{maxSize},
              [&s](TKey& key, TValue& value) {
                  s.object(key);
                  s.object(value);
              });
    }

    template<typename S, typename ... TArgs>
    void serialize(S &s, std::unordered_multimap<TArgs ... > &obj, size_t maxSize = std::numeric_limits<size_t>::max()) {
        using TKey = typename std::unordered_multimap<TArgs...>::key_type;
        using TValue = typename std::unordered_multimap<TArgs...>::mapped_type;
        s.ext(obj, ext::StdMap{maxSize},
              [&s](TKey& key, TValue& value) {
                  s.object(key);
                  s.object(value);
              });
    }

}

#endif //BITSERY_FLEXIBLE_TYPE_STD_UNORDERED_MAP_H
