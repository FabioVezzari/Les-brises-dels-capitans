#pragma once
#include <string>
#include "geo.hpp"

struct Ship {
    std::string id;
    std::string name;
    std::string allegiance;
    std::string colors;
    std::string class_id;

    GeoPoint pos;   // lon/lat (deg)
};
