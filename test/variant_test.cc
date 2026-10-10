// SPDX-License-Identifier: GPL-3.0-only
// spl::variant and spl::overloaded: made, moved, copied, compared and
// visited -- at run time, and as constant expressions.
import std;
import splice;
import gtest;

#include "gtest/gtest-macros.h"

namespace {

struct text {
  std::string said;
  friend auto operator<=>(const text&, const text&) = default;
};
struct picture {
  int width = 0;
  int height = 0;
  friend auto operator<=>(const picture&, const picture&) = default;
};
struct nothing {
  friend auto operator<=>(nothing, nothing) = default;
};
using message = spl::variant<text, picture, nothing>;

// UI nodes are move-only and their moves can throw. Containers must see
// that copying their variant is unavailable and relocate it by moving.
struct move_only {
  std::unique_ptr<int> value;
  explicit move_only(int n) : value(std::make_unique<int>(n)) {}
  move_only(const move_only&) = delete;
  move_only& operator=(const move_only&) = delete;
  move_only(move_only&& other) noexcept(false) : value(std::move(other.value)) {}
  move_only& operator=(move_only&&) = delete;
};
using move_only_variant = spl::variant<int, move_only>;
static_assert(!std::is_copy_constructible_v<move_only_variant>);
static_assert(!std::is_copy_assignable_v<move_only_variant>);
static_assert(std::is_move_constructible_v<move_only_variant>);
static_assert(std::is_move_assignable_v<move_only_variant>);
static_assert(!std::is_nothrow_move_constructible_v<move_only_variant>);
static_assert(std::same_as<decltype(std::move_if_noexcept(std::declval<move_only_variant&>())), move_only_variant&&>);
struct immovable {
  immovable() = default;
  immovable(const immovable&) = delete;
  immovable(immovable&&) = delete;
};
using immovable_variant = spl::variant<int, immovable>;
static_assert(!std::is_copy_constructible_v<immovable_variant>);
static_assert(!std::is_copy_assignable_v<immovable_variant>);
static_assert(!std::is_move_constructible_v<immovable_variant>);
static_assert(!std::is_move_assignable_v<immovable_variant>);
static_assert(std::is_copy_constructible_v<message>);
static_assert(std::is_copy_assignable_v<message>);

// A tree: a node holding a variant of a vector of itself, declared while
// the node is incomplete.
struct node;
struct branch {
  std::vector<node> children;
};
struct node {
  spl::variant<branch, std::string> held;
};

// How many times each was made and destroyed: that a variant destroys what
// it made, once.
struct counted {
  static inline int alive = 0;
  counted() { ++alive; }
  counted(const counted&) { ++alive; }
  counted(counted&&) noexcept { ++alive; }
  counted& operator=(const counted&) = default;
  counted& operator=(counted&&) = default;
  ~counted() { --alive; }
};

// Fail before a new object's lifetime starts; count only completed objects.
struct throwing {
  enum class failure { none, construct, copy, move };
  static inline failure fail = failure::none;
  static inline int alive = 0;
  int value = 0;
  explicit throwing(int n = 0) : value(n) {
    if (fail == failure::construct)
      throw std::runtime_error("construct");
    ++alive;
  }
  throwing(const throwing& other) : value(other.value) {
    if (fail == failure::copy)
      throw std::runtime_error("copy");
    ++alive;
  }
  throwing(throwing&& other) : value(other.value) {
    if (fail == failure::move)
      throw std::runtime_error("move");
    ++alive;
  }
  ~throwing() { --alive; }
  friend auto operator<=>(const throwing&, const throwing&) = default;
};
using fallible = spl::variant<int, throwing>;
static_assert(!std::is_nothrow_move_constructible_v<fallible>);
static_assert(!std::is_nothrow_move_assignable_v<fallible>);
static_assert(!noexcept(std::declval<fallible&>().swap(std::declval<fallible&>())));
static_assert(std::is_nothrow_move_constructible_v<spl::variant<int, double>>);
static_assert(std::is_nothrow_move_assignable_v<spl::variant<int, double>>);
static_assert(noexcept(std::declval<message&>().swap(std::declval<message&>())));

constexpr std::string kind_of(const message& one) {
  return spl::visit(spl::overloaded{[](const text&) { return std::string("text"); },
                                          [](const picture&) { return std::string("picture"); },
                                          [](nothing) { return std::string("nothing"); }},
                       one);
}

}  // namespace

TEST(Variant, DefaultIsTheFirst) {
  const message one;
  EXPECT_EQ(one.index(), 0u);
  EXPECT_EQ(kind_of(one), "text");
}

