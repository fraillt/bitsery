#ifndef BITSERY_EXT_DIRECT_WRITE_BUFFER_H
#define BITSERY_EXT_DIRECT_WRITE_BUFFER_H

#include "../traits/core/traits.h"

namespace bitsery {
namespace ext {

template <typename TSize, size_t Alignment>
class DirectWriteBuffer
{
public:
  explicit DirectWriteBuffer(TSize &size)
    : _size{size} {}

  template <typename Ser, typename T, typename Fnc>
  void serialize(Ser &ser, const T &obj, Fnc &&fnc) const
  {
    auto &writer = ser.adapter();

    // write size
    writer.template writeBytes<sizeof(TSize)>(static_cast<TSize>(_size));

    // get pointer to aligned region
    auto *dst = writer.template allocateForDirectWrite<Alignment>(_size);

    // let user fill the region (third-party API call)
    fnc(ser, obj, dst, _size);
  }

  template <typename Des, typename T, typename Fnc>
  void deserialize(Des &des, T &obj, Fnc &&fnc) const
  {
    auto &reader = des.adapter();

    reader.template readBytes<sizeof(TSize)>(_size);

    // call a provided fnc that knows how to interpret the buffer.
    fnc(des, obj, reader);
  }

private:
  TSize &_size;
};

} // namespace ext

namespace traits {

template <typename TSize, size_t Alignment, typename T>
struct ExtensionTraits<ext::DirectWriteBuffer<TSize, Alignment>, T>
{
  using TValue = void;
  static constexpr bool SupportValueOverload = false;
  static constexpr bool SupportObjectOverload = true;
  static constexpr bool SupportLambdaOverload = true;
};

} // namespace traits
} // namespace bitsery

#endif // BITSERY_EXT_DIRECT_WRITE_BUFFER_H
