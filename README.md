# cppmatch

A lightweight header-only Clang/GCC compatible C++ library for expressive pattern matching and error handling.

## Overview

A header-only C++ library that offers exceptionless error handling and type-safe enums, bringing Rust-inspired error propagation with the ? operator and the match operator to C++.

The **expect** macro simplifies error propagation by automatically returning the error from a function when an error (Err) is encountered, and it continues with the success value (Ok) otherwise. This results in code that is both safe and easy to read without unnecessary verbosity.

It allows to defer the error-checking to the point where it is really important while ensuring that all the error options are handled.


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

      //we can use the result r as if no error culd be returned.
      for(int i = 2; i < 10; ++i)
        r *= i;

      return r;
  }
  ```

This macro is dependent on 
[Statements and Declarations in Expressions](https://gcc.gnu.org/onlinedocs/gcc/Statement-Exprs.html) or also called braced-groups within expressions.

This extension, supported at least in **GCC** and **Clang** compilers, and is used in Linux Kernel  [linux kernel coding style guide](https://www.kernel.org/doc/html/v4.10/process/coding-style.html#macros-enums-and-rtl).

MSVC is not supported by this macro, if you want to use it in windows, use MinGW or  [Clang on visual studio](https://learn.microsoft.com/en-us/cpp/build/clang-support-msbuild?view=msvc-170)
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

### `zip_match(f, rs...)`
Combines multiple `Result`s. If all `Result`s are successes, it applies the function `f` to the success values and returns a `Result` with the result of `f`. If any `Result` is an error, it returns the first error encountered. The error type is deduced as a common type if all errors are the same, or a `std::variant` of the error types otherwise.

- **Parameters:**
  - `f`: A function to apply to the success values.
  - `rs...`: Variadic list of `Result` objects.
- **Return:** A `Result` with either the result of `f` or the first error.
- **Example:**
  ```cpp
  int value1 = expect(get_new_int());
  double value2 = expect(get_new_double());

  auto combined = cppmatch::zip_match(
      [](int i, double d) { return i + d; },
      r1, r2
  );
  // combined is a Result<double, Error<A,B>> where A, B are the possible errors of value1 and value2
  // if value1 and value2 where valid, the function is executed and combined has a correct value
  // if it was invalid, combined is a Result with the first error value encountered 
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

