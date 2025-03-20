#pragma once
/*
MIT License

Copyright (c) 2025 Ruben Cano Diaz

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <ranges>
#include <tuple>
#include <type_traits>
#include <utility>
#include <variant>

namespace cppmatch {

// Exposed Type: Result
// A type alias for std::variant<T, E>, representing a success value T or an
// error E.
template <typename T, typename E> using Result = std::variant<T, E>;

template <typename T, typename... Ts>
struct is_one_of : std::disjunction<std::is_same<T, Ts>...> {};

// Exposed Type: Error
// A variadic type alias for std::variant, allowing multiple error types as
// requested.
template <typename... Ts> class Error {
public:
  using VariantType = std::variant<Ts...>;
  VariantType value;

  template <typename T, typename = std::enable_if_t<
                            is_one_of<std::decay_t<T>, Ts...>::value>>
  Error(T &&t) : value(std::forward<T>(t)) {}

  template <typename... Us,
            typename = std::enable_if_t<(is_one_of<Us, Ts...>::value && ...)>>
  Error(const Error<Us...> &other) {
    std::visit(
        [this](auto &&arg) {
          value = std::variant<Ts...>(std::forward<decltype(arg)>(arg));
        },
        other.value);
  }

  template <typename... Us,
            typename = std::enable_if_t<(is_one_of<Us, Ts...>::value && ...)>>
  Error(Error<Us...> &&other) {
    std::visit(
        [this](auto &&arg) {
          value = std::variant<Ts...>(std::forward<decltype(arg)>(arg));
        },
        std::move(other.value));
  }
};

namespace cppmatch_detail {

template <class... Ts> struct overloaded : Ts... {
  using Ts::operator()...;
};

// Trait to check if a type is a std::variant.
template <typename T> struct is_variant : std::false_type {};
template <typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

template <typename T>
constexpr bool is_variant_v = is_variant<std::decay_t<T>>::value;

// Trait to detect our custom Error type.
template <typename T> struct is_error : std::false_type {};
template <typename... Ts> struct is_error<Error<Ts...>> : std::true_type {};

template <typename T>
constexpr bool is_error_v = is_error<std::decay_t<T>>::value;

// Recursively visit a nested variant until a non-variant (or non-error) value
// is found.
template <typename T, typename Visitor>
constexpr auto flat_visit(T &&value, Visitor &&vis) {
  if constexpr (is_error_v<std::decay_t<T>>) {
    return flat_visit(std::forward<T>(value).value, std::forward<Visitor>(vis));
  } else if constexpr (is_variant_v<std::decay_t<T>>) {
    return std::visit(
        [&](auto &&inner) {
          return flat_visit(std::forward<decltype(inner)>(inner),
                            std::forward<Visitor>(vis));
        },
        std::forward<T>(value));
  } else {
    return std::forward<Visitor>(vis)(std::forward<T>(value));
  }
}

} // namespace cppmatch_detail

template <typename T, typename E>
constexpr bool is_err(const Result<T, E> &result) noexcept {
  if constexpr (std::is_constant_evaluated()) {
    return std::holds_alternative<E>(result);
  }
  return result.index(); // at runtime is faster to check the index
}

template <typename T, typename E>
constexpr bool is_ok(const Result<T, E> &result) noexcept {
  return !is_err(result);
}

template <typename Variant, typename... Lambdas>
constexpr auto match(Variant &&v, Lambdas &&...lambdas) {
  return cppmatch_detail::flat_visit(
      std::forward<Variant>(v),
      cppmatch_detail::overloaded{std::forward<Lambdas>(lambdas)...});
}

#if (defined(__GNUC__) || (defined(__clang__)))
#define expect(expr)                                                           \
  __extension__({                                                              \
    auto &&expr_ = (expr);                                                     \
    if (cppmatch::is_err(expr_))                                               \
      return std::get<1>(std::move(expr_)); /* Handle error case */            \
    std::get<0>(std::move(expr_));                                             \
  })
#elif defined(_MSC_VER)
// MSVC workaround: Force a compile-time error *only when used*
#define expect(expr)                                                           \
  static_assert([] { return false; }(),                                        \
                "MSVC does not support 'statement expressions ({}), you can "  \
                "still use the expect_e / match_e which use exceptions.")
#else
#define expect(expr)                                                           \
  static_assert(                                                               \
      [] { return false; }(),                                                  \
      "Unknown compiler: does not support 'statement expressions ({}), you "   \
      "can still use the expect_e / match_e which use exceptions.")
#endif

