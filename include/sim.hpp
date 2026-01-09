#pragma once

#include <vector>
#include <unordered_map>

#include "ship.hpp"
#include "orders.hpp"
#include "ship_class.hpp"
#include "weather.hpp"

void simulate_turn(std::vector<Ship>& ships,
                   OrdersByShip& orders,
                   const ShipClassRegistry& classes,
                   const std::unordered_map<std::string, Wind>& wind_by_ship,
                   double dt_h,
                   double arrival_radius_nm);
