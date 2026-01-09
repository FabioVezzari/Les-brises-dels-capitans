#pragma once
#include <string>
#include <unordered_map>

struct ShipClass {
    std::string id;     // e.g. "sloop"
    double speed_kn;   // constant scalar speed
};

class ShipClassRegistry {
public:
    // Load ship classes from a JSON file (throws on errors)
    void load_from_file(const std::string& path);

    // Returns nullptr if not found
    const ShipClass* find(const std::string& class_id) const noexcept;

    // Optional: how many classes loaded
    std::size_t size() const noexcept { return classes_.size(); }

private:
    std::unordered_map<std::string, ShipClass> classes_;
};
