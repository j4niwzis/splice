// SPDX-License-Identifier: GPL-3.0-only
// splice.overloaded -- A visitor made of several callables: one for each
// alternative, or a generic one for the rest.
//
//   spl::visit(spl::overloaded{[](const text&) { ... },
//                                    [](const picture&) { ... }},
//                 message);
export module splice.overloaded;

export namespace spl {

template <class... Fs>
struct overloaded : Fs... {
  using Fs::operator()...;
};
template <class... Fs>
overloaded(Fs...) -> overloaded<Fs...>;

}  // namespace spl
