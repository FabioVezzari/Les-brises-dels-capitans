#pragma once
#include <string>
#include <unordered_map>
#include <vector>
#include <optional>

struct PolarEntry {
    double tws_kn{0.0};
    double twa_deg{0.0};
    double max_speed_kn{0.0};
};


struct ShipClass {
    std::string id;
    double speed_kn{0.0};                 // fallback speed (nm/h)
    std::vector<PolarEntry> polar;        // optional polar table

    // Returns boat speed (kn) from polar using bilinear interpolation
    // and linear extrapolation outside range.
    // Assumes symmetric polar: twa folded to [0,180].
    double speed_from_polar_kn(double tws_kn, double twa_deg) const;

};

class ShipClassRegistry {
public:
    void load_from_file(const std::string& path);
    const ShipClass* find(const std::string& id) const;

private:
    std::unordered_map<std::string, ShipClass> classes_;
};
