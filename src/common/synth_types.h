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

#pragma once

#include "circular_queue.h"
#include "synth_parameters.h"
#include "operators.h"
#include "value.h"

#include <map>
#include <string>

namespace vital {

  class ValueSwitch;
  class ModulationConnectionProcessor;

  struct ModulationConnection {
    ModulationConnection(int index) : ModulationConnection(index, "", "") { }

    ModulationConnection(int index, std::string from, std::string to);

    ~ModulationConnection();

    static bool isModulationSourceDefaultBipolar(const std::string& source);

    void resetConnection(const std::string& from, const std::string& to) {
      source_name = from;
      destination_name = to;
    }

    std::string source_name;
    std::string destination_name;
    std::unique_ptr<ModulationConnectionProcessor> modulation_processor;

    private: JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationConnection)
  };

  class ModulationConnectionBank {
    public:
      ModulationConnectionBank();
      ~ModulationConnectionBank();
      ModulationConnection* createConnection(const std::string& from, const std::string& to);

      ModulationConnection* atIndex(int index) { return all_connections_[index].get(); }
      size_t numConnections() { return all_connections_.size(); }

    private:
      std::vector<std::unique_ptr<ModulationConnection>> all_connections_;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ModulationConnectionBank)
  };

  class StringLayout {
    public:
      StringLayout() : up_key_(0), down_key_(0) { }
      std::wstring getLayout() { return layout_; }
      void setLayout(const std::wstring& layout) { layout_ = layout; }

      wchar_t getUpKey() { return up_key_; }
      void setUpKey(wchar_t up_key) { up_key_ = up_key; }

      wchar_t getDownKey() { return down_key_; }
      void setDownKey(wchar_t down_key) { down_key_ = down_key; }

    protected:
      std::wstring layout_;
      int up_key_;
      int down_key_;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(StringLayout)
  };

  typedef struct {
    Output* source;
    Processor* mono_destination;
    Processor* poly_destination;
    mono_float destination_scale;
    ValueSwitch* mono_modulation_switch;
    ValueSwitch* poly_modulation_switch;
    ModulationConnectionProcessor* modulation_processor;
    bool disconnecting;
    int num_audio_rate;
  } modulation_change;

  typedef std::map<std::string, Value*> control_map;
  typedef std::pair<Value*, mono_float> control_change;
  typedef std::map<std::string, Processor*> input_map;
  typedef std::map<std::string, Output*> output_map;
} // namespace vital

