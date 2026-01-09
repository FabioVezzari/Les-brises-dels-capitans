#include "orders.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

// ------------------------------------------------------------
// Load navigation orders from a GeoJSON FeatureCollection.
// Each Feature must have:
// - properties.ship_id (string)
// - geometry.type = LineString
// - geometry.coordinates = [ [lon, lat], ... ]
// ------------------------------------------------------------
OrdersByShip load_orders_geojson(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("Cannot open orders file: " + path);
    }

    json root = json::parse(in);

    if (root.value("type", "") != "FeatureCollection") {
        throw std::runtime_error("Orders: expected FeatureCollection");
    }

    if (!root.contains("features") || !root["features"].is_array()) {
        throw std::runtime_error("Orders: expected 'features' array");
    }

    OrdersByShip orders;

    for (const auto& f : root["features"]) {
        if (!f.contains("properties") || !f.contains("geometry")) {
            throw std::runtime_error("Orders: feature missing properties or geometry");
        }

        const auto& props = f.at("properties");
        const auto& geom  = f.at("geometry");

        if (!props.contains("ship_id") || !props["ship_id"].is_string()) {
            throw std::runtime_error("Orders: feature missing string property 'ship_id'");
        }

        const std::string ship_id = props["ship_id"].get<std::string>();
        if (ship_id.empty()) {
            throw std::runtime_error("Orders: ship_id cannot be empty");
        }

        if (geom.value("type", "") != "LineString") {
            throw std::runtime_error(
                "Orders: geometry for ship_id=" + ship_id + " must be LineString");
        }

        if (!geom.contains("coordinates") || !geom["coordinates"].is_array()) {
            throw std::runtime_error(
                "Orders: LineString coordinates missing for ship_id=" + ship_id);
        }

        const auto& coords = geom["coordinates"];
        Route route;
        route.reserve(coords.size());

        for (const auto& c : coords) {
            if (!c.is_array() || c.size() != 2) {
                throw std::runtime_error(
                    "Orders: coordinate must be [lon, lat] for ship_id=" + ship_id);
            }

            GeoPoint p;
            p.lon_deg = c[0].get<double>();
            p.lat_deg = c[1].get<double>();
            route.push_back(p);
        }

        orders.emplace(ship_id, std::move(route));
    }

    return orders;
}

// ------------------------------------------------------------
// Save navigation orders to a GeoJSON FeatureCollection.
// One Feature per ship, with a LineString route.
// ------------------------------------------------------------
void save_orders_geojson(const std::string& path,
                         const OrdersByShip& orders) {
    json root;
    root["type"] = "FeatureCollection";
    root["features"] = json::array();

    for (const auto& [ship_id, route] : orders) {
        json feature;
        feature["type"] = "Feature";
        feature["properties"] = { {"ship_id", ship_id} };

        json coords = json::array();
        for (const auto& p : route) {
            coords.push_back({ p.lon_deg, p.lat_deg });
        }

        feature["geometry"] = {
            {"type", "LineString"},
            {"coordinates", coords}
        };

        root["features"].push_back(std::move(feature));
    }

    std::ofstream out(path);
    if (!out) {
        throw std::runtime_error("Cannot write orders file: " + path);
    }

    out << root.dump(2) << "\n";
}
