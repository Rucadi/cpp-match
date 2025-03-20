# cppmatch

A lightweight header-only Clang/GCC compatible C++ library for expressive pattern matching and error handling.

Check an example in examples folder or [go to compiler-explorer to play with it!](https://compiler-explorer.com/z/9osq8WG7a)

## Overview

A header-only C++ library that offers exceptionless error handling and type-safe enums, bringing Rust-inspired error propagation with the ? operator and the match operator to C++.

The **expect** macro simplifies error propagation by automatically returning the error from a function when an error (Err) is encountered, and it continues with the success value (Ok) otherwise. This results in code that is both safe and easy to read without unnecessary verbosity.

It allows to defer the error-checking to the point where it is really important while ensuring that all the error options are handled.

# Performance

While I have not performed intensive testing, and performance can greately depend on the compiler version, I did perform some benchs based on: [exceptions vs errors](https://cedardb.com/blog/exceptions_vs_errors/) from CedarDB

As you can see, cppmatch implementation is equivalent to using std::expected, exceptions seem to be faster in recursive workloads, but I've seen that if you use "empty" types, cppmatch/expected code generation is better.

```
Results gotten from WSL / i9 9900K

Run on (16 X 3600.01 MHz CPU s)
CPU Caches:
  L1 Data 32 KiB (x8)
  L1 Instruction 32 KiB (x8)
  L2 Unified 256 KiB (x8)
  L3 Unified 16384 KiB (x1)
Load Average: 0.81, 0.63, 0.52
---------------------------------------------------------------------
Benchmark                           Time             CPU   Iterations
---------------------------------------------------------------------
recursive_fib_std_expected       3451 ns         3451 ns       203533
recursive_fib_cppmatch           3410 ns         3410 ns       207236
recursive_fib_throws             1714 ns         1714 ns       394960
coord_expected                    981 ns          981 ns       690791
coord_cppmatch                    978 ns          978 ns       722914
coord_throws                     1213 ns         1213 ns       580559
```

to run the tests perform nix command:
``nix run github.com:rucadi/cppmatch#benchmark`` 


## Types

### `Result<T, E>`
A type alias for `std::variant<T, E>`, representing either a **success value** of type `T` (index 0) or an **error value** of type `E` (index 1).

- **Example:**
  ```cpp
  cppmatch::Result<int, std::string> r = 42;  // Success value
  cppmatch::Result<int, std::string> e = std::string("fail");  // Error value
  ```

### `Error<Ts...>`
A type alias for `std::variant<Ts...>`, allowing multiple error types to be represented in a single variant.

- **Example:**
  ```cpp
  cppmatch::Error<std::string, int> err = 404;  // Error can be string or int
  ```

## Macros

### `expect(expr)`
A macro that evaluates `expr`, which must be a `Result<T, E>`. If the result holds a success value (`T`), it returns that value. If it holds an error (`E`), it **returns the error from the function**  propagating it up the call stack, essentially early-returning in case of error. This is useful for early returns in functions that return a `Result`.

- **Requirements:** The function using `expect` must return a `Result` type to handle the error case properly.
- **Example:**
  ```cpp
  cppmatch::Result<double, std::string> process(std::string_view floating_number) {
      double r = expect(parse_to_double(floating_number));
      // this line is only executed if parse_to_double succeded, if it failed
      // the function would early-return, as it happens with operator ? in rust.

      //we can use the result r as if no error could be returned.
      for(int i = 2; i < 10; ++i)
        r *= i;

      return r;
  }
  ```

This macro is dependent on 
[Statements and Declarations in Expressions](https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html) or also called braced-groups within expressions.

This extension, supported at least in **GCC** and **Clang** compilers, and is used in Linux Kernel  [linux kernel coding style guide](https://www.kernel.org/doc/html/v4.10/process/coding-style.html#macros-enums-and-rtl).

MSVC is not supported by this macro, if you want to use it in windows, use MinGW or  [Clang in visual studio](https://learn.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=msvc-170)
## Functions

### `match(v, lambdas...)`
Applies pattern matching on a variant `v` using the provided lambdas. Each lambda handles one of the possible types in the variant. For nested variants, it recursively visits until a non-variant value is reached and applies the appropriate lambda. 
Any callable which implements the operator() for the types is valid, so you can create structs overloading the operator () and pass them to this function too.

- **Parameters:**
  - `v`: The variant to match on.
  - `lambdas...`: Variadic lambdas, one for each possible type in the variant.
- **Return:** The result of the matched lambda.
- **Example:**
  ```cpp
  struct Err1{};
  struct Err2{};
  cppmatch::Result<int, cppmatch::Error<Err1, Err2>> v = 42;
  auto result = cppmatch::match(v,
      [](int i) { return "Integer: " + std::to_string(i); },
      [](Err1 err) { return "Error1"; },
      [](Err2 err) { return "Error2"; }
  );
  // result is "Integer: 42"
  ```

### `default_expect(result, default_value)`
Returns the success value of a `Result<T, E>` if it is a success, otherwise returns the provided `default_value` of type `T`.

- **Parameters:**
  - `result`: A `Result<T, E>`.
  - `default_value`: A value of type `T` to return if `result` is an error.
- **Return:** The success value or the default value.
- **Example:**
  ```cpp
  cppmatch::Result<int, std::string> r = std::string("error");
  int value = cppmatch::default_expect(r, 0);  // value is 0
  ```

### `map_error(result, f)`
Transforms the error type of a `Result<T, E1>` into a `Result<T, E2>` by applying the function `f` to the error if present. If the `Result` is a success, it remains unchanged.

- **Parameters:**
  - `result`: A `Result<T, E1>`.
  - `f`: A function that maps `E1` to `E2`.
- **Return:** A `Result<T, E2>`.
- **Example:**
  ```cpp
  struct Error1{ std::string_view message;};
  struct Error2{ std::string_view err;}
  cppmatch::Result<int, Error1> r = Error1{"hello world"};
  auto mapped = cppmatch::map_error(r, [](const Error1& s) { return Error2{s.message}; });
  // mapped is Result<int, Error2> with error "hello world"
  ```

### `successes`
A range adaptor that takes a range of `Result<T, E>` and returns a view containing only the success values (`T`). It filters out errors and extracts the success values using `std::get<0>`.

- **Usage:** Can be used with the pipe operator (`|`) or as a function call.
- **Example:**
  ```cpp
  std::vector<cppmatch::Result<int, std::string>> results = { 1, std::string("error"), 2 };
  auto success_values = results | cppmatch::successes;
  // success_values is a view containing 1 and 2
  ```

