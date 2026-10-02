
#include <algorithm>
#include <cstddef>
#include <new>
#include <tuple>
#include <type_traits>
#include <utility>
template <int I> struct alt { static constexpr int id = I; long value; };
template <int I> __attribute__((noinline)) long made(long x) { asm volatile("" : "+r"(x)); return x * 3 + I; }
// As splice holds one: a holder derived from one empty base, the pointer to
// it read once before each dispatch.
struct held {};
template <class T> struct holder : held { T value; };
template <class T> T& value_of(held* p) { return static_cast<holder<T>*>(p)->value; }

struct core {};
template <class T, class Rest> struct layer {};
template <class T> struct peel {};
template <class T, class Rest> constexpr layer<T, Rest> operator|(peel<T>, Rest) noexcept { return {}; }
template <class F, class T> [[gnu::always_inline]] constexpr decltype(auto) peel_to(layer<T, core>, std::size_t, F& f) { return f(std::type_identity<T>{}); }
template <class F, class T, class Rest> [[gnu::always_inline]] constexpr decltype(auto) peel_to(layer<T, Rest>, std::size_t index, F& f) {
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
  if constexpr (std::is_void_v<R>) {
    (void)((index == I ? (f(std::type_identity<Ts>{}), true) : false) || ...);
  } else {
    R out{};
    (void)((index == I ? (out = f(std::type_identity<Ts>{}), true) : false) || ...);
    return out;
  }
}
template <class... Ts> struct variant {
  std::size_t index;
  alignas(holder<Ts>...) unsigned char buffer[std::max({sizeof(holder<Ts>)...})];
  held* object;
  template <class R, class F> R visit(F&& f) {
    held* const p = object;  // once, before the dispatch
    auto at = [&f, p]<class T>(std::type_identity<T>) -> R { return f(value_of<T>(p)); };
#if defined(PEEL)
    using onion = decltype((peel<Ts>{} | ... | core{}));
    return peel_to(onion{}, index, at);
#elif defined(FOLD)
    return fold_to<R, decltype(at), Ts...>(index, at, std::index_sequence_for<Ts...>{});
#else
    return halve_to<0, sizeof...(Ts), decltype(at), Ts...>(index, at);
#endif
  }
};
template <std::size_t... I> auto make(std::index_sequence<I...>) -> variant<alt<int(I)>...>;
using big = decltype(make(std::make_index_sequence<290>{}));


struct write_each {
  long seed;
  template <class T> void operator()(T& one) const { one.value = made<T::id>(seed); }
};
struct read {
  template <class T> long operator()(T& one) const { return one.value; }
};
long run(big& v, long seed) {
  v.visit<void>(write_each{seed});
  return v.visit<long>(read{});
}
