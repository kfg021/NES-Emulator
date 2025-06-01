#ifndef SAVESTATE_HPP
#define SAVESTATE_HPP

#include "core/bus.hpp"

#include <QString>

bool createSaveState(const QString& filePath, const Bus& bus);
bool loadSaveState(const QString& filePath, Bus& bus);

#endif // SAVESTATE_HPP