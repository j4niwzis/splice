# splice

What the libraries of the family share, as C++26 modules. One import:

```cpp
import std;
import splice;

struct text { std::string said; };
struct picture { int width, height; };
using message = spl::variant<text, picture>;

std::string kind(const message& one) {
  return spl::visit(spl::overloaded{[](const text&) { return "text"; },
                                          [](const picture&) { return "picture"; }},
                       one);
}
```

- **`spl::variant`** -- `std::variant`'s interface (index, `get`, `get_if`,
  `holds_alternative`, `emplace`, comparisons, C++26's member `visit`), built
  in time linear in how many alternatives it has: an index, a buffer as large
  as the largest, and a pointer to the object in it. libc++ nests a variant as
  deep as it has alternatives, and one of 148 costs 148 * 149 / 2
  instantiations in every unit that makes them all. Outside a release build
  (`SPLICE_ERASED`, on unless `-DSPLICE_ERASE=OFF`) the alternatives are
  reached through tables of function pointers, so that only the tables are
  made; in one, by halving the index, which the optimiser lays out as a jump.
  Everything is `constexpr`.
  A failed replacement can leave the variant valueless: `index()` then
  returns `std::variant_npos`, `get_if` returns null, and `get` and `visit`
  throw `std::bad_variant_access`. It can be assigned or emplaced again;
  valueless variants compare equal and order before variants holding a value.
  Copy and move construction and assignment are available only when every
  alternative supports the corresponding construction. Move-only alternatives
  remain usable in containers even when their moves can throw.
  Moves and swaps are `noexcept` only when every alternative's move
  construction is nonthrowing.
- **`spl::visit`** -- over one or more variants, splice's or std's.
- **`spl::overloaded`** -- a visitor made of several callables.

`spl::erased_call` also propagates exceptions from a held callable's copy or
move construction. A failed assignment leaves an empty, reusable call;
calling it returns an empty result, as for a default-constructed call.

`docs/missed-optimisation-argument-promotion.md`: why the variant reads its
object's pointer once before each dispatch.

## Building

C++23 or newer, CMake 4.3 or newer and clang with libc++ (`import std`). Dependencies come
through [cmake-everywhere](https://github.com/j4niwzis/cmake-everywhere); the
tests use googletest as a module.

```sh
cmake -S . -B build -G Ninja -DCMAKE_CXX_FLAGS=-stdlib=libc++
cmake --build build && ctest --test-dir build
```

## License

GPL-3.0-only.
