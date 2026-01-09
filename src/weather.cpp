#include "weather.hpp"

#include <array>
#include <cstdio>
#include <fstream>
#include <memory>
#include <sstream>
#include <stdexcept>
#include <string>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

static std::string exec_cmd(const std::string& cmd) {
    std::array<char, 4096> buffer{};
    std::string result;

    struct PcloseDeleter {
        void operator()(FILE* f) const noexcept { if (f) pclose(f); }
    };

    std::unique_ptr<FILE, PcloseDeleter> pipe(popen(cmd.c_str(), "r"));
    if (!pipe) {
        throw std::runtime_error("Failed to run command: " + cmd);
    }

    while (fgets(buffer.data(), (int)buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}


std::string read_openweather_api_key(const std::string& config_path) {
    std::ifstream in(config_path);
    if (!in) {
        throw std::runtime_error("Cannot open config file: " + config_path);
    }

    json j = json::parse(in);

    if (!j.contains("openweather") || !j["openweather"].is_object()) {
        throw std::runtime_error("Config missing 'openweather' object");
    }

    const auto& ow = j["openweather"];

    if (!ow.contains("api_key") || !ow["api_key"].is_string()) {
        throw std::runtime_error("Config missing 'openweather.api_key'");
    }

    const std::string key = ow["api_key"].get<std::string>();
    if (key.empty() || key == "PUT_YOUR_KEY_HERE") {
        throw std::runtime_error("OpenWeather api_key not set in " + config_path);
    }

    return key;
}

Wind fetch_wind_openweather(double lat_deg, double lon_deg, const std::string& api_key) {
    static constexpr double kMpsToKn = 1.943844;

    std::ostringstream url;
    url.setf(std::ios::fixed);
    url.precision(6);

    url << "https://api.openweathermap.org/data/2.5/weather"
        << "?lat=" << lat_deg
        << "&lon=" << lon_deg
        << "&appid=" << api_key
        << "&units=metric";

    const std::string cmd = "curl -s --fail \"" + url.str() + "\"";
    const std::string body = exec_cmd(cmd);

    if (body.empty()) {
        throw std::runtime_error("OpenWeather response empty");
    }

    json j = json::parse(body);

    if (!j.contains("wind") || !j["wind"].is_object()) {
        throw std::runtime_error("OpenWeather response missing 'wind'");
    }

    const auto& w = j["wind"];

    if (!w.contains("speed") || !w["speed"].is_number()) {
        throw std::runtime_error("OpenWeather wind missing 'speed'");
    }

    double speed_mps = w["speed"].get<double>();
    double from_deg = w.value("deg", 0.0);

    Wind out;
    out.speed_kn = speed_mps * kMpsToKn;
    out.from_deg = from_deg;
    return out;
}
