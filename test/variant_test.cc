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
