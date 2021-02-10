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
#include "delay.h"

namespace vital {
  class ChorusModule : public SynthModule {
    public:
      static constexpr mono_float kMaxChorusModulation = 0.03f;
      static constexpr mono_float kMaxChorusDelay = 0.08f;
      static constexpr int kMaxDelayPairs = 4;

      ChorusModule(const Output* beats_per_second);
      virtual ~ChorusModule() { }

      void init() override;
      void enable(bool enable) override;

      void processWithInput(const poly_float* audio_in, int num_samples) override;
      void correctToTime(double seconds) override;
      Processor* clone() const override { VITAL_ASSERT(false); return nullptr; }

      int getNextNumVoicePairs();

    protected:
      const Output* beats_per_second_;
      Value* voices_;

      int last_num_voices_;
    
      cr::Output delay_status_outputs_[kMaxDelayPairs];

      Output* frequency_;
      Output* delay_time_1_;
      Output* delay_time_2_;
      Output* mod_depth_;
      Output* wet_output_;
      poly_float phase_;
      poly_float wet_;
      poly_float dry_;

      poly_float delay_input_buffer_[kMaxBufferSize];

      cr::Value delay_frequencies_[kMaxDelayPairs];
      MultiDelay* delays_[kMaxDelayPairs];

      JUCE_LEAK_DETECTOR(ChorusModule) 
  };
} // namespace vital

