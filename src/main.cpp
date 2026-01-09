#include <filesystem>
#include <chrono>
#include <thread>
#include <iomanip>
#include <cmath>
#include <sstream>
#include <stdexcept>
#include <string>
#include <ctime>
#include <iostream>
#include <vector>
#include <unordered_map>

#include "ship_class.hpp"
#include "sim.hpp"
#include "state.hpp"
#include "orders.hpp"
#include "weather.hpp"
#include "geo.hpp"

namespace fs = std::filesystem;

// ------------------------------------------------------------
// Utilities
// ------------------------------------------------------------
static std::string timestamp_utc_compact() {
    using namespace std::chrono;
    const auto now = system_clock::now();
    const std::time_t t = system_clock::to_time_t(now);
    std::tm tm{};
    gmtime_r(&t, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y%m%d_%H%M%S");
    return oss.str();
}

static void backup_file(const std::string& src_path, const std::string& backup_dir) {
    if (!fs::exists(src_path)) return;

    fs::create_directories(backup_dir);

    const fs::path src(src_path);
    const std::string ts = timestamp_utc_compact();
    const fs::path dst = fs::path(backup_dir) /
        (src.stem().string() + "_" + ts + src.extension().string());

    fs::copy_file(src, dst, fs::copy_options::overwrite_existing);
}

static void print_pos(const std::string& label, const GeoPoint& p) {
    std::cout << label
              << " lat=" << std::fixed << std::setprecision(6) << p.lat_deg
              << " lon=" << std::fixed << std::setprecision(6) << p.lon_deg
              << "\n";
}

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------
int main() {
    try {
        // Paths
        const std::string ship_classes_path = "data/ship_classes.json";
        const std::string game_state_path   = "data/game_state.geojson";
        const std::string orders_path       = "data/navigation_orders.geojson";
        const std::string backup_dir        = "data/_backup";

        // Backup
        backup_file(game_state_path, backup_dir);
        backup_file(orders_path, backup_dir);

        // Load ship classes
        ShipClassRegistry classes;
        classes.load_from_file(ship_classes_path);

        // Load state and orders
        std::vector<Ship> ships = load_game_state(game_state_path);
        OrdersByShip orders = load_orders_geojson(orders_path);

        // --------------------------------------------------------
        // Fetch wind ONCE per ship (I/O belongs here)
        // --------------------------------------------------------
        std::unordered_map<std::string, Wind> wind_by_ship;
        const std::string api_key = read_openweather_api_key("config.local.json");

        using namespace std::chrono_literals;

        for (const auto& s : ships) {
            wind_by_ship[s.id] = fetch_wind_openweather(
                s.pos.lat_deg,
                s.pos.lon_deg,
                api_key
            );

            // Rate limiting: wait 1s between API calls
            std::this_thread::sleep_for(1s);
        }


        // Simulation parameters
        const double dt_h = 0.25;          // 15 minutes
        const double arrival_radius_nm = 0.03;

        // --------------------------------------------------------
        // TURN START LOGGING
        // --------------------------------------------------------
        std::cout << "\n================ TURN START ================\n";

        for (const auto& s : ships) {
            std::cout << "\nShip: " << s.id << " (" << s.name << ")\n";
            print_pos("  Initial position:", s.pos);

            const auto* cls = classes.find(s.class_id);
            if (!cls) continue;

            const Wind& wind = wind_by_ship.at(s.id);

            std::cout << "  Wind:\n";
            std::cout << "    TWS  = " << std::setprecision(2) << wind.speed_kn << " kn\n";
            std::cout << "    FROM = " << wind.from_deg << " deg\n";

            auto it = orders.find(s.id);
            if (it != orders.end() && !it->second.empty()) {
                const GeoPoint target = it->second.front();

                const double heading = bearing_deg(s.pos, target);
                const double twa = angle_diff_0_180(wind.from_deg, heading);
                const double speed = cls->speed_from_polar_kn(wind.speed_kn, twa);

                std::cout << "  Navigation:\n";
                std::cout << "    Heading = " << heading << " deg\n";
                std::cout << "    TWA     = " << twa << " deg\n";
                std::cout << "    Speed   = " << speed << " kn\n";
                std::cout << "    Step    = " << speed * dt_h << " nm\n";
            } else {
                std::cout << "  No navigation orders\n";
            }
        }

        std::cout << "============================================\n\n";

        // --------------------------------------------------------
        // SIMULATION (pure physics)
        // --------------------------------------------------------
        simulate_turn(
            ships,
            orders,
            classes,
            wind_by_ship,
            dt_h,
            arrival_radius_nm
        );

        // --------------------------------------------------------
        // TURN END LOGGING
        // --------------------------------------------------------
        std::cout << "\n================ TURN END ==================\n";

        for (const auto& s : ships) {
            std::cout << "\nShip: " << s.id << " (" << s.name << ")\n";
            print_pos("  Final position:", s.pos);
        }

        std::cout << "============================================\n\n";

        // Save updated state
        save_game_state(game_state_path, ships);
        save_orders_geojson(orders_path, orders);

        std::cout << "Simulation turn completed.\n";
        return 0;
    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << "\n";
        return 1;
    }
}
