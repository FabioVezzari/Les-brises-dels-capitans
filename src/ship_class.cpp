#include "ship_class.hpp"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>
using json = nlohmann::json;

void ShipClassRegistry::load_from_file(const std::string& path) {
    std::ifstream in(path);
    if (!in) {
        throw std::runtime_error("ShipClassRegistry: cannot open file: " + path);
    }

    json j = json::parse(in);

    if (!j.is_array()) {
        throw std::runtime_error("ShipClassRegistry: expected JSON array at root");
    }

    classes_.clear();

    for (const auto& item : j) {
        if (!item.is_object()) {
            throw std::runtime_error("ShipClassRegistry: each entry must be an object");
        }

        if (!item.contains("id") || !item["id"].is_string()) {
            throw std::runtime_error("ShipClassRegistry: missing/invalid 'id' (string)");
        }
        if (!item.contains("speed_kn") || !item["speed_kn"].is_number()) {
            throw std::runtime_error("ShipClassRegistry: missing/invalid 'speed_kn' (number)");
        }

        ShipClass sc;
        sc.id = item["id"].get<std::string>();
        sc.speed_kn = item["speed_kn"].get<double>();

        if (sc.id.empty()) {
            throw std::runtime_error("ShipClassRegistry: 'id' cannot be empty");
        }
        if (sc.speed_kn <= 0.0) {
            throw std::runtime_error("ShipClassRegistry: 'speed_kn' must be > 0 for id=" + sc.id);
        }

        auto [it, inserted] = classes_.emplace(sc.id, sc);
        if (!inserted) {
            throw std::runtime_error("ShipClassRegistry: duplicate class id: " + sc.id);
        }
    }
}

const ShipClass* ShipClassRegistry::find(const std::string& class_id) const noexcept {
    auto it = classes_.find(class_id);
    if (it == classes_.end()) return nullptr;
    return &it->second;
}
