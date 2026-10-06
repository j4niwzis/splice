# The fold dispatch, and why it assumes its index

`spl::variant` finds the alternative it holds, in an optimised build, by
one fold over the alternatives in one function (`detail::fold_to`):

```cpp
template <class R, class F, class... Ts, std::size_t... I>
constexpr R fold_to(std::size_t index, F& f, std::index_sequence<I...>) {
  [[assume(index < sizeof...(I))]];
  // ... ((index == I ? (out = f(std::type_identity<Ts>{}), true) : false) || ...);
}
```

**With the `[[assume]]` it optimises well. Without it, terribly.** Do not
take it out.

## What was measured

clang 23, a variant of 290 alternatives, LLVM IR after the `-O3` pipeline,
and run time at `-O3` and `-O3 -flto=full`. The case that matters is not one
dispatch alone but what the optimiser keeps from one dispatch to the next on
the same, unknown index: a first visit writes into the alternative held (a
value from an opaque function per alternative, so nothing folds), a second
visit reads it back. Kept, the two dispatches merge into one switch and the
value goes from the write to the return with no reload.

| dispatch | functions left | IR instructions | i64 loads | merged |
|---|---|---|---|---|
| fold + `[[assume(index < N)]]` | 2 | 903 | 3 | yes |
| fold, no assume | 2 | 2351 | 293 | **no**: every arm reloads |
| onion (`peel_to`) + `always_inline` | 2 | 893 | 3 | yes |
| onion (`peel_to`), no `always_inline` | 73 | 2764 | 290 | no: the inliner stops part way down the recursion |
| halving tree (`halve_to`) | 37 | 2138 | 134 | no |

Without the assumption the fold has to keep a path for an index past the
last alternative, so the switch has a default that does something, and the
second dispatch cannot be threaded through the first.

Compile time of the dispatch alone (290 alternatives, each arm a different
opaque call): fold + assume 0.28 s at `-O3` (0.29 s at `-O0`), onion +
`always_inline` 0.92 s (0.96 s), halving tree 0.90 s.

Run time, 8.2M write-then-read pairs over variants of random alternatives,
median of 7 runs:

| | `-O3` | `-O3 -flto=full` |
|---|---|---|
| onion + `always_inline` | 80.7 ms | 74.3 ms |
| fold + assume | 82.6 ms | 74.3 ms |

Equal within the noise: the same merged switch. The fold is chosen for its
compile time.

The benchmarks are in the mux chat's working directory (`/tmp/dispatch`:
`bench4.cc`, `twice_*.cc`, `store.cc`), each a single translation unit with a
stand-in variant laid out as this one is (a buffer, `holder<T>` on an empty
`held` base, the pointer read once before the dispatch).
