#include "match.hpp"
#include <string_view>
#include <iostream>
#include <cmath>
using namespace cppmatch;

enum class MathError {
    NegativeSquareRoot
};

Result<double, MathError> sqrt_(double x) {
    if (x < 0.0)
        return MathError::NegativeSquareRoot;
    else
        return std::sqrt(x);
}

int main() {
    double result = default_expect(sqrt_(-4.0), -5.1);

    std::cout << result << std::endl;
    
    return 0;
}