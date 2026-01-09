#pragma once
#include <cmath>
#include <algorithm>

struct GeoPoint {
    double lon_deg{0.0};   // GeoJSON order: lon, lat
    double lat_deg{0.0};
};

inline double deg2rad(double d) { return d * M_PI / 180.0; }
inline double rad2deg(double r) { return r * 180.0 / M_PI; }

// Earth radius in nautical miles (mean)
static constexpr double kEarthRadiusNm = 3440.065;

// Distance between two lat/lon points in nautical miles (nm)
// Equirectangular approximation: accurate enough for short/medium distances.
inline double distance_nm(GeoPoint a, GeoPoint b) {
    const double lat0 = deg2rad(a.lat_deg);
    const double dlat = deg2rad(b.lat_deg - a.lat_deg);
    const double dlon = deg2rad(b.lon_deg - a.lon_deg);

    const double x = dlon * std::cos(lat0) * kEarthRadiusNm;
    const double y = dlat * kEarthRadiusNm;

    return std::hypot(x, y);
}

// Move from pos toward target by step_nm (<= distance_nm(pos,target))
// Returns the new position (lat/lon in degrees).
inline GeoPoint move_toward_nm(GeoPoint pos, GeoPoint target, double step_nm) {
    if (step_nm <= 0.0) return pos;

    const double lat0 = deg2rad(pos.lat_deg);
    const double dlat = deg2rad(target.lat_deg - pos.lat_deg);
    const double dlon = deg2rad(target.lon_deg - pos.lon_deg);

    const double x = dlon * std::cos(lat0) * kEarthRadiusNm;
    const double y = dlat * kEarthRadiusNm;

    const double dist = std::hypot(x, y);
    if (dist < 1e-12) return pos;

    const double step = std::min(step_nm, dist);
    const double ux = x / dist;
    const double uy = y / dist;

    const double dx_nm = ux * step;
    const double dy_nm = uy * step;

    const double dlat2 = (dy_nm / kEarthRadiusNm);
    const double dlon2 = (dx_nm / (kEarthRadiusNm * std::cos(lat0)));

    pos.lat_deg += rad2deg(dlat2);
    pos.lon_deg += rad2deg(dlon2);
    return pos;
}
