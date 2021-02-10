/* Copyright 2013-2019 Matt Tytel
 *
 * vital is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * vital is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "synth_types.h"

#include "synth_constants.h"
#include "modulation_connection_processor.h"

namespace vital {
  namespace {
    const std::string kModulationSourceDelimiter = "_";
    const std::set<std::string> kBipolarModulationSourcePrefixes = {
      "lfo",
      "stereo",
      "random",
      "pitch"
    };

    force_inline bool isConnectionAvailable(ModulationConnection* connection) {
      return connection->source_name.empty() && connection->destination_name.empty();
    }
  }

  ModulationConnection::ModulationConnection(int index, std::string from, std::string to) :
      source_name(std::move(from)), destination_name(std::move(to)) {
    modulation_processor = std::make_unique<ModulationConnectionProcessor>(index);
  }

  ModulationConnection::~ModulationConnection() { }

  bool ModulationConnection::isModulationSourceDefaultBipolar(const std::string& source) {
    std::size_t pos = source.find(kModulationSourceDelimiter);
    std::string prefix = source.substr(0, pos);
    return kBipolarModulationSourcePrefixes.count(prefix) > 0;
  }

  ModulationConnectionBank::ModulationConnectionBank() {
    for (int i = 0; i < kMaxModulationConnections; ++i) {
      std::unique_ptr<ModulationConnection> connection = std::make_unique<ModulationConnection>(i);
      all_connections_.push_back(std::move(connection));
    }
  }

  ModulationConnectionBank::~ModulationConnectionBank() { }

  ModulationConnection* ModulationConnectionBank::createConnection(const std::string& from, const std::string& to) {
    int index = 1;
    for (auto& connection : all_connections_) {
      std::string invalid_connection = "modulation_" + std::to_string(index++) + "_amount";
      if (to != invalid_connection && isConnectionAvailable(connection.get())) {
        connection->resetConnection(from, to);
        connection->modulation_processor->setBipolar(ModulationConnection::isModulationSourceDefaultBipolar(from));
        return connection.get();
      }
    }

    return nullptr;
  }
} // namespace vital
