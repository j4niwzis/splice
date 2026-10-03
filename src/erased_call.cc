// SPDX-License-Identifier: GPL-3.0-only
// splice.erased_call -- A call, whatever it is: std::function's use, held
// as splice::variant holds its alternative (splice.held) -- in a buffer of
// its own at run time, never on the heap; allocated in constant evaluation,
// where it is a constant expression too.
//
// What it holds is said by a table of what can be done with it -- called,
// copied, moved, destroyed -- one per type, made where it is erased. A call
// too large for the buffer is not taken: Capacity says how large one may be,
// and a larger one is an error where it is made, not a heap allocation.
// Holding nothing, it answers as an empty Result -- nothing, for a void one.
export module splice.erased_call;

import std;
import splice.held;

export namespace splice {

template <class Signature, std::size_t Capacity = 3 * sizeof(void*)>
class erased_call;

template <class Result, class... Args, std::size_t Capacity>
class erased_call<Result(Args...), Capacity> {
  // What can be done with what it holds.
  struct ops {
    Result (*call)(const held*, Args&&...);
    held* (*copy)(unsigned char*, const held*);
    held* (*move)(unsigned char*, held*);
    void (*destroy)(held*);
  };
  // Holding nothing: called, an empty Result; nothing to copy, move or
  // destroy.
  static constexpr ops kNothing{
      +[](const held*, Args&&...) -> Result { return Result(); },
      +[](unsigned char*, const held*) -> held* { return nullptr; },
      +[](unsigned char*, held*) -> held* { return nullptr; },
      +[](held*) {},
  };
  template <class Call>
  static constexpr ops kOps{
      +[](const held* object, Args&&... args) -> Result {
        return std::invoke(value_of<Call>(object), std::forward<Args>(args)...);
      },
      +[](unsigned char* buffer, const held* from) -> held* { return make_held<Call>(buffer, value_of<Call>(from)); },
      +[](unsigned char* buffer, held* from) -> held* { return make_held<Call>(buffer, std::move(value_of<Call>(from))); },
      &destroy_held<Call>,
  };

public:
  constexpr erased_call() noexcept {}
  template <class Call>
    requires(!std::same_as<std::remove_cvref_t<Call>, erased_call> && std::copy_constructible<Call> &&
             std::is_invocable_r_v<Result, const Call&, Args...>)
  constexpr explicit erased_call(Call call) : fOps(&kOps<Call>) {
    static_assert(sizeof(holder<Call>) <= Capacity, "the call is larger than the erased_call's Capacity");
    static_assert(alignof(holder<Call>) <= alignof(std::max_align_t), "the call is aligned beyond the buffer");
    fObject = make_held<Call>(fBuffer, std::move(call));
  }
  constexpr erased_call(const erased_call& other) : fOps(other.fOps), fObject(other.fOps->copy(fBuffer, other.fObject)) {}
  constexpr erased_call(erased_call&& other) noexcept
      : fOps(other.fOps), fObject(other.fOps->move(fBuffer, other.fObject)) {}
  constexpr erased_call& operator=(const erased_call& other) {
    if (this != &other) {
      fOps->destroy(fObject);
      fOps = other.fOps;
      fObject = fOps->copy(fBuffer, other.fObject);
    }
    return *this;
  }
  constexpr erased_call& operator=(erased_call&& other) noexcept {
    if (this != &other) {
      fOps->destroy(fObject);
      fOps = other.fOps;
      fObject = fOps->move(fBuffer, other.fObject);
    }
    return *this;
  }
  constexpr ~erased_call() { fOps->destroy(fObject); }

  constexpr Result operator()(Args... args) const { return fOps->call(fObject, std::forward<Args>(args)...); }
  // Whether it holds a call.
  [[nodiscard]] constexpr bool holds() const noexcept { return fObject != nullptr; }

private:
  const ops* fOps = &kNothing;
  held* fObject = nullptr;
  alignas(std::max_align_t) unsigned char fBuffer[Capacity];
};

}  // namespace splice
