
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>
// Each arm does something the optimiser cannot fold into arithmetic of the
// index: a call it cannot see into.
long seen(long);
template <int I> struct alt { static constexpr int id = I; long value = I * 7; };
template <int I> __attribute__((noinline)) long seen_t(long x) { asm volatile("" : "+r"(x)); return x + I; }
struct core {};
template <class T, class Rest> struct layer {};
template <class T> struct peel {};
template <class T, class Rest> constexpr layer<T, Rest> operator|(peel<T>, Rest) noexcept { return {}; }
template <class F, class T> constexpr decltype(auto) peel_to(layer<T, core>, std::size_t, F& f) { return f(std::type_identity<T>{}); }
template <class F, class T, class Rest> constexpr decltype(auto) peel_to(layer<T, Rest>, std::size_t index, F& f) {
  if (index == 0) return f(std::type_identity<T>{});
  return peel_to(Rest{}, index - 1, f);
}
template <std::size_t Lo, std::size_t Hi, class F, class... Ts> constexpr decltype(auto) halve_to(std::size_t index, F& f) {
  if constexpr (Hi - Lo == 1) {
    return f(std::type_identity<std::tuple_element_t<Lo, std::tuple<Ts...>>>{});
  } else {
    constexpr std::size_t mid = Lo + (Hi - Lo) / 2;
    if (index < mid) return halve_to<Lo, mid, F, Ts...>(index, f);
    return halve_to<mid, Hi, F, Ts...>(index, f);
  }
}
template <class R, class F, class... Ts, std::size_t... I>
constexpr R fold_to(std::size_t index, F& f, std::index_sequence<I...>) {
  R out{};
  (void)((index == I ? (out = f(std::type_identity<Ts>{}), true) : false) || ...);
  return out;
}
template <class... Ts> struct variant {
  std::size_t index;
  long dispatch() const {
    auto at = []<class T>(std::type_identity<T>) -> long { return seen_t<T::id>(T{}.value); };
#if defined(PEEL)
    using onion = decltype((peel<Ts>{} | ... | core{}));
    return peel_to(onion{}, index, at);
#elif defined(FOLD)
    return fold_to<long, decltype(at), Ts...>(index, at, std::index_sequence_for<Ts...>{});
#else
    return halve_to<0, sizeof...(Ts), decltype(at), Ts...>(index, at);
#endif
  }
};
template <std::size_t... I> auto make(std::index_sequence<I...>) -> variant<alt<int(I)>...>;
using big = decltype(make(std::make_index_sequence<290>{}));
long run(const big& v) { return v.dispatch(); }
__attribute__((noinline)) long seen(long x) { asm volatile("" : "+r"(x)); return x; }
int main(int argc, char**) { big v{static_cast<std::size_t>(argc) % 290}; return static_cast<int>(run(v)); }
