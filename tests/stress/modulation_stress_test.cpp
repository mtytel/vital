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

#include "modulation_stress_test.h"
#include "synth_constants.h"
#include "synth_types.h"
#include "synth_parameters.h"
#include "modulation_connection_processor.h"

namespace {
  constexpr int kProcessAmount = 35;
  constexpr int kNumSamples = vital::kMaxBufferSize;
  constexpr float kLargeModulationAmount = 1000.0f;
  constexpr int kModulationHookupNumber = 35;
  const std::string kDefaultConnection = "osc_1_level";

  vital::modulation_change createModulationChange(vital::ModulationConnection* connection, vital::SoundEngine* engine) {
    vital::modulation_change change;
    change.source = engine->getModulationSource(connection->source_name);
    change.mono_destination = engine->getMonoModulationDestination(connection->destination_name);
    change.mono_modulation_switch = engine->getMonoModulationSwitch(connection->destination_name);
    change.destination_scale = 1.0f;
    VITAL_ASSERT(change.source != nullptr);
    VITAL_ASSERT(change.mono_destination != nullptr);
    VITAL_ASSERT(change.mono_modulation_switch != nullptr);

    change.poly_modulation_switch = engine->getPolyModulationSwitch(connection->destination_name);
    change.poly_destination = engine->getPolyModulationDestination(connection->destination_name);
    change.modulation_processor = connection->modulation_processor.get();
    return change;
  }

  void turnEverythingOn(vital::SoundEngine* engine) {
    std::map<std::string, vital::ValueDetails> parameters = vital::Parameters::lookup_.getAllDetails();
    vital::control_map controls = engine->getControls();
    for (auto& parameter : parameters) {
      String name = parameter.second.name;
      if (name.endsWith("_on") && controls.count(parameter.second.name))
        controls[parameter.second.name]->set(1.0f);
    }
  }
} // namespace

void ModulationStressTest::processAndCheckFinite(vital::Processor* processor) {
  processor->setSampleRate(processor->getSampleRate());

  int num_outputs = processor->numOutputs();
  for (int i = 0; i < kProcessAmount; ++i)
    processor->process(kNumSamples);

  for (int i = 0; i < num_outputs; ++i) {
    vital::Output* output = processor->output(i);
    expect(vital::utils::isFinite(output->buffer, output->buffer_size));
  }
}

void ModulationStressTest::allModulations() {
  beginTest("All Modulations");

  vital::SoundEngine engine;
  engine.noteOn(60, 1.0f, 0, 0);
  processAndCheckFinite(&engine);
  engine.noteOn(62, 1.0f, 0, 0);
  processAndCheckFinite(&engine);
  engine.noteOn(64, 1.0f, 0, 0);
  processAndCheckFinite(&engine);

  vital::output_map& source_map = engine.getModulationSources();
  vital::input_map& mono_destination_map = engine.getMonoModulationDestinations();
  vital::input_map& poly_destination_map = engine.getMonoModulationDestinations();

  std::vector<std::string> sources;
  std::vector<std::string> destinations;
  for (auto& source_iter : source_map)
    sources.push_back(source_iter.first);

  for (auto& destination_iter : mono_destination_map)
    destinations.push_back(destination_iter.first);

  for (auto& destination_iter : poly_destination_map) {
    if (!mono_destination_map.count(destination_iter.first))
      destinations.push_back(destination_iter.first);
  }

  std::vector<vital::ModulationConnection*> connections;
  turnEverythingOn(&engine);
  vital::ModulationConnectionBank& modulation_bank = engine.getModulationBank();

  int sources_size = static_cast<int>(sources.size());
  int destinations_size = static_cast<int>(destinations.size());

  for (int i = 0; i < kModulationHookupNumber; ++i) {
    int source_start = (i * (sources_size - vital::kMaxModulationConnections)) / kModulationHookupNumber;
    int dest_start = (i * (destinations_size - vital::kMaxModulationConnections)) / kModulationHookupNumber;
    for (int m = 0; m < vital::kMaxModulationConnections; ++m) {
      int source_index = vital::utils::iclamp(source_start + m, 0, sources_size - 1);
      int destination_index = vital::utils::iclamp(dest_start + m, 0, destinations_size - 1);
      std::string source = sources[source_index];
      std::string destination = destinations[destination_index];
      vital::ModulationConnection* connection = modulation_bank.createConnection(source, destination);
      if (connection == nullptr)
        connection = modulation_bank.createConnection(source, kDefaultConnection);

      connection->modulation_processor->setBaseValue(kLargeModulationAmount * (rand() % 2 ? 1.0f : -1.0f));
      expect(connection != nullptr);
      connections.push_back(connection);

      vital::modulation_change change = createModulationChange(connection, &engine);
      change.disconnecting = false;
      engine.connectModulation(change);
    }

    processAndCheckFinite(&engine);

    for (vital::ModulationConnection* connection : connections) {
      vital::modulation_change change = createModulationChange(connection, &engine);
      change.disconnecting = true;
      engine.disconnectModulation(change);
      connection->source_name = "";
      connection->destination_name = "";
    }
    connections.clear();
    processAndCheckFinite(&engine);
  }
}

