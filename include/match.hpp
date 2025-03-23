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

/**
 * @brief A Result type alias representing either a success value or an error.
 *
 * @tparam T Type for the success value.
 * @tparam E Type for the error.
 */
template <typename T, typename E>
using Result = std::variant<T, E>;

/**
 * @brief Type trait to check if type T is the same as one of the types in Ts.
 *
 * @tparam T The type to check.
 * @tparam Ts The candidate types.
 */
template <typename T, typename... Ts>
struct is_one_of : std::disjunction<std::is_same<T, Ts>...> {};

/**
 * @brief A variadic error type encapsulating one of several possible error types.
 *
 * The Error class is a thin wrapper around std::variant to represent multiple
 * error types. It supports construction from any error type included in its list.
 *
 * @tparam Ts List of possible error types.
 */
template <typename... Ts>
class Error {
public:
  /// The underlying variant type holding the error value.
  using VariantType = std::variant<Ts...>;
  VariantType value;

  /**
   * @brief Constructs an Error from an error value.
   *
   * This constructor is enabled only if the provided type is one of the error types.
   *
   * @tparam T The error type.
   * @param t The error value.
   */
  template <typename T, typename = std::enable_if_t<
                            is_one_of<std::decay_t<T>, Ts...>::value>>
  Error(T &&t) : value(std::forward<T>(t)) {}

  /**
   * @brief Copy constructor for converting between different Error types.
   *
   * This constructor is enabled if all types in the other Error are among the allowed types.
   *
   * @tparam... Us The error types from the other Error.
   * @param other The other Error instance.
   */
  template <typename... Us,
            typename = std::enable_if_t<(is_one_of<Us, Ts...>::value && ...)>>
  Error(const Error<Us...> &other) {
    std::visit(
        [this](auto &&arg) {
          value = std::variant<Ts...>(std::forward<decltype(arg)>(arg));
        },
        other.value);
  }

  /**
   * @brief Move constructor for converting between different Error types.
   *
   * This constructor is enabled if all types in the other Error are among the allowed types.
   *
   * @tparam... Us The error types from the other Error.
   * @param other The other Error instance (rvalue reference).
   */
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

/**
 * @brief Helper struct to allow overloaded lambda expressions.
 *
 * This struct inherits from multiple lambdas to allow a single call to
 * std::visit to use overloaded call operators.
 *
 * @tparam Ts Lambda types.
 */
template <class... Ts>
struct overloaded : Ts... {
  using Ts::operator()...;
};

/**
 * @brief Trait to check if a type is a std::variant.
 *
 * @tparam T The type to check.
 */
template <typename T> struct is_variant : std::false_type {};
template <typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

/**
 * @brief Variable template for is_variant.
 *
 * @tparam T The type to check.
 */
template <typename T>
constexpr bool is_variant_v = is_variant<std::decay_t<T>>::value;

/**
 * @brief Trait to detect our custom Error type.
 *
 * @tparam T The type to check.
 */
template <typename T> struct is_error : std::false_type {};
template <typename... Ts> struct is_error<Error<Ts...>> : std::true_type {};

/**
 * @brief Variable template for is_error.
 *
 * @tparam T The type to check.
 */
template <typename T>
constexpr bool is_error_v = is_error<std::decay_t<T>>::value;

/**
 * @brief Recursively visits a nested variant until a non-variant (or non-error) value is found.
 *
 * @tparam T The type of the value.
 * @tparam Visitor The visitor type.
 * @param value The value to be visited.
 * @param vis The visitor callable.
 * @return The result of applying the visitor to the innermost value.
 */
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

/**
 * @brief Checks if the Result holds an error.
 *
 * This function uses constant evaluation if possible for faster runtime checks.
 *
 * @tparam T The success type.
 * @tparam E The error type.
 * @param result The Result to check.
 * @return true if the result holds an error; false otherwise.
 */
template <typename T, typename E>
constexpr bool is_err(const Result<T, E> &result) noexcept {
  #if defined(__cpp_if_consteval)
  if consteval  {
    return std::holds_alternative<E>(result);
  }
  #else 
  if constexpr (std::is_constant_evaluated()) {
    return std::holds_alternative<E>(result);
  }
  #endif
  return result.index(); // At runtime, checking the index is faster.
}

/**
 * @brief Checks if the Result holds a success value.
 *
 * @tparam T The success type.
 * @tparam E The error type.
 * @param result The Result to check.
 * @return true if the result holds a success value; false otherwise.
 */
template <typename T, typename E>
constexpr bool is_ok(const Result<T, E> &result) noexcept {
  return !is_err(result);
}

/**
 * @brief Matches over the alternatives in a Result or nested variant.
 *
 * This function flattens nested Error or variant types and applies the provided
 * overloaded lambdas to the innermost value.
 *
 * @tparam Variant The type of the variant.
 * @tparam Lambdas The lambda functions to handle each alternative.
 * @param v The variant value.
 * @param lambdas The lambda handlers.
 * @return The result of the matching.
 */
template <typename Variant, typename... Lambdas>
constexpr auto match(Variant &&v, Lambdas &&...lambdas) {
  return cppmatch_detail::flat_visit(
      std::forward<Variant>(v),
      cppmatch_detail::overloaded{std::forward<Lambdas>(lambdas)...});
}

/**
 * @brief Macro to unwrap a Result or return an error.
 *
 * This macro evaluates an expression that returns a Result. If the result is an error,
 * it returns the error immediately; otherwise, it extracts and returns the success value.
 *
 * @param expr An expression that returns a Result.
 */
#if (defined(__GNUC__) || (defined(__clang__)))
#define expect(expr)                                                           \
  __extension__({                                                              \
    auto &&expr_ = (expr);                                                     \
    if (cppmatch::is_err(expr_))                                               \
      return std::get<1>(std::move(expr_)); /* Handle error case */            \
    std::get<0>(std::move(expr_));                                             \
  })
#elif defined(_MSC_VER)
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

/**
 * @brief Returns the success value from a Result or a provided default.
 *
 * @tparam T The success type.
 * @tparam E The error type.
 * @param result The Result to extract from.
 * @param default_value The default value to return if the Result holds an error.
 * @return The success value or the default value.
 */
template <typename T, typename E>
constexpr T default_expect(const Result<T, E> &result, T &&default_value) {
  return std::holds_alternative<T>(result) ? std::get<T>(result)
                                           : default_value;
}

/**
 * @brief Transforms the error value in a Result.
 *
 * This function applies a transformation function to the error value of a Result,
 * returning a new Result with the transformed error type.
 *
 * @tparam T The success type.
 * @tparam E1 The original error type.
 * @tparam F The function type for transforming the error.
 * @param result The original Result.
 * @param f The transformation function.
 * @return A new Result with the error transformed.
 */
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

/**
 * @brief Trait to flatten nested Error variants.
 *
 * This trait is used to extract and flatten error types encapsulated within nested
 * std::variant or Error structures.
 *
 * @tparam T The type to flatten.
 */
template <typename T> struct FlattenErrorVariant {
  using type = T;
};

/// Specialization for std::variant types.
template <typename T, typename... Ts>
struct FlattenErrorVariant<std::variant<T, Ts...>> {
private:
  template <typename... Us>
  static auto helper(std::variant<T, Ts...> *, Error<Us...> *)
      -> std::variant<T, Ts..., Us...>;

