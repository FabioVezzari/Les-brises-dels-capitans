#pragma once

#include <vector>
#include "ship.hpp"
#include "orders.hpp"
#include "ship_class.hpp"

// Simulate one turn (tick) of duration dt_h (hours).
// Units:
// - speed: knots (nm/h)
// - distance: nautical miles (nm)
// - dt_h: hours
//
// Behavior:
// - For each ship, consume up to (speed_kn * dt_h) nm along its route.
// - Waypoints reached (<= arrival_radius_nm) are removed from the route.
// - No wp_index is stored anywhere.
void simulate_turn(std::vector<Ship>& ships,
                   OrdersByShip& orders,                 // NOT const: we consume waypoints
                   const ShipClassRegistry& classes,
                   double dt_h,                          // hours
                   double arrival_radius_nm = 0.1);       // nm
