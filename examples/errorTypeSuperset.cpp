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

std::string generate_random_coordinate_string() {
    static std::random_device rd;
    static std::mt19937 gen(rd());
    
    std::uniform_real_distribution<> prob_dist(0.0, 1.0);
    double probability = prob_dist(gen);
    
    if (probability < 0.9) { // 90% chance of valid coordinates
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


struct NullError{};
struct Err3{int B;};
Result<double, Error<NullError, Err3>> safe_str_to_double_cppmatch_monostate(std::string_view str) {
    double value = 0.0;
    auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), value);
    if (ec == std::errc::invalid_argument)
        return NullError{};
    if (ec == std::errc::result_out_of_range)
        return NullError{};
    if (ptr != str.data() + str.size())
        return Err3{0};
    return value;
}



Result<Coordinate, Error<NullError, std::string, std::monostate,  Err3>> parse_coordinate_cppmatch_monostate(const std::string& input) {
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
    return std::string{"error"};
    if (false) return std::monostate{};
    
    return Coordinate{latitude, longitude};
}

static void coord_cppmatch_monostate() {
    //for (auto _ : state) {
        std::string input = generate_random_coordinate_string();
        auto result = parse_coordinate_cppmatch_monostate(input);
   // }
}

int main()
{
    coord_cppmatch_monostate();
}