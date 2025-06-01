#ifndef SAVESTATE_HPP
#define SAVESTATE_HPP

#include "core/bus.hpp"

#include <string>

bool createSaveState(const Bus& bus);
bool loadSaveState(Bus& bus);

#endif // SAVESTATE_HPP