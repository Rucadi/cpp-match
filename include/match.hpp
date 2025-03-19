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

#include <variant>
#include <utility>
#include <tuple>
#include <type_traits>
#include <ranges>
namespace cppmatch {

    // Exposed Type: Result
    // A type alias for std::variant<T, E>, representing a success value T or an error E.
    template<typename T, typename E>
    using Result = std::variant<T, E>;

    template<typename T, typename... Ts>
    struct is_one_of : std::disjunction<std::is_same<T, Ts>...> {};

    // Exposed Type: Error
    // A variadic type alias for std::variant, allowing multiple error types as requested.
    template<typename... Ts>
    class Error {
    public:
        std::variant<Ts...> value;

        template<typename T, typename = std::enable_if_t<is_one_of<std::decay_t<T>, Ts...>::value>>
        Error(T&& t) : value(std::forward<T>(t)) {}

        template<typename... Us, typename = std::enable_if_t<(is_one_of<Us, Ts...>::value && ...)>>
        Error(const Error<Us...>& other) {
            std::visit([this](auto&& arg) {
                value = std::variant<Ts...>(std::forward<decltype(arg)>(arg));
            }, other.value);
        }

        template<typename... Us, typename = std::enable_if_t<(is_one_of<Us, Ts...>::value && ...)>>
        Error(Error<Us...>&& other) {
            std::visit([this](auto&& arg) {
                value = std::variant<Ts...>(std::forward<decltype(arg)>(arg));
            }, std::move(other.value));
        }
    };

    namespace cppmatch_detail {

        template<class... Ts> 
        struct overloaded : Ts... { using Ts::operator()...; };

        // Trait to check if a type is a std::variant.
        template<typename T>
        struct is_variant : std::false_type {};
        template<typename... Ts>
        struct is_variant<std::variant<Ts...>> : std::true_type {};
    
        template<typename T>
        constexpr bool is_variant_v = is_variant<std::decay_t<T>>::value;
    
        // Trait to detect our custom Error type.
        template<typename T>
        struct is_error : std::false_type {};
        template<typename... Ts>
        struct is_error<Error<Ts...>> : std::true_type {};
    
        template<typename T>
        constexpr bool is_error_v = is_error<std::decay_t<T>>::value;

        // Recursively visit a nested variant until a non-variant (or non-error) value is found.
        template<typename T, typename Visitor>
        constexpr auto flat_visit(T&& value, Visitor&& vis) {
            if constexpr (is_error_v<std::decay_t<T>>) {
                return flat_visit(std::forward<T>(value).value, std::forward<Visitor>(vis));
            } else if constexpr (is_variant_v<std::decay_t<T>>) {
                return std::visit(
                    [&](auto&& inner) {
                        return flat_visit(std::forward<decltype(inner)>(inner), std::forward<Visitor>(vis));
                    },
                    std::forward<T>(value)
                );
            } else {
                return std::forward<Visitor>(vis)(std::forward<T>(value));
            }
        }
    
    } // namespace cppmatch_detail
    // Applies pattern matching on a variant (or our Error type) using provided lambdas.
    template<typename Variant, typename... Lambdas>
    constexpr auto match(Variant&& v, Lambdas&&... lambdas) {
        return cppmatch_detail::flat_visit(
            std::forward<Variant>(v),
            cppmatch_detail::overloaded{ std::forward<Lambdas>(lambdas)... }
        );
    }
  
  

    // Exposed Function: default_expect
    // Returns the success value of a Result or a provided default if an error occurred.
    template <typename T, typename E>
    constexpr T default_expect(const Result<T, E>& result, T&& default_value) {
        return std::holds_alternative<T>(result) ? std::get<T>(result) : default_value;
    }

    // Exposed Function: map_error
    // Transforms the error alternative using the provided function, constructing the Result explicitly.
    template <typename T, typename E1, typename F>
    constexpr auto map_error(const Result<T, E1>& result, F&& f) {
        using E2 = std::invoke_result_t<F, const E1&>;
        return match(result,
            [](const T& val) static { return Result<T, E2>(std::in_place_index_t<0>{}, val); },
            [&](const E1& err) { return Result<T, E2>(std::in_place_index_t<1>{}, f(err)); }
        );
    }

    template<typename T, typename E>
    constexpr bool is_err(const Result<T, E>& result) noexcept {
        if consteval {
            return std::holds_alternative<E>(result);
        }
        return result.index(); // at runtime is faster to check the index
    }

    template<typename T, typename E>
    constexpr bool is_ok(const Result<T, E>& result) noexcept {
        return !is_err(result);
    }
    
