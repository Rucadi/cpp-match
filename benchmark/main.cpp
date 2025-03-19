//benchmark copied from  https://cedardb.com/blog/exceptions_vs_errors/
#include <benchmark/benchmark.h>

#include "match.hpp"

#include <expected>
#include <string>
#include <sstream>
#include <vector>
#include <random>
#include <charconv>
#include <cassert>
#include <iostream>


using namespace cppmatch;

struct invalid_value { std::string reason; };

unsigned do_fib_throws(unsigned n, unsigned max_depth) {
   if (!max_depth) throw invalid_value(std::to_string(n) + " exceeds max_depth");
   if (n <= 2) return 1;
   return do_fib_throws(n - 2, max_depth - 1) + do_fib_throws(n - 1, max_depth - 1);
}

std::expected<unsigned, invalid_value> do_fib_expected(unsigned n, unsigned max_depth) {
   if (!max_depth) return std::unexpected<invalid_value>(std::to_string(n) + " exceeds max_depth");
   if (n <= 2) return 1;
   auto n2 = do_fib_expected(n - 2, max_depth - 1);
   if (!n2) return std::unexpected(n2.error());
   auto n1 = do_fib_expected(n - 1, max_depth - 1);
   if (!n1) return std::unexpected(n1.error());
   return *n1 + *n2;
}

Result<unsigned, invalid_value> do_fib_cppmatch(unsigned n, unsigned max_depth) {
   if (!max_depth) return invalid_value{std::to_string(n) + " exceeds max_depth"};
   if (n <= 2) return 1U;
   return expect(do_fib_cppmatch(n - 2, max_depth - 1)) + expect(do_fib_cppmatch(n - 1, max_depth - 1));
}

