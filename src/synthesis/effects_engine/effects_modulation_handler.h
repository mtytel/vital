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

#include "synth_module.h"
#include "producers_module.h"
#include "voice_handler.h"
#include "wavetable.h"
#include "synth_types.h"
#include "line_generator.h"

#include <vector>

namespace vital {
  class AudioRateEnvelope;
  class FiltersModule;
  class LegatoFilter;
  class LineMap;
  class LfoModule;
  class EnvelopeModule;
  class RandomLfoModule;
  class TriggerRandom;

  class EffectsModulationHandler : public VoiceHandler {
    public:
      EffectsModulationHandler(Output* beats_per_second);
      virtual ~EffectsModulationHandler() { }

      virtual Processor* clone() const override { VITAL_ASSERT(false); return nullptr; }

      void init() override;
      void prepareDestroy();

      void process(int num_samples) override;
      void noteOn(int note, mono_float velocity, int sample, int channel) override;
      void noteOff(int note, mono_float lift, int sample, int channel) override;
      bool shouldAccumulate(Output* output) override { return false;  }
      void correctToTime(double seconds) override;
      void disableUnnecessaryModSources();
      void disableModSource(const std::string& source);

      output_map& getPolyModulations() override;
      ModulationConnectionBank& getModulationBank() { return modulation_bank_; }
      LineGenerator* getLfoSource(int index) { return &lfo_sources_[index]; }
      Output* getDirectOutput() { return getAccumulatedOutput(sub_direct_output_->output()); }

      Output* note_retrigger() { return &note_retriggered_; }

      Output* midi_offset_output() { return midi_offset_output_; }

    private:
      void createArticulation();
      void createModulators();
      void createFilters(Output* keytrack);

      void setupPolyModulationReadouts();

      ModulationConnectionBank modulation_bank_;
      Output* beats_per_second_;

      Processor* note_from_reference_;
      Output* midi_offset_output_;
      Processor* bent_midi_;
      Processor* current_midi_note_;

      FiltersModule* filters_module_;

      LfoModule* lfos_[kNumLfos];
      EnvelopeModule* envelopes_[kNumEnvelopes];

      Output note_retriggered_;

      LineGenerator lfo_sources_[kNumLfos];

      TriggerRandom* random_;
      RandomLfoModule* random_lfos_[kNumRandomLfos];

      LineMap* note_mapping_;
      LineMap* velocity_mapping_;
      LineMap* aftertouch_mapping_;
      LineMap* slide_mapping_;
      LineMap* lift_mapping_;
      LineMap* mod_wheel_mapping_;
      LineMap* pitch_wheel_mapping_;

      cr::Value* stereo_;
      cr::Multiply* note_percentage_;

      Multiply* output_;
      Multiply* sub_direct_output_;

      output_map poly_readouts_;

      JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EffectsModulationHandler)
  };
} // namespace vital
