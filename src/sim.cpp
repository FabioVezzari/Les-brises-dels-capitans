#include "sim.hpp"
#include "geo.hpp"

#include <algorithm>
#include <stdexcept>

void simulate_turn(std::vector<Ship>& ships,
                   OrdersByShip& orders,
                   const ShipClassRegistry& classes,
                   double dt_h,
                   double arrival_radius_nm)
{
    if (dt_h <= 0.0) return;
    if (arrival_radius_nm < 0.0) arrival_radius_nm = 0.0;

    for (auto& s : ships) {
        const auto* cls = classes.find(s.class_id);
        if (!cls) {
            throw std::runtime_error("Unknown class_id for ship " + s.id + ": " + s.class_id);
        }

        // kn = nm/h, dt_h = h  => remaining_nm = nm
        double remaining_nm = cls->speed_kn * dt_h;
        if (remaining_nm <= 0.0) continue;

        auto it = orders.find(s.id);
        if (it == orders.end()) continue;        // no orders -> do nothing

        Route& route = it->second;

        // Consume waypoints in order until time/distance is over
        while (remaining_nm > 0.0 && !route.empty()) {
            const GeoPoint target = route.front();
            const double d_nm = distance_nm(s.pos, target);

            // If already "arrived", consume this waypoint and continue
            if (d_nm <= arrival_radius_nm) {
                route.erase(route.begin());
                continue;
            }

            const double step_nm = std::min(remaining_nm, d_nm);
            s.pos = move_toward_nm(s.pos, target, step_nm);
            remaining_nm -= step_nm;

            // If we reached the waypoint after moving, consume it
            if (distance_nm(s.pos, target) <= arrival_radius_nm) {
                route.erase(route.begin());
            }
        }

        // If route is finished, remove it from orders (clean state)
        if (route.empty()) {
            orders.erase(s.id);   // usa la chiave, non l'iteratore
        }

    }
}