  static auto helper(std::variant<T, Ts...> *, ...) -> std::variant<T, Ts...>;

public:
  using type = decltype(helper(static_cast<std::variant<T, Ts...> *>(nullptr),
                               static_cast<Ts *>(nullptr)...));
};

/**
 * @brief Helper alias to flatten error variants.
 *
 * @tparam T The type to flatten.
 */
template <typename T>
using FlattenErrorVariant_t = typename FlattenErrorVariant<T>::type;

/**
 * @brief Unwraps a Result or throws an exception if an error is present.
 *
 * This function is used in exception-enabled environments to extract the success value
 * from a Result. If the Result contains an error, the error is thrown.
 *
 * @tparam T The success type.
 * @param v The Result to unwrap.
 * @return The success value.
 * @throws The error contained in the Result.
 */
template <typename Variant> constexpr auto expect_e(Variant &&v) {
  if (cppmatch::is_err(v)) {
    match(std::get<1>(v), [](auto &&err) -> void { throw err; });
  }
  return std::get<0>(v);
}

/**
 * @brief Helper function to handle exceptions by matching them to an error type.
 *
 * This function attempts to rethrow and catch an exception pointer as one of the expected error types.
 *
 * @tparam ErrorType The variant type of error.
 * @tparam I The current index within the variant.
 * @tparam Lambdas The lambda functions to handle each error alternative.
 * @param eptr The exception pointer.
 * @param lambdas The lambda handlers.
 * @return The result of matching the caught exception.
 * @throws If no match is found.
 */
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
    throw; // Rethrow if no handler matches.
  }
}

/**
 * @brief Dynamically matches a Result while catching exceptions.
 *
 * This function calls the provided expression that returns a Result. If an exception is thrown,
 * it will attempt to handle the exception by matching it against the expected error types.
 *
 * @tparam Expr The expression type that returns a Result.
 * @tparam Lambdas The lambda functions to handle each alternative.
 * @param expr A callable that returns a Result.
 * @return The result of matching.
 */
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

/**
 * @brief Macro for matching a Result with exception handling.
 *
 * This macro wraps a function call that returns a Result and handles exceptions by matching
 * them to the appropriate error type using dynamic_match.
 *
 * @param EXPR The expression to evaluate.
 * @param ... Lambda functions to handle the alternatives.
 */
#define match_e(EXPR, ...)                                                     \
  ([&] {                                                                       \
    static_assert(!std::is_lvalue_reference_v<decltype((EXPR))>,               \
                  "EXPR must be a function call, not an lvalue!");             \
    return dynamic_match([&] { return (EXPR); },                               \
                         cppmatch::cppmatch_detail::overloaded{__VA_ARGS__});            \
  }())
} // namespace cppmatch
#endif

namespace cppmatch {
namespace cppmatch_ranges {

/**
 * @brief Function object to extract success values from a range of Results.
 *
 * This functor filters a range of Result variants to only those that are successful,
 * then transforms the range to extract the underlying success values.
 */
struct successes_fn {
  /**
   * @brief Overloads the pipe operator to apply the success filter and transformation.
   *
   * @tparam R A range type where each element is a Result with exactly two alternatives.
   * @param range The input range.
   * @return A transformed range containing only the success values.
   */
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

  /**
   * @brief Function call operator to apply the successes view.
   *
   * @tparam R A range type.
   * @param range The input range.
   * @return A range containing only the success values.
   */
  template <std::ranges::range R>
  auto operator()(R &&range) const {
    return range | *this;
  }
};

} // namespace cppmatch_ranges

/// An inline constant instance of successes_fn for easy use with ranges.
inline constexpr cppmatch_ranges::successes_fn successes{};
} // namespace cppmatch
