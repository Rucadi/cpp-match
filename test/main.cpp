#include "match.hpp"

#include <print>
#include <iostream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <vector>
#include <source_location>
#include <string_view>

using namespace cppmatch;

// ANSI escape codes as inline constexpr string_views.
inline constexpr std::string_view RESET  = "\033[0m";
inline constexpr std::string_view RED    = "\033[31m";
inline constexpr std::string_view GREEN  = "\033[32m";
inline constexpr std::string_view YELLOW = "\033[33m";
inline constexpr std::string_view BLUE   = "\033[34m";

// A helper function that throws an exception if the check fails.
// Uses std::source_location to provide file and line information.
inline void check(bool expr, std::string_view exprStr,
                  const std::source_location& loc = std::source_location::current()) {
    if (!expr) {
        throw std::runtime_error("Check failed: " + std::string(exprStr) + " at " +
                                   loc.file_name() + ":" + std::to_string(loc.line()));
    }
}

// A macro that calls our helper function. This keeps the convenience of using the expression text.
#define CHECK(expr) ({ check((expr), #expr); })


// ---------------------------------------------------------------------------
// Test functions using the expect macro.
//
// test_expect_macro_success returns a success result if expect extracts the value;
// test_expect_macro_error returns early with an error.
Result<int, std::string> test_expect_macro_success() {
    Result<int, std::string> res = 10; // success variant (index 0)
    int value = expect(res);
    return value; // Should be a success containing 10.
}

Result<int, Error<std::string>> test_expect_macro_error() {
    Result<int, std::string> res = std::string("Failed"); // error variant (index 1)
    int value = expect(res); // expect returns early with the error.
    return value; // This line is never reached.
}

// ---------------------------------------------------------------------------
// A simple test runner helper that prints colorful output.
template<typename Func>
void run_test(const char* test_name, Func test_func, int& passed, int& failed) {
    std::print("{}Running {}...{}\n", BLUE, test_name, RESET);
    try {
        test_func();
        std::print("{}{} passed.{}\n\n", GREEN, test_name, RESET);
        ++passed;
    } catch (const std::exception& e) {
        std::print("{}{} failed: {}{}\n\n", RED, test_name, e.what(), RESET);
        ++failed;
    } catch (...) {
        std::print("{}{} failed with unknown error.{}\n\n", RED, test_name, RESET);
        ++failed;
    }
}