    namespace cppmatch_ranges{
        struct successes_fn {
            template <std::ranges::range R>
            friend auto operator|(R&& range, const successes_fn& self) {
                using VariantType = std::ranges::range_value_t<R>;
                static_assert(std::variant_size_v<VariantType> == 2, "Variants must have exactly two alternatives");
    
                return std::forward<R>(range)
                    | std::views::filter([](const auto& res) static { return is_ok(res); })
                    | std::views::transform([](const auto& res) static { return std::get<0>(res); });
            }
    
            template <std::ranges::range R>
            auto operator()(R&& range) const {
                return range | *this;
            }
        };
    
    } // namespace cppmatch_ranges

    inline constexpr cppmatch_ranges::successes_fn successes{};
} // namespace cppmatch

#define expect(expr) __extension__ ({                                         \
    auto&& expr_ = (expr);                                                     \
    if (cppmatch::is_err(expr_))                                                \
        return std::get<1>(std::move(expr_)); /* Handle error case */            \
    std::get<0>(std::move(expr_));                                                \
})



// This functionality is not yet decided to be included in the library, keep it "separated" for now to allow deletion if needed.
namespace cppmatch {
    namespace cppmatch_detail {
  
        template<typename... Es>
        constexpr bool all_same_v = std::conjunction_v<std::is_same<Es, std::tuple_element_t<0, std::tuple<Es...>>>...>;
        template<typename... Es>
        using deduced_error_t = std::conditional_t<
            all_same_v<Es...>,
            std::tuple_element_t<0, std::tuple<Es...>>,
            std::variant<Es...>
        >;

        template<typename R>
        using success_type_t = std::variant_alternative_t<0, std::decay_t<R>>;
        template<typename R>
        using error_type_t = std::variant_alternative_t<1, std::decay_t<R>>;



      // Recursively pick the first error from a tuple of arguments.
      template < std::size_t I, typename SuccessTuple, typename Tuple >
        constexpr auto pick_first_error_impl(const Tuple & tup) {
          if constexpr(I < std::tuple_size_v < Tuple > ) {
            using Expected = std::tuple_element_t < I, SuccessTuple > ;
            using Actual = std::decay_t < decltype(std::get < I > (tup)) > ;
            if constexpr(!std::is_same_v < Actual, Expected > ) {
              return std::get < I > (tup);
            } else {
              return pick_first_error_impl < I + 1, SuccessTuple > (tup);
            }
          } else {
            // This branch should be unreachable if an error is present.
            static_assert(true, "No error found in pick_first_error_impl");
          }
        }
  
      template < typename SuccessTuple, typename...Args >
        constexpr auto pick_first_error(Args && ...args) {
          auto tup = std::forward_as_tuple(args...);
          return pick_first_error_impl < 0, SuccessTuple > (tup);
        }
  
      // Internal implementation of zip_match.
      template < typename SuccessTuple, typename Return, typename F, typename...Rs, std::size_t...I >
        constexpr Return zip_match_impl(F && f, std::index_sequence < I... > ,
          const Rs & ...rs) {
          return std::visit(
            [ & ] < typename...Args > (Args && ...args) -> Return {
              constexpr bool allSuccess = ((std::is_same_v < std::decay_t < Args > , std::tuple_element_t < I, SuccessTuple >> ) && ...);
              if constexpr(allSuccess) {
                if constexpr(std::is_void_v < decltype(f(std::forward < Args > (args)...)) > ) {
                  f(std::forward < Args > (args)...);
                  return Return(std::in_place_index_t < 0 > {}, std::monostate {});
                } else {
                  return Return(std::in_place_index_t < 0 > {}, f(std::forward < Args > (args)...));
                }
              } else {
                return Return(std::in_place_index_t < 1 > {}, pick_first_error < SuccessTuple > (std::forward < Args > (args)...));
              }
            },
            rs...
          );
        }
  
    }
    template < typename F, typename...Rs >
      constexpr auto zip_match(F && f,
        const Rs & ...rs) {
        using SuccessTuple = std::tuple < cppmatch_detail::success_type_t < Rs > ... > ;
        using ErrorCommon = cppmatch_detail::deduced_error_t < cppmatch_detail::error_type_t < Rs > ... > ;
        using f_return_t = decltype(f(std::declval < cppmatch_detail::success_type_t < Rs >> ()...));
        using success_t = std::conditional_t < std::is_void_v < f_return_t > , std::monostate, f_return_t > ;
        using Return = Result < success_t, ErrorCommon > ;
        return cppmatch_detail::zip_match_impl < SuccessTuple, Return > (std::forward < F > (f), std::index_sequence_for < Rs... > {}, rs...);
      }
  
  }