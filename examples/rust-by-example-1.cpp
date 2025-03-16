//https://doc.rust-lang.org/rust-by-example/std/result/question_mark.html

#include "match.hpp"
#include <iostream>
#include <cmath>
#include <cstdlib>
using namespace cppmatch;

// Define an enum class for math errors.
enum class MathError {
    DivisionByZero,
    NonPositiveLogarithm,
    NegativeSquareRoot
};

// Type alias for our math result.
using MathResult = Result<double, MathError>;

// Returns x/y or an error if y is zero.
MathResult div_(double x, double y) {
    if (y == 0.0)
        return MathError::DivisionByZero;
    else
        return x / y;
}

// Returns the square root of x or an error if x is negative.
MathResult sqrt_(double x) {
    if (x < 0.0)
        return MathError::NegativeSquareRoot;
    else
        return std::sqrt(x);
}

// Returns the natural logarithm of x or an error if x is non-positive.
MathResult ln_(double x) {
    if (x <= 0.0)
        return MathError::NonPositiveLogarithm;
    else
        return std::log(x);
}

// Intermediate function that chains the operations using expect (similar to the ? operator).
MathResult op_(double x, double y) {
    // If div_ fails, its error is immediately returned.
    double ratio = expect(div_(x, y));
    // If ln_ fails, its error is returned.
    double ln_val = expect(ln_(ratio));
    // Finally, return the result of the square root computation.
    return expect(sqrt_(ln_val));
}

// Public function that calls op_ and either prints the result or prints an error message and exits.
void op(double x, double y) {
    auto result = op_(x, y);
    match(result,
        [](double value) { // On success, print the computed value.
            std::cout << value << std::endl;
        },
        [](const MathError& err) { // On error, print a corresponding message and exit.
            switch (err) {
                case MathError::DivisionByZero:
                    std::cerr << "division by zero" << std::endl;
                    break;
                case MathError::NonPositiveLogarithm:
                    std::cerr << "logarithm of non-positive number" << std::endl;
                    break;
                case MathError::NegativeSquareRoot:
                    std::cerr << "square root of negative number" << std::endl;
                    break;
            }
            std::exit(1);
        }
    );
}

int main() {
    op(1.0, 10.0);
    return 0;
}
