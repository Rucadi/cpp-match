#include "match.hpp"
#include <string_view>
#include <iostream>
using namespace cppmatch;

struct ERR1{ std::string_view message; };
struct ERR2{ std::string_view message; };
Result<int, ERR1> getError() { return ERR1{"Error1"}; }
Result<int, ERR2> getError2() { return ERR2{"Error2"}; }

Result<int, Error<ERR1, ERR2>> test()
{
    int t = expect(getError());
    int t2 = expect(getError2());
    return 10;
}

int main()
{

    match(test(),
        [](int val) { std::cout << val << std::endl; },
        [](const ERR1& err) { std::cout << err.message << std::endl; },
        [](const ERR2& err) { std::cout << err.message << std::endl; }
    );

}