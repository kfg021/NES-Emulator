#ifndef SAVESTATE_HPP
#define SAVESTATE_HPP

#include "core/bus.hpp"

#include <string>

bool createSaveState(const std::string& filePath, const Bus& bus);

bool loadSaveState(const std::string& filePath, Bus& bus);

#endif // SAVESTATE_HPP