#include "state.hpp"
#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

std::vector<Ship> load_game_state(const std::string& path) {
    std::ifstream in(path);
    if (!in) throw std::runtime_error("Cannot open game state: " + path);

    json root = json::parse(in);
    if (root.value("type", "") != "FeatureCollection")
        throw std::runtime_error("GameState: expected FeatureCollection");

    const auto& features = root.at("features");
    if (!features.is_array()) throw std::runtime_error("GameState: features must be array");

    std::vector<Ship> ships;
    for (const auto& f : features) {
        const auto& geom = f.at("geometry");
        const auto& props = f.at("properties");

        if (geom.at("type").get<std::string>() != "Point")
            throw std::runtime_error("GameState: geometry must be Point");

        const auto& coords = geom.at("coordinates");
        if (!coords.is_array() || coords.size() != 2)
            throw std::runtime_error("GameState: Point coordinates must be [lon, lat]");

        Ship s;
        s.pos = GeoPoint{coords[0].get<double>(), coords[1].get<double>()};

        s.id = props.value("id", "");
        s.name = props.value("name", "");
        s.allegiance = props.value("allegiance", "");
        s.colors = props.value("colors", "");
        s.class_id = props.value("class_id", "");

        if (s.id.empty()) throw std::runtime_error("GameState: ship missing id");

        ships.push_back(std::move(s));
    }
    return ships;
}

void save_game_state(const std::string& path, const std::vector<Ship>& ships) {
    json root;
    root["type"] = "FeatureCollection";
    root["features"] = json::array();

    for (const auto& s : ships) {
        json f;
        f["type"] = "Feature";
        f["geometry"] = {
            {"type", "Point"},
            {"coordinates", json::array({s.pos.lon_deg, s.pos.lat_deg})}
        };
        f["properties"] = {
            {"id", s.id},
            {"name", s.name},
            {"allegiance", s.allegiance},
            {"colors", s.colors},
            {"class_id", s.class_id}
        };
        root["features"].push_back(std::move(f));
    }

    std::ofstream out(path);
    if (!out) throw std::runtime_error("Cannot write game state: " + path);
    out << root.dump(2) << "\n";
}
