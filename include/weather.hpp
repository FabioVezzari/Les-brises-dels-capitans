#pragma once
#include <string>

struct Wind {
    double speed_kn{0.0}; // wind speed in knots
    double from_deg{0.0}; // wind FROM (meteo), degrees
};

std::string read_openweather_api_key(const std::string& config_path);

Wind fetch_wind_openweather(double lat_deg, double lon_deg, const std::string& api_key);
