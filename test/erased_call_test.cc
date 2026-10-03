// splice::erased_call: called as what it holds, copied and moved with it,
// empty answering nothing -- at run time and in constant evaluation.
import std;
import splice;
import gtest;

#include "gtest/gtest-macros.h"

namespace {

struct adds {
  int by;
  constexpr int operator()(int x) const { return x + by; }
};
struct counts {
  int* n;
  constexpr void operator()() const { ++*n; }
};
// A call that counts its copies, its moves and the ones alive.
struct tracked {
  static inline int alive = 0;
  int* calls;
  tracked(int* c) : calls(c) { ++alive; }
  tracked(const tracked& other) : calls(other.calls) { ++alive; }
  tracked(tracked&& other) noexcept : calls(other.calls) { ++alive; }
  ~tracked() { --alive; }
  void operator()() const { ++*calls; }
};

// In constant evaluation: made, called, copied, moved, assigned, emptied.
constexpr int in_constant_evaluation() {
  splice::erased_call<int(int)> add{adds{2}};
  splice::erased_call<int(int)> copy = add;
  splice::erased_call<int(int)> moved = std::move(copy);
  splice::erased_call<int(int)> empty;
  int n = 0;
  splice::erased_call<void()> count{counts{&n}};
  count();
  count();
  empty = moved;
  return add(1) * 100 + empty(10) + n * 1000 + (splice::erased_call<int(int)>{}(5));
}
static_assert(in_constant_evaluation() == 2000 + 300 + 12);

TEST(erased_call, calls_what_it_holds) {
  splice::erased_call<int(int)> add{adds{5}};
  EXPECT_TRUE(add.holds());
  EXPECT_EQ(add(1), 6);
}

TEST(erased_call, empty_answers_nothing) {
  const splice::erased_call<int(int)> none;
  EXPECT_FALSE(none.holds());
  EXPECT_EQ(none(7), 0);
  splice::erased_call<void()> nothing;
  nothing();
}

TEST(erased_call, copies_and_moves_with_what_it_holds) {
  int calls = 0;
  {
    splice::erased_call<void()> one{tracked{&calls}};
    EXPECT_EQ(tracked::alive, 1);
    splice::erased_call<void()> two = one;
    EXPECT_EQ(tracked::alive, 2);
    splice::erased_call<void()> three = std::move(two);
    two = three;
    one();
    two();
    three();
    std::vector<splice::erased_call<void()>> many(3, one);
    many.emplace_back(tracked{&calls});
    std::ranges::for_each(many, [](const auto& each) { each(); });
  }
  EXPECT_EQ(calls, 7);
  EXPECT_EQ(tracked::alive, 0);
}

TEST(erased_call, assigned_over_another) {
  splice::erased_call<int(int)> one{adds{1}};
  splice::erased_call<int(int)> other{[](int x) { return x * 3; }};
  one = other;
  EXPECT_EQ(one(2), 6);
  one = splice::erased_call<int(int)>{};
  EXPECT_FALSE(one.holds());
}

}  // namespace