// Exposed Function: default_expect
// Returns the success value of a Result or a provided default if an error
// occurred.
template <typename T, typename E>
constexpr T default_expect(const Result<T, E> &result, T &&default_value) {
  return std::holds_alternative<T>(result) ? std::get<T>(result)
                                           : default_value;
}

// Exposed Function: map_error
// Transforms the error alternative using the provided function, constructing
// the Result explicitly.
template <typename T, typename E1, typename F>
constexpr auto map_error(const Result<T, E1> &result, F &&f) {
  using E2 = std::invoke_result_t<F, const E1 &>;
  return match(
      result,
      [](const T &val) {
        return Result<T, E2>(std::in_place_index_t<0>{}, val);
      },
      [&](const E1 &err) {
        return Result<T, E2>(std::in_place_index_t<1>{}, f(err));
      });
}

} // namespace cppmatch

#if defined(_CPPUNWIND) || defined(__EXCEPTIONS) || defined(__cpp_exceptions)
namespace cppmatch {
template <typename T> struct FlattenErrorVariant {
  using type = T;
};

// Specialization for std::variant
template <typename T, typename... Ts>
struct FlattenErrorVariant<std::variant<T, Ts...>> {
private:
  // If any type is `Error<Us...>`, extract the types inside it
  template <typename... Us>
  static auto helper(std::variant<T, Ts...> *, Error<Us...> *)
      -> std::variant<T, Ts..., Us...>;

  // Fallback for when `Error<TYPES...>` is not in the variant
  static auto helper(std::variant<T, Ts...> *, ...) -> std::variant<T, Ts...>;

public:
  using type = decltype(helper(static_cast<std::variant<T, Ts...> *>(nullptr),
                               static_cast<Ts *>(nullptr)...));
};

template <typename T>
using FlattenErrorVariant_t = typename FlattenErrorVariant<T>::type;

template <typename Variant> constexpr auto expect_e(Variant &&v) {
  if (cppmatch::is_err(v)) {
    match(std::get<1>(v), [](auto &&err) -> void { throw err; });
  }
  return std::get<0>(v);
}

template <typename ErrorType, size_t I, typename... Lambdas>
constexpr auto handle_exception_index(std::exception_ptr eptr,
                                      Lambdas &&...lambdas) {
  using CurrentType = std::variant_alternative_t<I, std::decay_t<ErrorType>>;
  try {
    std::rethrow_exception(eptr);
  } catch (const CurrentType &err) {
    return cppmatch_detail::flat_visit(
        err, cppmatch_detail::overloaded{std::forward<Lambdas>(lambdas)...});
  } catch (...) {
  }

  if constexpr (I + 1 < std::variant_size_v<std::decay_t<ErrorType>>) {
    return handle_exception_index<ErrorType, I + 1>(
        eptr, std::forward<Lambdas>(lambdas)...);
  } else {
    throw; // Rethrow if nothing matches
  }
}

template <typename Expr, typename... Lambdas>
constexpr auto dynamic_match(Expr &&expr, Lambdas &&...lambdas) {
  using ResultType = std::invoke_result_t<Expr>;
  using SuccessType = std::variant_alternative_t<0, ResultType>;
  using ErrorType = std::variant_alternative_t<1, ResultType>;

  try {
    return match(std::forward<Expr>(expr)(), std::forward<Lambdas>(lambdas)...);
  } catch (...) {
    std::exception_ptr eptr = std::current_exception();
    return handle_exception_index<FlattenErrorVariant_t<ResultType>, 0>(
        eptr, std::forward<Lambdas>(lambdas)...);
  }
}

#define match_e(EXPR, ...)                                                     \
  ([&] {                                                                       \
    static_assert(!std::is_lvalue_reference_v<decltype((EXPR))>,               \
                  "EXPR must be a function call, not an lvalue!");             \
    return dynamic_match([&] { return (EXPR); },                               \
                         cppmatch_detail::overloaded{__VA_ARGS__});            \
  }())
} // namespace cppmatch
#endif

namespace cppmatch {
namespace cppmatch_ranges {
struct successes_fn {
  template <std::ranges::range R>
  friend auto operator|(R &&range, const successes_fn &self) {
    using VariantType = std::ranges::range_value_t<R>;
    static_assert(std::variant_size_v<VariantType> == 2,
                  "Variants must have exactly two alternatives");

    return std::forward<R>(range) |
           std::views::filter([](const auto &res) { return is_ok(res); }) |
           std::views::transform(
               [](const auto &res) { return std::get<0>(res); });
  }

  template <std::ranges::range R> auto operator()(R &&range) const {
    return range | *this;
  }
};

} // namespace cppmatch_ranges

inline constexpr cppmatch_ranges::successes_fn successes{};
} // namespace cppmatch