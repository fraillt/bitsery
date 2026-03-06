// MIT License
//
// Copyright (c) 2024 Mindaugas Vinkelis and Victor Stewart
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#ifndef BITSERY_DETAILS_SERIALIZER_SHARED_H
#define BITSERY_DETAILS_SERIALIZER_SHARED_H

#include "serialization_common.h"
#include <utility>

namespace bitsery {

namespace details {

template<typename TSerializer>
class SerializerShorthand
{
public:
  template<typename... TArgs>
  TSerializer& operator()(TArgs&&... args)
  {
    archive(std::forward<TArgs>(args)...);
    return derived();
  }

  template<typename T>
  void value1b(T&& v)
  {
    derived().template value<1>(std::forward<T>(v));
  }

  template<typename T>
  void value2b(T&& v)
  {
    derived().template value<2>(std::forward<T>(v));
  }

  template<typename T>
  void value4b(T&& v)
  {
    derived().template value<4>(std::forward<T>(v));
  }

  template<typename T>
  void value8b(T&& v)
  {
    derived().template value<8>(std::forward<T>(v));
  }

  template<typename T>
  void value16b(T&& v)
  {
    derived().template value<16>(std::forward<T>(v));
  }

  template<typename T, typename Ext>
  void ext1b(const T& v, Ext&& extension)
  {
    derived().template ext<1, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext2b(const T& v, Ext&& extension)
  {
    derived().template ext<2, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext4b(const T& v, Ext&& extension)
  {
    derived().template ext<4, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext8b(const T& v, Ext&& extension)
  {
    derived().template ext<8, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T, typename Ext>
  void ext16b(const T& v, Ext&& extension)
  {
    derived().template ext<16, T, Ext>(v, std::forward<Ext>(extension));
  }

  template<typename T>
  void text1b(const T& str, size_t maxSize)
  {
    derived().template text<1>(str, maxSize);
  }

  template<typename T>
  void text2b(const T& str, size_t maxSize)
  {
    derived().template text<2>(str, maxSize);
  }

  template<typename T>
  void text4b(const T& str, size_t maxSize)
  {
    derived().template text<4>(str, maxSize);
  }

  template<typename T>
  void text1b(const T& str)
  {
    derived().template text<1>(str);
  }

  template<typename T>
  void text2b(const T& str)
  {
    derived().template text<2>(str);
  }

  template<typename T>
  void text4b(const T& str)
  {
    derived().template text<4>(str);
  }

  template<typename T>
  void container1b(T&& obj, size_t maxSize)
  {
    derived().template container<1>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container2b(T&& obj, size_t maxSize)
  {
    derived().template container<2>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container4b(T&& obj, size_t maxSize)
  {
    derived().template container<4>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container8b(T&& obj, size_t maxSize)
  {
    derived().template container<8>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container16b(T&& obj, size_t maxSize)
  {
    derived().template container<16>(std::forward<T>(obj), maxSize);
  }

  template<typename T>
  void container1b(T&& obj)
  {
    derived().template container<1>(std::forward<T>(obj));
  }

  template<typename T>
  void container2b(T&& obj)
  {
    derived().template container<2>(std::forward<T>(obj));
  }

  template<typename T>
  void container4b(T&& obj)
  {
    derived().template container<4>(std::forward<T>(obj));
  }

  template<typename T>
  void container8b(T&& obj)
  {
    derived().template container<8>(std::forward<T>(obj));
  }

  template<typename T>
  void container16b(T&& obj)
  {
    derived().template container<16>(std::forward<T>(obj));
  }

protected:
  TSerializer& derived() { return static_cast<TSerializer&>(*this); }
  const TSerializer& derived() const
  {
    return static_cast<const TSerializer&>(*this);
  }

private:
  template<typename T, typename... TArgs>
  void archive(T&& head, TArgs&&... tail)
  {
    BriefSyntaxFunction<TSerializer, T>::invoke(derived(),
                                                std::forward<T>(head));
    archive(std::forward<TArgs>(tail)...);
  }

  void archive() {}
};

}

}

#endif // BITSERY_DETAILS_SERIALIZER_SHARED_H