static void recursive_fib_std_expected(benchmark::State& state) {
  for (auto _ : state) {
    auto res = do_fib_expected(15, 20);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(recursive_fib_std_expected);

static void recursive_fib_cppmatch(benchmark::State& state) {
  for (auto _ : state) {
    auto res = do_fib_cppmatch(15, 20);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(recursive_fib_cppmatch);


static void recursive_fib_throws(benchmark::State& state) {
  for (auto _ : state) {
    auto res = do_fib_throws(15, 20);
    benchmark::DoNotOptimize(res);
  }
}
BENCHMARK(recursive_fib_throws);


//=== Coordinate Parsing Benchmarks ===

std::string generate_random_coordinate_string() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    double probability = prob_dist(gen);
    
    if (probability < 0.1) { // 90% chance of valid coordinates
        std::uniform_real_distribution<> lat_dist(-90.0, 90.0);
        std::uniform_real_distribution<> lon_dist(-180.0, 180.0);
        double lat = lat_dist(gen);
        double lon = lon_dist(gen);
        return std::to_string(lat) + "," + std::to_string(lon);
    } else { // 10% chance of invalid coordinates
        std::uniform_int_distribution<> error_dist(0, 2);
        int error_type = error_dist(gen);
        
        if (error_type == 0) {
            return "abc,def"; // Completely invalid format
        } else if (error_type == 1) {
            std::uniform_real_distribution<> lon_dist(-180.0, 180.0);
            double lon = lon_dist(gen);
            return "100.0," + std::to_string(lon); // Invalid latitude (100 > 90)
        } else {
            std::uniform_real_distribution<> lat_dist(-90.0, 90.0);
            double lat = lat_dist(gen);
            return std::to_string(lat) + ",200.0"; // Invalid longitude (200 > 180)
        }
    }
}

struct Coordinate {
    double latitude;
    double longitude;
};

struct InvalidDoubleConversion { std::string_view message; };
struct InvalidCoordinate       { std::string_view message; };
struct InvalidCoordinateFormat { std::string_view message; };

std::expected<double, InvalidDoubleConversion> safe_str_to_double_expected(std::string_view str) {
    double value = 0.0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    
    if (ec == std::errc::invalid_argument) {
        return std::unexpected{InvalidDoubleConversion{"Invalid number format"}};
    }
    if (ec == std::errc::result_out_of_range) {
        return std::unexpected{InvalidDoubleConversion{"Number out of range"}};
    }
    if (ptr != str.data() + str.size()) {
        return std::unexpected{InvalidDoubleConversion{"Extra characters found in number"}};
    }
    
    return value; 
}

std::expected<Coordinate, std::variant<InvalidDoubleConversion, InvalidCoordinate, InvalidCoordinateFormat>> 
parse_coordinate_expected(const std::string& input) {
    std::istringstream ss(input);
    std::string lat_str, lon_str;

    if (!std::getline(ss, lat_str, ',') || !std::getline(ss, lon_str)) {
        return std::unexpected{InvalidCoordinateFormat{"Invalid format (expected 'latitude,longitude')"}};
    }

    auto lat_result = safe_str_to_double_expected(lat_str);
    if (!lat_result.has_value()) {
        return std::unexpected{lat_result.error()};
    }

    auto lon_result = safe_str_to_double_expected(lon_str);
    if (!lon_result.has_value()) {
        return std::unexpected{lon_result.error()};
    }

    double latitude = lat_result.value();
    double longitude = lon_result.value();

    if (latitude < -90 || latitude > 90) {
        return std::unexpected{InvalidCoordinate{"Latitude out of range (-90 to 90)"}};
    }
    if (longitude < -180 || longitude > 180) {
        return std::unexpected{InvalidCoordinate{"Longitude out of range (-180 to 180)"}};
    }

    return Coordinate{latitude, longitude};
}




Result<double, InvalidDoubleConversion> safe_str_to_double_cppmatch(std::string_view str) {
    double value = 0.0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    if (ec == std::errc::invalid_argument)
        return InvalidDoubleConversion{"Invalid number format"};
    if (ec == std::errc::result_out_of_range)
        return InvalidDoubleConversion{"Number out of range"};
    if (ptr != str.data() + str.size())
        return InvalidDoubleConversion{"Extra characters found in number"};
    return value;
}



Result<Coordinate, Error<InvalidDoubleConversion, InvalidCoordinate, InvalidCoordinateFormat>>
parse_coordinate_cppmatch(const std::string& input) {
    std::istringstream ss(input);
    std::string lat_str, lon_str;
    
    if (!std::getline(ss, lat_str, ',') || !std::getline(ss, lon_str))
        return InvalidCoordinateFormat{"Invalid format (expected 'latitude,longitude')"};
    
    double latitude  = expect(safe_str_to_double_cppmatch(lat_str));
    double longitude = expect(safe_str_to_double_cppmatch(lon_str));
    
    if (latitude < -90 || latitude > 90)
        return InvalidCoordinate{"Latitude out of range (-90 to 90)"};
    if (longitude < -180 || longitude > 180)
        return InvalidCoordinate{"Longitude out of range (-180 to 180)"};
    
    return Coordinate{latitude, longitude};
}


double safe_str_to_double_throws(std::string_view str) {
    double value = 0.0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);

    if (ec == std::errc::invalid_argument) {
        throw InvalidDoubleConversion{"Invalid number format"};
    }
    if (ec == std::errc::result_out_of_range) {
        throw InvalidDoubleConversion{"Number out of range"};
    }
    if (ptr != str.data() + str.size()) {
        throw InvalidDoubleConversion{"Extra characters found in number"};
    }

    return value;
}


Coordinate parse_coordinate_throws(const std::string& input) {
    std::istringstream ss(input);
    std::string lat_str, lon_str;

    if (!std::getline(ss, lat_str, ',') || !std::getline(ss, lon_str)) {
        throw InvalidCoordinateFormat{"Invalid format (expected 'latitude,longitude')"};
    }

    double latitude = safe_str_to_double_throws(lat_str);
    double longitude = safe_str_to_double_throws(lon_str);

    if (latitude < -90 || latitude > 90) {
        throw InvalidCoordinate{"Latitude out of range (-90 to 90)"};
    }
    if (longitude < -180 || longitude > 180) {
        throw InvalidCoordinate{"Longitude out of range (-180 to 180)"};
    }

    return Coordinate{latitude, longitude};
}


static void coord_expected(benchmark::State& state) {
    for (auto _ : state) {
        std::string input = generate_random_coordinate_string();
        auto result = parse_coordinate_expected(input);       
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(coord_expected);


static void coord_cppmatch(benchmark::State& state) {
    for (auto _ : state) {
        std::string input = generate_random_coordinate_string();
        auto result = parse_coordinate_cppmatch(input);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(coord_cppmatch);



static void coord_throws(benchmark::State& state) {
    for (auto _ : state) {
        std::string input = generate_random_coordinate_string();
        try {
            auto result = parse_coordinate_throws(input);
            benchmark::DoNotOptimize(result);
        } catch (const InvalidDoubleConversion& e) {
            benchmark::DoNotOptimize(e);
        } catch (const InvalidCoordinate& e) {
            benchmark::DoNotOptimize(e);
        } catch (const InvalidCoordinateFormat& e) {
            benchmark::DoNotOptimize(e);
        }
    }
}
BENCHMARK(coord_throws);

struct ERR{ std::string_view message; };

struct NullError{};

struct Err3{int B;};
Result<double, Error<NullError, ERR>> safe_str_to_double_cppmatch_monostate(std::string_view str) {
    double value = 0.0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    if (ec == std::errc::invalid_argument)
        return NullError{};
    if (ec == std::errc::result_out_of_range)
        return NullError{};
    if (ptr != str.data() + str.size())
        return ERR{"AFDASF"};
    return value;
}




Result<Coordinate, Error<NullError, ERR, Err3>> parse_coordinate_cppmatch_monostate(const std::string& input) {
    std::istringstream ss(input);
    std::string lat_str, lon_str;
    
    if (!std::getline(ss, lat_str, ',') || !std::getline(ss, lon_str))
        return NullError{};
    
    double latitude  = expect(safe_str_to_double_cppmatch_monostate(lat_str));
    double longitude = expect(safe_str_to_double_cppmatch_monostate(lon_str));
    
    if (latitude < -90 || latitude > 90)
        return NullError{};
    if (longitude < -180 || longitude > 180)
        return NullError{};
    
    return Coordinate{latitude, longitude};
}

static void coord_cppmatch_monostate(benchmark::State& state) {
    for (auto _ : state) {
        std::string input = generate_random_coordinate_string();
        auto result = parse_coordinate_cppmatch_monostate(input);
        benchmark::DoNotOptimize(result);
    }
}
BENCHMARK(coord_cppmatch_monostate);

BENCHMARK_MAIN();