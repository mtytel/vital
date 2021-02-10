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

  class FlangerModule : public SynthModule {
    public:
      static constexpr mono_float kMaxFlangerSemitoneOffset = 24.0f;
      static constexpr mono_float kFlangerDelayRange = 0.01f;
      static constexpr mono_float kFlangerCenter = kFlangerDelayRange * 0.5f + 0.0005f;
      static constexpr mono_float kModulationDelayBuffer = 0.0005f;

      enum {
        kAudioOutput,
        kFrequencyOutput,
        kNumOutputs
      };

      FlangerModule(const Output* beats_per_second);
      virtual ~FlangerModule();

      void init() override;
      void hardReset() override { delay_->hardReset(); }
      void enable(bool enable) override {
        SynthModule::enable(enable);
        process(1);
        if (!enable)
          delay_->hardReset();
      }

      void processWithInput(const poly_float* audio_in, int num_samples) override;
      void correctToTime(double seconds) override;

      Processor* clone() const override { VITAL_ASSERT(false); return nullptr; }

    protected:
      const Output* beats_per_second_;
      Output* frequency_;
      Output* phase_offset_;
      Output* center_;
      Output* mod_depth_;
      poly_float phase_;

      cr::Value delay_frequency_;
      StereoDelay* delay_;

      JUCE_LEAK_DETECTOR(FlangerModule)
  };
} // namespace vital

