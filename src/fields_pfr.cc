// SPDX-License-Identifier: GPL-3.0-only
// splice.fields -- An aggregate's fields, as a tuple of references to
// them, through Boost.PFR: for a compiler without C++26's structured
// binding packs (fields_packs.cc), which CMake builds in its place where it
// has them.
export module splice.fields;

import std;
import boost.pfr;

export namespace splice {

template <class Aggregate>
[[nodiscard]] constexpr auto fields_of(Aggregate& value) {
  return boost::pfr::structure_tie(value);
}

}  // namespace splice
