// SPDX-License-Identifier: GPL-3.0-only
// splice.fields -- An aggregate's fields, as a tuple of references to
// them, through C++26's structured binding packs. CMake builds this one
// where the compiler has them, and fields_pfr.cc -- the same, through
// Boost.PFR -- where it has not: splice stays a C++23 library.
export module splice.fields;

import std;

export namespace splice {

template <class Aggregate>
[[nodiscard]] constexpr auto fields_of(Aggregate& value) {
  auto& [... field] = value;
  return std::tie(field...);
}

}  // namespace splice
