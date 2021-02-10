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

#include "synth_constants.h"

namespace vital {

  class StereoMemory;

  class ReorderableEffectChain : public SynthModule {
    public:
      enum {
        kAudio,
        kOrder,
        kNumInputs
      };

      ReorderableEffectChain(const Output* beats_per_second, const Output* keytrack);

      virtual void process(int num_samples) override;
      virtual void hardReset() override;
      virtual void processWithInput(const poly_float* audio_in, int num_samples) override;
      virtual Processor* clone() const override { return new ReorderableEffectChain(*this); }

      virtual void correctToTime(double seconds) override;

      SynthModule* getEffect(constants::Effect effect) { return effects_[effect]; }
      const StereoMemory* getEqualizerMemory() { return equalizer_memory_; }

    protected:
      SynthModule* createEffectModule(int index);

      const StereoMemory* equalizer_memory_;
      const Output* beats_per_second_;
      const Output* keytrack_;
      SynthModule* effects_[constants::kNumEffects];
      Value* effects_on_[constants::kNumEffects];
      int effect_order_[constants::kNumEffects];
      float last_order_;

      JUCE_LEAK_DETECTOR(ReorderableEffectChain)
  };
} // namespace vital

