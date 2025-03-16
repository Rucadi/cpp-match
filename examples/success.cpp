#include "match.hpp"
#include <iostream>
#include <vector>
#include <string>
#include <sstream>
#include <charconv>
#include <ranges>
#include <print>

using cppmatch::Result;
using cppmatch::Error;
using cppmatch::successes;

// ANSI escape codes for colored output
inline constexpr std::string_view RESET  = "\033[0m";
inline constexpr std::string_view GREEN  = "\033[32m";
inline constexpr std::string_view RED    = "\033[31m";

// Structure to hold a coordinate
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

// Parses multiple coordinates from a string separated by semicolons
std::vector<Result<Coordinate, std::string>> parse_multiple_coordinates(const std::string& input) {
    std::vector<Result<Coordinate, std::string>> results;
    std::istringstream ss(input);
    std::string token;
    
    while (std::getline(ss, token, ';')) {
        token.erase(0, token.find_first_not_of(" \t")); // Trim left
        token.erase(token.find_last_not_of(" \t") + 1); // Trim right
        results.push_back(parse_coordinate(token));
    }

    return results;
}

// Processes and prints only successfully parsed coordinates
void process_coordinates(const std::vector<Result<Coordinate, std::string>>& results) {
    auto valid_coordinates = results | successes; // Extract only successful results

    std::print("{}Successfully parsed coordinates:{}\n", GREEN, RESET);
    for (const auto& coord : valid_coordinates) {
        std::print(" - Latitude: {}, Longitude: {}\n", coord.latitude, coord.longitude);
    }
}

int main() {
    std::string input = "40.7128,-74.0060; 34.0522,-118.2437; invalid,data; 91.0000,45.0000; 48.8566,2.3522";

    auto parsed_results = parse_multiple_coordinates(input);
    process_coordinates(parsed_results);

    return 0;
}
