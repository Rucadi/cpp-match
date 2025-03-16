#include "match.hpp"
#include <print>
#include <sstream>
#include <vector>
#include <string>
#include <charconv>
using namespace cppmatch;

// ANSI escape codes for colors
inline constexpr std::string_view RESET  = "\033[0m";
inline constexpr std::string_view RED    = "\033[31m";
inline constexpr std::string_view GREEN  = "\033[32m";
inline constexpr std::string_view YELLOW = "\033[33m";
inline constexpr std::string_view BLUE   = "\033[34m";
inline constexpr std::string_view CYAN   = "\033[36m";

// Define a simple structure to hold latitude & longitude
struct Coordinate {
    double latitude;
    double longitude;
};

// Exception-free string to double conversion using std::from_chars
Result<double, std::string> safe_str_to_double(const std::string& str) {
    double value = 0.0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    
    if (ec == std::errc::invalid_argument) return "Invalid number format";
    if (ec == std::errc::result_out_of_range) return "Number out of range";
    if (ptr != str.data() + str.size()) return "Extra characters found in number";

    return value;
}

// Parses a single latitude,longitude pair safely
Result<Coordinate, std::string> parse_coordinate(const std::string& input) {
    std::istringstream ss(input);
    std::string lat_str, lon_str;
    
    if (!std::getline(ss, lat_str, ',') || !std::getline(ss, lon_str)) {
        return "Invalid format (expected 'latitude,longitude')";
    }

    // Convert latitude and longitude safely
    double latitude = expect(safe_str_to_double(lat_str));
    double longitude = expect(safe_str_to_double(lon_str));
    
    // Validate ranges
    if (latitude < -90 || latitude > 90) return "Latitude out of range (-90 to 90)";
    if (longitude < -180 || longitude > 180) return "Longitude out of range (-180 to 180)";
    
    return Coordinate{latitude, longitude};
}

// Parses a string containing multiple coordinates separated by semicolons
Result<std::vector<Coordinate>, std::string> parse_coordinates(const std::string& input) {
    std::vector<Coordinate> coordinates;
    std::istringstream ss(input);
    std::string token;
    
    while (std::getline(ss, token, ';')) {
        token.erase(0, token.find_first_not_of(" \t")); // Trim left
        token.erase(token.find_last_not_of(" \t") + 1); // Trim right
        
        Coordinate coord = expect(parse_coordinate(token));
        coordinates.push_back(coord);
    }

    return coordinates;
}

// Pretty-printing a coordinate
void print_coordinate(const Coordinate& coord) {
    std::print("{}Parsed Coordinate -> Latitude: {}, Longitude: {}{}\n",
               GREEN, coord.latitude, coord.longitude, RESET);
}

// Testing our parser
void test_parser() {
    std::vector<std::string> test_cases = {
        "40.7128,-74.0060; 34.0522,-118.2437; 48.8566,2.3522",  // Valid
        "91.0000,45.0000; 50.0000,-30.0000", // Invalid latitude
        "50.0000,190.0000; -20.0000,120.0000", // Invalid longitude
        "abcd,efgh; 10.0,20.0",  // Invalid number format
        "10.5,20.5",  // Single valid coordinate
        "  40.0 , -75.0 ; 50.0 , -45.0   ",  // With extra spaces
        ";;"  // Empty inputs
    };

    for (const auto& test : test_cases) {
        std::print("{}\nInput: \"{}\"{}\n", CYAN, test, RESET);
        auto result = parse_coordinates(test);
        
        match(result,
            [](const std::vector<Coordinate>& coords) {
                std::print("{}Successfully parsed {} coordinates:\n{}", GREEN, coords.size(), RESET);
                for (const auto& coord : coords) {
                    print_coordinate(coord);
                }
            },
            [](const std::string& err) {
                std::print("{}Error: {}{}\n", RED, err, RESET);
            }
        );
    }
}

// Entry point
int main() {
    test_parser();
    return 0;
}