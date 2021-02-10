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

#include "synth_types.h"
#include "processor_router.h"

#include <climits>
#include <vector>

namespace vital {
  class ValueSwitch;
  class SynthModule;

  class StatusOutput {
    public:
      static constexpr float kClearValue = INT_MIN;

      StatusOutput(Output* source) : source_(source), value_(0.0f) { }

      force_inline poly_float value() const { return value_; }

      force_inline void update(poly_mask voice_mask) {
        poly_float masked_value = source_->buffer[0] & voice_mask;
        value_ = masked_value + utils::swapVoices(masked_value);
      }

      force_inline void update() {
        value_ = source_->buffer[0];
      }

      force_inline void clear() { value_ = kClearValue; }
      force_inline bool isClearValue(poly_float value) const { return poly_float::equal(value, kClearValue).anyMask(); }
      force_inline bool isClearValue(float value) const { return value == kClearValue; }

    private:
      Output* source_;
      poly_float value_;

      JUCE_LEAK_DETECTOR(StatusOutput)
  };

  struct ModuleData {
    std::vector<Processor*> owned_mono_processors;
    std::vector<SynthModule*> sub_modules;

    control_map controls;
    output_map mod_sources;
    std::map<std::string, std::unique_ptr<StatusOutput>> status_outputs;
    input_map mono_mod_destinations;
    input_map poly_mod_destinations;
    output_map mono_modulation_readout;
    output_map poly_modulation_readout;
    std::map<std::string, ValueSwitch*> mono_modulation_switches;
    std::map<std::string, ValueSwitch*> poly_modulation_switches;

    JUCE_LEAK_DETECTOR(ModuleData)
  };

  class SynthModule : public ProcessorRouter {
    public:
      SynthModule(int num_inputs, int num_outputs, bool control_rate = false) :
          ProcessorRouter(num_inputs, num_outputs, control_rate) {
        data_ = std::make_shared<ModuleData>();
      }
      virtual ~SynthModule() { }

      // Returns a map of all controls of this module and all submodules.
      control_map getControls();

      Output* getModulationSource(std::string name);
      const StatusOutput* getStatusOutput(std::string name) const;
      Processor* getModulationDestination(std::string name, bool poly);
      Processor* getMonoModulationDestination(std::string name);
      Processor* getPolyModulationDestination(std::string name);

      ValueSwitch* getModulationSwitch(std::string name, bool poly);
      ValueSwitch* getMonoModulationSwitch(std::string name);
      ValueSwitch* getPolyModulationSwitch(std::string name);
      void updateAllModulationSwitches();

      output_map& getModulationSources();
      input_map& getMonoModulationDestinations();
      input_map& getPolyModulationDestinations();
      virtual output_map& getMonoModulations();
      virtual output_map& getPolyModulations();
      virtual void correctToTime(double seconds) { }
      void enableOwnedProcessors(bool enable);
      virtual void enable(bool enable) override;
      void addMonoProcessor(Processor* processor, bool own = true);
      void addIdleMonoProcessor(Processor* processor);

      virtual Processor* clone() const override { return new SynthModule(*this); }
      void addSubmodule(SynthModule* module) { data_->sub_modules.push_back(module); }

    protected:
      // Creates a basic linear non-scaled control.
      Value* createBaseControl(std::string name, bool audio_rate = false, bool smooth_value = false);

      // Creates a basic non-scaled linear control that you can modulate monophonically
      Output* createBaseModControl(std::string name, bool audio_rate = false, bool smooth_value = false,
                                   Output* internal_modulation = nullptr);

      // Creates any control that you can modulate monophonically.
      Output* createMonoModControl(std::string name, bool audio_rate = false, bool smooth_value = false,
                                   Output* internal_modulation = nullptr);

      // Creates any control that you can modulate polyphonically and monophonically.
      Output* createPolyModControl(std::string name, bool audio_rate = false, bool smooth_value = false,
                                   Output* internal_modulation = nullptr, Input* reset = nullptr);

      // Creates a switch from free running frequencies to tempo synced frequencies.
      Output* createTempoSyncSwitch(std::string name, Processor* frequency,
                                    const Output* beats_per_second, bool poly, Input* midi = nullptr);

      void createStatusOutput(std::string name, Output* source);

      std::shared_ptr<ModuleData> data_;

      JUCE_LEAK_DETECTOR(SynthModule)
  };
} // namespace vital