TEST(Variant, HoldsWhatItWasMadeWith) {
  const message one = picture{3, 4};
  EXPECT_EQ(one.index(), 1u);
  ASSERT_NE(one.get_if<picture>(), nullptr);
  EXPECT_EQ(one.get_if<picture>()->width, 3);
  EXPECT_EQ(one.get_if<text>(), nullptr);
  EXPECT_EQ(spl::get<picture>(one).height, 4);
  EXPECT_EQ(spl::get<1>(one).height, 4);
  EXPECT_THROW((void)spl::get<text>(one), std::bad_variant_access);
}

TEST(Variant, HoldsATreeOfItself) {
  node root;
  spl::get<branch>(root.held).children.push_back(node{std::string("leaf")});
  spl::get<branch>(root.held).children.push_back(node{});
  node copy = root;
  EXPECT_EQ(spl::get<branch>(copy.held).children.size(), 2u);
  EXPECT_EQ(spl::get<std::string>(spl::get<branch>(copy.held).children[0].held), "leaf");
}

TEST(Variant, GetIfByIndex) {
  message one = picture{5, 6};
  ASSERT_NE(spl::get_if<1>(&one), nullptr);
  EXPECT_EQ(spl::get_if<1>(&one)->width, 5);
  EXPECT_EQ(spl::get_if<0>(&one), nullptr);
  const message& same = one;
  EXPECT_EQ(spl::get_if<1>(&same)->height, 6);
  EXPECT_TRUE(spl::holds_alternative<picture>(same));
}

TEST(Variant, MemberVisit) {
  message one = text{"hi"};
  one.visit(spl::overloaded{[](text& t) { t.said += "!"; }, [](auto&) {}});
  EXPECT_EQ(spl::get<text>(one).said, "hi!");
  EXPECT_EQ(one.visit<int>(spl::overloaded{[](const text& t) { return static_cast<int>(t.said.size()); },
                                              [](const auto&) { return -1; }}),
            3);
}

TEST(Variant, CopiedMovedAndAssigned) {
  message one = text{"a long enough string not to be kept in place"};
  message copy = one;
  EXPECT_EQ(copy, one);
  message moved = std::move(copy);
  EXPECT_EQ(moved, one);
  moved = picture{1, 2};
  EXPECT_EQ(kind_of(moved), "picture");
  moved = one;
  EXPECT_EQ(moved, one);
  moved.emplace<nothing>();
  EXPECT_EQ(kind_of(moved), "nothing");
  one.swap(moved);
  EXPECT_EQ(kind_of(one), "nothing");
  EXPECT_EQ(kind_of(moved), "text");
}

TEST(Variant, Ordered) {
  const message a = text{"a"};
  const message b = text{"b"};
  const message p = picture{};
  EXPECT_LT(a, b);
  EXPECT_LT(b, p);  // by index first
  EXPECT_NE(a, b);
}

TEST(Variant, DestroysWhatItMakes) {
  {
    spl::variant<counted, int> one;
    EXPECT_EQ(counted::alive, 1);
    spl::variant<counted, int> two = one;
    EXPECT_EQ(counted::alive, 2);
    two = 5;
    EXPECT_EQ(counted::alive, 1);
    two = std::move(one);
    EXPECT_EQ(counted::alive, 2);  // a moved-from alternative is still there
  }
  EXPECT_EQ(counted::alive, 0);
}

TEST(Variant, VisitsSeveralAtOnce) {
  const message a = text{"x"};
  const spl::variant<int, double> b = 2.5;
  const std::variant<char, bool> c = true;
  const auto said = spl::visit(
      []<class A, class B, class C>(const A&, const B&, const C&) {
        return std::string(typeid(A) == typeid(text) ? "text" : "?") + (std::same_as<B, double> ? "/double" : "/?") +
               (std::same_as<C, bool> ? "/bool" : "/?");
      },
      a, b, c);
  EXPECT_EQ(said, "text/double/bool");
}

// The same, as constant expressions.
static_assert([] {
  message one = picture{2, 3};
  message other = one;
  other = text{"made at compile time"};
  return kind_of(one) == "picture" && kind_of(other) == "text" && one != other &&
         spl::get<text>(other).said.size() == 20;
}());
static_assert(spl::variant_size_v<message> == 3);
static_assert(std::same_as<spl::variant_alternative_t<1, message>, picture>);

