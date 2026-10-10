// spl::erased_call: called as what it holds, copied and moved with it,
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

struct throwing_call {
  static inline bool fail_copy = false;
  static inline bool fail_move = false;
  static inline int alive = 0;
  throwing_call() { ++alive; }
  throwing_call(const throwing_call&) {
    if (fail_copy)
      throw std::runtime_error("copy");
    ++alive;
  }
  throwing_call(throwing_call&&) {
    if (fail_move)
      throw std::runtime_error("move");
    ++alive;
  }
  ~throwing_call() { --alive; }
  int operator()(int n) const { return n + 1; }
};
static_assert(!std::is_nothrow_move_constructible_v<spl::erased_call<int(int)>>);
static_assert(!std::is_nothrow_move_assignable_v<spl::erased_call<int(int)>>);

// In constant evaluation: made, called, copied, moved, assigned, emptied.
constexpr int in_constant_evaluation() {
  spl::erased_call<int(int)> add{adds{2}};
  spl::erased_call<int(int)> copy = add;
  spl::erased_call<int(int)> moved = std::move(copy);
  spl::erased_call<int(int)> empty;
  int n = 0;
  spl::erased_call<void()> count{counts{&n}};
  count();
  count();
  empty = moved;
  return add(1) * 100 + empty(10) + n * 1000 + (spl::erased_call<int(int)>{}(5));
}
static_assert(in_constant_evaluation() == 2000 + 300 + 12);

TEST(erased_call, calls_what_it_holds) {
  spl::erased_call<int(int)> add{adds{5}};
  EXPECT_TRUE(add.holds());
  EXPECT_EQ(add(1), 6);
}

TEST(erased_call, empty_answers_nothing) {
  const spl::erased_call<int(int)> none;
  EXPECT_FALSE(none.holds());
  EXPECT_EQ(none(7), 0);
  spl::erased_call<void()> nothing;
  nothing();
}

TEST(erased_call, copies_and_moves_with_what_it_holds) {
  int calls = 0;
  {
    spl::erased_call<void()> one{tracked{&calls}};
    EXPECT_EQ(tracked::alive, 1);
    spl::erased_call<void()> two = one;
    EXPECT_EQ(tracked::alive, 2);
    spl::erased_call<void()> three = std::move(two);
    two = three;
    one();
    two();
    three();
    std::vector<spl::erased_call<void()>> many(3, one);
    many.emplace_back(tracked{&calls});
    std::ranges::for_each(many, [](const auto& each) { each(); });
  }
  EXPECT_EQ(calls, 7);
  EXPECT_EQ(tracked::alive, 0);
}

TEST(erased_call, assigned_over_another) {
  spl::erased_call<int(int)> one{adds{1}};
  spl::erased_call<int(int)> other{[](int x) { return x * 3; }};
  one = other;
  EXPECT_EQ(one(2), 6);
  one = spl::erased_call<int(int)>{};
  EXPECT_FALSE(one.holds());
}

TEST(erased_call, failed_copy_leaves_an_empty_reusable_call) {
  {
    spl::erased_call<int(int)> source{throwing_call{}};
    spl::erased_call<int(int)> destination{throwing_call{}};
    throwing_call::fail_copy = true;
    EXPECT_THROW(destination = source, std::runtime_error);
    EXPECT_THROW((void)spl::erased_call<int(int)>(source), std::runtime_error);
    throwing_call::fail_copy = false;
    EXPECT_EQ(throwing_call::alive, 1);
    EXPECT_FALSE(destination.holds());
    EXPECT_EQ(destination(3), 0);
    EXPECT_EQ(source(3), 4);
    destination = source;
    EXPECT_TRUE(destination.holds());
    EXPECT_EQ(destination(3), 4);
  }
  EXPECT_EQ(throwing_call::alive, 0);
}

TEST(erased_call, failed_move_propagates_and_leaves_an_empty_call) {
  {
    spl::erased_call<int(int)> source{throwing_call{}};
    spl::erased_call<int(int)> destination{throwing_call{}};
    throwing_call::fail_move = true;
    EXPECT_THROW(destination = std::move(source), std::runtime_error);
    EXPECT_THROW((void)spl::erased_call<int(int)>(std::move(source)), std::runtime_error);
    throwing_call::fail_move = false;
    EXPECT_EQ(throwing_call::alive, 1);
    EXPECT_FALSE(destination.holds());
    EXPECT_EQ(destination(3), 0);
    destination = std::move(source);
    EXPECT_TRUE(destination.holds());
    EXPECT_EQ(destination(3), 4);
  }
  EXPECT_EQ(throwing_call::alive, 0);
}

}  // namespace
