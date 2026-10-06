// SPDX-License-Identifier: GPL-3.0-only
// splice.held -- An object of a type said only where it is made, held in
// storage of its holder's: what spl::variant holds its alternative in,
// and spl::erased_call its call.
//
// The object is held in a holder<T>, derived from one empty base, held, the
// pointer to it kept as: a base pointer cast down to its holder is a
// constant expression everywhere, where a void* cast back is not. At run
// time the holder is put in the buffer given (placement new); in constant
// evaluation it is allocated instead -- placement into a buffer is not a
// constant expression -- and let go of as it is destroyed.
export module splice.held;

import std;

export namespace spl {

// What every holder derives from: one base for the pointer, empty.
struct held {};
template <class T>
struct holder : held {
  T value;
  template <class... Args>
  constexpr explicit holder(std::in_place_t, Args&&... args) : value(std::forward<Args>(args)...) {}
};
// The object a pointer to its holder's base holds, as a T: it being one is
// the caller's to know.
template <class T>
[[nodiscard]] constexpr T& value_of(held* p) noexcept {
  return static_cast<holder<T>*>(p)->value;
}
template <class T>
[[nodiscard]] constexpr const T& value_of(const held* p) noexcept {
  return static_cast<const holder<T>*>(p)->value;
}
// A T made in `buffer` -- large and aligned enough for its holder -- or, in
// constant evaluation, allocated: the base pointer it is then kept as.
template <class T, class... Args>
[[nodiscard]] constexpr held* make_held(unsigned char* buffer, Args&&... args) {
  if consteval {
    return new holder<T>(std::in_place, std::forward<Args>(args)...);
  } else {
    return ::new (static_cast<void*>(buffer)) holder<T>(std::in_place, std::forward<Args>(args)...);
  }
}
// The T made by make_held, destroyed (and, in constant evaluation, let go of).
template <class T>
constexpr void destroy_held(held* object) {
  if consteval {
    delete static_cast<holder<T>*>(object);
  } else {
    static_cast<holder<T>*>(object)->~holder();
  }
}

}  // namespace spl
