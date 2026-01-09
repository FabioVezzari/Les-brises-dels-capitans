#include "sim.hpp"
#include "geo.hpp"

#include <algorithm>
#include <stdexcept>

void simulate_turn(std::vector<Ship>& ships,
                   OrdersByShip& orders,
                   const ShipClassRegistry& classes,
                   const std::unordered_map<std::string, Wind>& wind_by_ship,
                   double dt_h,
                   double arrival_radius_nm)
{
    if (dt_h <= 0.0) return;
    if (arrival_radius_nm < 0.0) arrival_radius_nm = 0.0;

    for (auto& s : ships) {
        const auto* cls = classes.find(s.class_id);
        if (!cls) {
            throw std::runtime_error(
                "Unknown class_id for ship " + s.id + ": " + s.class_id
            );
        }

        // No wind -> cannot move
        auto w_it = wind_by_ship.find(s.id);
        if (w_it == wind_by_ship.end()) continue;
        const Wind& wind = w_it->second;

        auto it = orders.find(s.id);
        if (it == orders.end()) continue;

        Route& route = it->second;
        if (route.empty()) continue;

        // Target waypoint
        const GeoPoint target = route.front();

        // Heading toward waypoint
        const double heading_deg = bearing_deg(s.pos, target);

        // True Wind Angle
        const double twa_deg = angle_diff_0_180(wind.from_deg, heading_deg);

        // Speed from polar
        const double speed_kn =
            cls->speed_from_polar_kn(wind.speed_kn, twa_deg);

        double remaining_nm = speed_kn * dt_h;
        if (remaining_nm <= 0.0) continue;

        // ----------------------------------------------------
        // Move toward waypoint(s)
        // ----------------------------------------------------
        while (remaining_nm > 0.0 && !route.empty()) {
            const GeoPoint wp = route.front();
            const double d_nm = distance_nm(s.pos, wp);

            if (d_nm <= arrival_radius_nm) {
                route.erase(route.begin());
                continue;
            }

            const double step_nm = std::min(remaining_nm, d_nm);
            s.pos = move_toward_nm(s.pos, wp, step_nm);
            remaining_nm -= step_nm;

            if (distance_nm(s.pos, wp) <= arrival_radius_nm) {
                route.erase(route.begin());
            }
        }

        if (route.empty()) {
            orders.erase(s.id);
        }
    }
}
