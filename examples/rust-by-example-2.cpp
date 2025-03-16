//https://doc.rust-lang.org/rust-by-example/std/result/question_mark.html

#include "match.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib>
#include <string>
using namespace cppmatch;

// Define individual error types as structs.
struct DivisionByZeroError {
    std::string message;
};

struct NonPositiveLogarithmError {
    std::string message;
};

struct NegativeSquareRootError {
    std::string message;
};


// Returns x/y or an error if y is zero.
Result<double, DivisionByZeroError> div_(double x, double y) {
    if (y == 0.0)
        return DivisionByZeroError{"division by zero"};
    else
        return x / y;
}

// Returns the natural logarithm of x or an error if x is non-positive.
Result<double, NonPositiveLogarithmError>  ln_(double x) {
    if (x <= 0.0)
        return NonPositiveLogarithmError{"logarithm of non-positive number"};
    else
        return std::log(x);
}

// Returns the square root of x or an error if x is negative.
Result<double, NegativeSquareRootError> sqrt_(double x) {
    if (x < 0.0)
        return NegativeSquareRootError{"square root of negative number"};
    else
        return std::sqrt(x);
}

// Alias for a combined error type.
using MathError = Error<DivisionByZeroError, NonPositiveLogarithmError, NegativeSquareRootError>;

// Type alias for our math result.
using MathResult = Result<double, MathError>;


// Intermediate function that chains the operations using expect (similar to the ? operator).
MathResult op_(double x, double y) {
    double ratio = expect(div_(x, y));
    double ln_val = expect(ln_(ratio));
    return expect(sqrt_(ln_val));;
}

// Public function that calls op_ and either prints the result or prints an error message and exits.
void op(double x, double y) {
    auto result = op_(x, y);
    match(result,
        [](double value) { // On success, print the computed value.
            std::cout << value << std::endl;
        },
        [](const DivisionByZeroError& err) { // Handle division by zero error.
            std::cerr << err.message << std::endl;
            std::exit(1);
        },
        [](const NonPositiveLogarithmError& err) { // Handle logarithm error.
            std::cerr << err.message << std::endl;
            std::exit(1);
        },
        [](const NegativeSquareRootError& err) { // Handle square root error.
            std::cerr << err.message << std::endl;
            std::exit(1);
        }
    );
}

int main() {
    op(1.0, 10.0);
    return 0;
}
