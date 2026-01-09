#pragma once
#include <string>
#include <vector>
#include "ship.hpp"

std::vector<Ship> load_game_state(const std::string& path);
void save_game_state(const std::string& path, const std::vector<Ship>& ships);
