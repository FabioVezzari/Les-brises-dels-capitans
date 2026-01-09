#include "ship_class.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <set>
#include <sstream>
#include <stdexcept>
#include <tuple>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

static double fold_twa_0_180(double twa_deg) {
    // fold to [0, 360)
    double a = std::fmod(twa_deg, 360.0);
    if (a < 0) a += 360.0;
    // symmetric: map to [0,180]
    if (a > 180.0) a = 360.0 - a;
    return a;
}

static std::vector<double> unique_sorted(const std::vector<double>& v) {
    std::set<double> s(v.begin(), v.end());
    return std::vector<double>(s.begin(), s.end());
}

// returns indices (i0,i1) and t in [0,1] for interpolation,
// or extrapolation using first/last segment.
static std::tuple<size_t, size_t, double> bracket_linear(const std::vector<double>& grid, double x) {
    if (grid.size() < 2) throw std::runtime_error("Polar grid needs at least 2 points");

    if (x <= grid.front()) {
        // extrapolate using first segment
        const double x0 = grid[0], x1 = grid[1];
        const double t = (x1 - x0 != 0.0) ? (x - x0) / (x1 - x0) : 0.0;
        return {0, 1, t};
    }
    if (x >= grid.back()) {
        // extrapolate using last segment
        const size_t n = grid.size();
        const double x0 = grid[n - 2], x1 = grid[n - 1];
        const double t = (x1 - x0 != 0.0) ? (x - x0) / (x1 - x0) : 0.0;
        return {n - 2, n - 1, t};
    }

    // inside: find upper_bound
    auto it = std::upper_bound(grid.begin(), grid.end(), x);
    const size_t i1 = (size_t)std::distance(grid.begin(), it);
    const size_t i0 = i1 - 1;

    const double x0 = grid[i0], x1 = grid[i1];
    const double t = (x1 - x0 != 0.0) ? (x - x0) / (x1 - x0) : 0.0;
    return {i0, i1, t};
}

double ShipClass::speed_from_polar_kn(double tws_kn_in, double twa_deg_in) const {
    if (polar.empty()) return speed_kn;

    const double tws_kn = tws_kn_in;
    const double twa_deg = fold_twa_0_180(twa_deg_in);

    // Build grids
    std::vector<double> tws_list, twa_list;
    tws_list.reserve(polar.size());
    twa_list.reserve(polar.size());
    for (const auto& e : polar) {
        tws_list.push_back(e.tws_kn);
        twa_list.push_back(e.twa_deg);
    }
    const auto tws_grid = unique_sorted(tws_list);
    const auto twa_grid = unique_sorted(twa_list);

    // Map lookup for exact grid points
    auto lookup = [&](double tws, double twa) -> double {
        for (const auto& e : polar) {
            if (e.tws_kn == tws && e.twa_deg == twa) return e.max_speed_kn;
        }
        std::ostringstream oss;
        oss << "Polar missing value at (tws=" << tws << ", twa=" << twa << ") for class " << id;
        throw std::runtime_error(oss.str());
    };

    // Bracket/extrapolate on both axes
    auto [iT0, iT1, tT] = bracket_linear(tws_grid, tws_kn);
    auto [iA0, iA1, tA] = bracket_linear(twa_grid, twa_deg);

    const double T0 = tws_grid[iT0], T1 = tws_grid[iT1];
    const double A0 = twa_grid[iA0], A1 = twa_grid[iA1];

    // Corner values
    const double v00 = lookup(T0, A0);
    const double v01 = lookup(T0, A1);
    const double v10 = lookup(T1, A0);
    const double v11 = lookup(T1, A1);

    // Interpolate in TWA for each TWS
    const double v0 = v00 + (v01 - v00) * tA;
    const double v1 = v10 + (v11 - v10) * tA;

    // Interpolate in TWS
    const double v = v0 + (v1 - v0) * tT;

    // Optional: don’t allow negative speeds
    return std::max(0.0, v);
}

void ShipClassRegistry::load_from_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("ShipClassRegistry: cannot open " + path);

    json root = json::parse(in);
    if (!root.is_array()) throw std::runtime_error("ShipClassRegistry: expected JSON array at root");

    classes_.clear();

    for (const auto& item : root) {
        ShipClass sc;
        sc.id = item.value("id", "");
        sc.speed_kn = item.value("speed_kn", 0.0);

        if (sc.id.empty()) throw std::runtime_error("ShipClassRegistry: ship class missing id");

        // Optional polar
        if (item.contains("polar")) {
            if (!item["polar"].is_array()) {
                throw std::runtime_error("ShipClassRegistry: 'polar' must be an array for class " + sc.id);
            }

            for (const auto& row : item["polar"]) {
                if (!row.is_object()) {
                    throw std::runtime_error("Polar entry must be an object for class " + sc.id);
                }

                PolarEntry e;
                e.tws_kn        = row.at("tws_kn").get<double>();
                e.twa_deg       = row.at("twa_deg").get<double>();
                e.max_speed_kn  = row.at("max_speed_kn").get<double>();

                sc.polar.push_back(e);
            }

        }

        classes_[sc.id] = std::move(sc);
    }
}

const ShipClass* ShipClassRegistry::find(const std::string& id) const {
    auto it = classes_.find(id);
    if (it == classes_.end()) return nullptr;
    return &it->second;
}
