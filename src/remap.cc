// SPDX-License-Identifier: GPL-3.0-only
// splice.remap -- One aggregate made of another's fields, each matched by
// its type: what a part needs, taken from what the whole it is in was
// given.
//
//   struct window_needs { actions* ask; const palette* colours; speaker* sound; };
//   struct row_needs { const palette* colours; speaker* sound; };
//   const row_needs mine = spl::remapped<row_needs>(given);
//
// Each field of `To` is taken from the field of `from` of its type. The
// match is overload resolution -- one overload for each of `from`'s fields,
// asked with the type of each field of `To` -- so a type `from` has not, or
// has twice, does not compile, and no type is compared with another.
export module splice.remap;

import std;
import splice.fields;
import splice.overloaded;

export namespace spl {

// A type, carried as a value: what a field is asked for by.
template <class T>
struct of_type {};

template <class To, class From>
[[nodiscard]] constexpr To remapped(const From& from) {
  return std::apply(
      [](const auto&... had) {
        const auto pick = overloaded{[&had](of_type<std::remove_cvref_t<decltype(had)>>) { return had; }...};
        To out{};
        std::apply([&pick](auto&... wanted) { ((wanted = pick(of_type<std::remove_cvref_t<decltype(wanted)>>{})), ...); },
                   fields_of(out));
        return out;
      },
      fields_of(from));
}

}  // namespace spl