void ModulationStressTest::randomModulations() {
  beginTest("Random Modulations");

  vital::SoundEngine engine;
  engine.noteOn(60, 1.0f, 0, 0);
  processAndCheckFinite(&engine);
  engine.noteOn(62, 1.0f, 0, 0);
  processAndCheckFinite(&engine);
  engine.noteOn(64, 1.0f, 0, 0);
  processAndCheckFinite(&engine);

  vital::output_map& source_map = engine.getModulationSources();
  vital::input_map& mono_destination_map = engine.getMonoModulationDestinations();
  vital::input_map& poly_destination_map = engine.getMonoModulationDestinations();

  std::vector<std::string> sources;
  std::vector<std::string> destinations;
  for (auto& source_iter : source_map)
    sources.push_back(source_iter.first);

  for (auto& destination_iter : mono_destination_map)
    destinations.push_back(destination_iter.first);

  for (auto& destination_iter : poly_destination_map) {
    if (!mono_destination_map.count(destination_iter.first))
      destinations.push_back(destination_iter.first);
  }

  std::vector<vital::ModulationConnection*> connections;
  turnEverythingOn(&engine);
  vital::ModulationConnectionBank& modulation_bank = engine.getModulationBank();

  int sources_size = static_cast<int>(sources.size());
  int destinations_size = static_cast<int>(destinations.size());
  for (int i = 0; i < kModulationHookupNumber; ++i) {
    DBG("");

    int source_start = (i * (sources_size - vital::kMaxModulationConnections / 2)) / kModulationHookupNumber;
    int dest_start = (i * (destinations_size - vital::kMaxModulationConnections / 2)) / kModulationHookupNumber;
    for (int m = 0; m < vital::kMaxModulationConnections / 2; ++m) {
      int source_index = vital::utils::iclamp(source_start + m, 0, sources_size - 1);
      int destination_index = vital::utils::iclamp(dest_start + m, 0, destinations_size - 1);
      std::string source = sources[source_index];
      std::string destination = destinations[destination_index];
      vital::ModulationConnection* connection = modulation_bank.createConnection(source, destination);
      if (connection == nullptr)
        connection = modulation_bank.createConnection(source, kDefaultConnection);

      DBG(source + " -> " + destination);
      connection->modulation_processor->setBaseValue(kLargeModulationAmount * (rand() % 2 ? 1.0f : -1.0f));
      expect(connection != nullptr);
      connections.push_back(connection);

      vital::modulation_change change = createModulationChange(connection, &engine);
      change.disconnecting = false;
      engine.connectModulation(change);
    }

    for (int m = vital::kMaxModulationConnections / 2; m < vital::kMaxModulationConnections; ++m) {
      std::string source = sources[rand() % sources.size()];
      std::string destination = destinations[rand() % destinations.size()];
      vital::ModulationConnection* connection = modulation_bank.createConnection(source, destination);
      if (connection == nullptr)
        connection = modulation_bank.createConnection(source, kDefaultConnection);

      DBG(source + " -> " + destination);

      expect(connection != nullptr);
      connections.push_back(connection);
      connection->modulation_processor->setBaseValue(kLargeModulationAmount * (rand() % 2 ? 1.0f : -1.0f));

      vital::modulation_change change = createModulationChange(connection, &engine);
      change.disconnecting = false;
      engine.connectModulation(change);
    }

    processAndCheckFinite(&engine);

    for (vital::ModulationConnection* connection : connections) {
      vital::modulation_change change = createModulationChange(connection, &engine);
      change.disconnecting = true;
      engine.disconnectModulation(change);
      connection->source_name = "";
      connection->destination_name = "";
    }
    connections.clear();
    processAndCheckFinite(&engine);
  }
}

void ModulationStressTest::runTest() {
  allModulations();
  randomModulations();
}

static ModulationStressTest modulation_stress_test;
