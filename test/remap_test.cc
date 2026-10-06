// SPDX-License-Identifier: GPL-3.0-only
// spl::remapped: each field of the aggregate made taken from the field of
// the other of its type -- the same whichever way the fields are walked.
import std;
import splice;
import gtest;

#include "gtest/gtest-macros.h"

namespace {

struct palette {
  int text = 7;
};
struct speaker {
  int playing = 3;
};
struct actions {};

struct window_needs {
  actions* ask = nullptr;
  const palette* colours = nullptr;
  speaker* sound = nullptr;
};
// In another order, and fewer.
struct row_needs {
  speaker* sound = nullptr;
  const palette* colours = nullptr;
};
// Values, not only pointers.
struct settings {
  int size = 0;
  std::string name;
};
struct named {
  std::string name;
};

}  // namespace

TEST(Remap, TakesEachFieldByItsType) {
  actions ask;
  const palette colours;
  speaker sound;
  const window_needs mine{&ask, &colours, &sound};
  const row_needs theirs = spl::remapped<row_needs>(mine);
  EXPECT_EQ(theirs.colours, &colours);
  EXPECT_EQ(theirs.sound, &sound);
}

TEST(Remap, CopiesValues) {
  const settings given{.size = 12, .name = "mux"};
  EXPECT_EQ(spl::remapped<named>(given).name, "mux");
}

TEST(Remap, IsConstant) {
  static constexpr palette colours;
  constexpr window_needs mine{nullptr, &colours, nullptr};
  constexpr row_needs theirs = spl::remapped<row_needs>(mine);
  static_assert(theirs.colours == &colours);
  SUCCEED();
}

TEST(Fields, WalksAnAggregate) {
  settings one{.size = 4, .name = "a"};
  auto fields = spl::fields_of(one);
  std::get<0>(fields) = 5;
  EXPECT_EQ(one.size, 5);
  EXPECT_EQ(std::tuple_size_v<decltype(fields)>, 2u);
}