// ---------------------------------------------------------------------------
// Test suite that exercises the library's features.
void run_tests() {
    int passed = 0, failed = 0;

    run_test("test_expect_macro_success", [](){
        auto res = test_expect_macro_success();
        CHECK(res.index() == 0);
        CHECK(std::get<0>(res) == 10);
    }, passed, failed);

    run_test("test_expect_macro_error", [](){
        auto res = test_expect_macro_error();
        CHECK(res.index() == 1);
    }, passed, failed);

    run_test("is_ok() with success variant", [](){
        Result<int, std::string> r = 5;
        CHECK(is_ok(r));
    }, passed, failed);

    run_test("is_ok() with error variant", [](){
        Result<int, std::string> r = std::string("oops");
        CHECK(!is_ok(r));
    }, passed, failed);

    run_test("is_err() with success variant", [](){
        Result<int, std::string> r = 5;
        CHECK(!is_err(r));
    }, passed, failed);
    
    run_test("is_err() with error variant", [](){
        Result<int, std::string> r = std::string("oops");
        CHECK(is_err(r));
    }, passed, failed);

    run_test("match() with success variant", [](){
        Result<int, std::string> r = 5;
        auto result = match(r,
            [](int val) { return val * 2; },
            [](const std::string& /*err*/) { return -1; }
        );
        CHECK(result == 10);
    }, passed, failed);

    run_test("match() with error variant", [](){
        Result<int, std::string> r = std::string("oops");
        auto result = match(r,
            [](int val) { return val * 2; },
            [](const std::string& /*err*/) { return -1; }
        );
        CHECK(result == -1);
    }, passed, failed);

    run_test("Error conversion: from Error<int, float> to Error<int, float, std::string>", [](){
        // Create a small error type.
        Error<int, float> e_small = 3.14f;
        // Convert it to a larger error type.
        Error<int, float, std::string> e_large = e_small;
        // Wrap the error into a Result.
        Result<double, Error<int, float, std::string>> r = e_large;
        auto result = match(r,
            [](long long int val) { return 1; },
            [](int e) { return 2; },
            [](float e) { return  3; },
            [](double e) { return  4; },
            [](const std::string& err) { return 5; }
        );
            
        auto result2 = match(r, 
            [](double){return 1;},
            [](const auto& e) {
                 return match(e,
                    [](int){return 2;},
                    [](float){return 3;},
                    [](const std::string&){return 4;});
            }
        );
        CHECK(result == 3);
        CHECK(result2 == result);
      }, passed, failed);

    run_test("zip_match() with two successes", [](){
        Result<int, std::string> a = 3;
        Result<int, std::string> b = 7;
        auto result = zip_match([](int x, int y) { return x + y; }, a, b);
        CHECK(result.index() == 0);
        CHECK(std::get<0>(result) == 10);
    }, passed, failed);

    run_test("zip_match() with one error", [](){
        Result<int, std::string> a = 3;
        Result<int, std::string> b = std::string("error in b");
        auto result = zip_match([](int x, int y) { return x + y; }, a, b);
        CHECK(result.index() == 1);
        CHECK(std::get<1>(result) == "error in b");
    }, passed, failed);

    run_test("zip_match() with both errors", [](){
        Result<int, std::string> a = std::string("first error");
        Result<int, std::string> b = std::string("second error");
        auto result = zip_match([](int x, int y) { return x + y; }, a, b);
        CHECK(result.index() == 1);
        CHECK(std::get<1>(result) == "first error");
    }, passed, failed);

    run_test("zip_match() with three successes", [](){
        Result<int, std::string> a = 2;
        Result<int, std::string> b = 3;
        Result<int, std::string> c = 4;
        auto result = zip_match([](int x, int y, int z) { return x * y * z; }, a, b, c);
        CHECK(result.index() == 0);
        CHECK(std::get<0>(result) == 24);
    }, passed, failed);

    run_test("zip_match() with three arguments (one error)", [](){
        Result<int, std::string> a = 2;
        Result<int, std::string> b = std::string("error in b");
        Result<int, std::string> c = 4;
        auto result = zip_match([](int x, int y, int z) { return x * y * z; }, a, b, c);
        CHECK(result.index() == 1);
        CHECK(std::get<1>(result) == "error in b");
    }, passed, failed);

    run_test("zip_match() void zip_match returns std::monostate as Ok value", [](){
        Result<int, std::string> a = 2;
        Result<int, std::string> b = 1;
        Result<int, std::string> c = 4;
        auto t = zip_match([](int x, int y, int z) -> void { }, a, b, c);
        CHECK(t.index() == 0);
        static_assert(std::is_same_v<std::variant_alternative_t<0, decltype(t)>, std::monostate>, "Expected monostate");
    }, passed, failed);

    run_test("map_error with success", [](){
        Result<int, std::string> r = 42;
        auto r2 = map_error(r, [](const std::string& s) { return s.size(); });
        CHECK(r2.index() == 0);
        CHECK(std::get<0>(r2) == 42);
    }, passed, failed);

    run_test("map_error with error", [](){
        struct ErrorType1 {};
        struct ErrorType2 {};
        Result<int, ErrorType1> r = ErrorType1{};  // Error case
        auto r2 = map_error(r, [](const ErrorType1& /*s*/) { return ErrorType2{}; });
        CHECK(r2.index() == 1);
        CHECK(std::holds_alternative<ErrorType2>(r2));
    }, passed, failed);

    run_test("successes range adaptor", [](){
        std::vector<Result<int, std::string>> results = {
            Result<int, std::string>{1},
            Result<int, std::string>{"error"},
            Result<int, std::string>{2},
            Result<int, std::string>{"oops"}
        };
        
        std::vector<int> collected;
        for (int i : results | cppmatch::successes | std::views::transform([](int x){ return x * x; })) { 
            collected.push_back(i);
        }
        
        std::vector<int> expected = {1, 4};
        CHECK(collected == expected);
    }, passed, failed);
    
    std::print("{}Summary: Tests passed: {}, Tests failed: {}{}\n", YELLOW, passed, failed, RESET);

}

// ---------------------------------------------------------------------------
// Main function.
int main() {
    run_tests();
    return 0;
}
