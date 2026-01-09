#pragma once

#include <string>
#include <vector>
#include <unordered_map>

#include "geo.hpp"

// One route = ordered list of waypoints
using Route = std::vector<GeoPoint>;

// Orders = map ship_id -> route
using std::unordered_map;
using OrdersByShip = std::unordered_map<std::string, Route>;

// Load / save GeoJSON FeatureCollection where each ship has a LineString route
OrdersByShip load_orders_geojson(const std::string& path);

void save_orders_geojson(const std::string& path,
                         const OrdersByShip& orders);
