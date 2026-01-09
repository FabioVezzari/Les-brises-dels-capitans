#include <filesystem>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <ctime>

#include <iostream>
#include <vector>

#include "ship_class.hpp"
#include "sim.hpp"
#include "state.hpp"
#include "orders.hpp"

namespace fs = std::filesystem;

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

int main() {
    try {
        const std::string ship_classes_path = "data/ship_classes.json";
        const std::string game_state_path   = "data/game_state.geojson";
        const std::string orders_path       = "data/navigation_orders.geojson";
        const std::string backup_dir        = "data/_backup";

        backup_file(game_state_path, backup_dir);
        backup_file(orders_path, backup_dir);

        ShipClassRegistry classes;
        classes.load_from_file(ship_classes_path);

        std::vector<Ship> ships = load_game_state(game_state_path);
        OrdersByShip orders = load_orders_geojson(orders_path);

        const double dt_h = 0.25;                 // hours per turn
        const double arrival_radius_nm = 0.03;  

        simulate_turn(ships, orders, classes, dt_h, arrival_radius_nm);

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