TEST(Variant, FailedEmplaceIsValuelessAndCanRecover) {
  {
    fallible one(std::in_place_type<throwing>, 7);
    EXPECT_EQ(throwing::alive, 1);
    throwing::fail = throwing::failure::construct;
    EXPECT_THROW(one.emplace<throwing>(9), std::runtime_error);
    throwing::fail = throwing::failure::none;
    EXPECT_EQ(throwing::alive, 0);
    EXPECT_TRUE(one.valueless_by_exception());
    EXPECT_EQ(one.index(), std::variant_npos);
    EXPECT_EQ(one.get_if<int>(), nullptr);
    EXPECT_EQ(one.get_if<throwing>(), nullptr);
    EXPECT_FALSE(spl::holds_alternative<throwing>(one));
    EXPECT_THROW((void)spl::get<int>(one), std::bad_variant_access);
    EXPECT_THROW(one.visit([](auto&) {}), std::bad_variant_access);
    const fallible& same = one;
    EXPECT_EQ(spl::get_if<0>(&same), nullptr);
    EXPECT_THROW(same.visit<int>([](const auto&) { return 0; }), std::bad_variant_access);
    fallible other = 3;
    EXPECT_THROW(spl::visit([](auto&, auto&) {}, other, one), std::bad_variant_access);
    one.emplace<throwing>(11);
    EXPECT_FALSE(one.valueless_by_exception());
    EXPECT_EQ(spl::get<throwing>(one).value, 11);
  }
  EXPECT_EQ(throwing::alive, 0);
}

TEST(Variant, ThrowingCopyLeavesSafeDestination) {
  {
    fallible source(std::in_place_type<throwing>, 7);
    fallible destination(std::in_place_type<throwing>, 8);
    throwing::fail = throwing::failure::copy;
    EXPECT_THROW(destination = source, std::runtime_error);
    EXPECT_THROW((void)fallible(source), std::runtime_error);
    EXPECT_THROW(destination = spl::get<throwing>(source), std::runtime_error);
    throwing::fail = throwing::failure::none;
    EXPECT_TRUE(destination.valueless_by_exception());
    EXPECT_EQ(throwing::alive, 1);
    EXPECT_EQ(spl::get<throwing>(source).value, 7);
    destination = source;
    EXPECT_EQ(throwing::alive, 2);
    EXPECT_EQ(destination, source);
  }
  EXPECT_EQ(throwing::alive, 0);
}

TEST(Variant, ThrowingMovePropagatesAndLeavesSafeDestination) {
  {
    fallible source(std::in_place_type<throwing>, 7);
    fallible destination(std::in_place_type<throwing>, 8);
    throwing::fail = throwing::failure::move;
    EXPECT_THROW(destination = std::move(source), std::runtime_error);
    EXPECT_THROW((void)fallible(std::move(source)), std::runtime_error);
    EXPECT_THROW(destination.swap(source), std::runtime_error);
    throwing::fail = throwing::failure::none;
    EXPECT_TRUE(destination.valueless_by_exception());
    EXPECT_EQ(throwing::alive, 1);
    destination = std::move(source);
    EXPECT_EQ(throwing::alive, 2);
  }
  EXPECT_EQ(throwing::alive, 0);
}

TEST(Variant, CopiesMovesComparesAndSwapsValuelessObjects) {
  fallible empty = 1;
  throwing::fail = throwing::failure::construct;
  EXPECT_THROW(empty.emplace<throwing>(), std::runtime_error);
  throwing::fail = throwing::failure::none;
  fallible copy = empty;
  fallible moved = std::move(empty);
  EXPECT_TRUE(empty.valueless_by_exception());
  EXPECT_EQ(copy, moved);
  EXPECT_EQ(copy <=> moved, std::strong_ordering::equal);
  fallible value = 2;
  EXPECT_NE(empty, value);
  EXPECT_LT(empty, value);
  EXPECT_GT(value, empty);
  value = empty;
  EXPECT_TRUE(value.valueless_by_exception());
  value = 3;
  value = std::move(empty);
  EXPECT_TRUE(value.valueless_by_exception());
  value = 4;
  empty.swap(value);
  EXPECT_EQ(spl::get<int>(empty), 4);
  EXPECT_TRUE(value.valueless_by_exception());
  value.swap(copy);
  EXPECT_TRUE(value.valueless_by_exception());
  EXPECT_TRUE(copy.valueless_by_exception());
  copy = copy;
  copy = std::move(copy);
  copy.swap(copy);
  EXPECT_TRUE(copy.valueless_by_exception());
  empty = empty;
  empty = std::move(empty);
  empty.swap(empty);
  EXPECT_EQ(spl::get<int>(empty), 4);
}

TEST(Variant, VectorReallocationMovesNoncopyableAlternativesEvenWhenMoveCanThrow) {
  std::vector<move_only_variant> rows;
  rows.reserve(1);
  rows.emplace_back(std::in_place_type<move_only>, 42);
  rows.reserve(8);
  EXPECT_EQ(*spl::get<move_only>(rows[0]).value, 42);
  rows.emplace_back(7);
  move_only_variant destination = 0;
  destination = std::move(rows[0]);
  EXPECT_EQ(*spl::get<move_only>(destination).value, 42);
  EXPECT_EQ(spl::get<int>(rows[1]), 7);
}
