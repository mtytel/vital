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

#include "processor.h"
#include "synth_filter.h"

#include "one_pole_filter.h"
#include "futils.h"

namespace vital {

  class LadderFilter : public Processor, public SynthFilter {
    public:
      static constexpr int kNumStages = 4;
      static constexpr mono_float kResonanceTuning = 1.66f;

      static constexpr mono_float kMinResonance = 0.001f;
      static constexpr mono_float kMaxResonance = 4.1f;
      static constexpr mono_float kMaxCoefficient = 0.35f;
      static constexpr mono_float kDriveResonanceBoost = 5.0f;
      static constexpr mono_float kMinCutoff = 1.0f;
      static constexpr mono_float kMaxCutoff = 20000.0f;

      LadderFilter();
      virtual ~LadderFilter() { }

      virtual Processor* clone() const override { return new LadderFilter(*this); }
      virtual void process(int num_samples) override;

      void setupFilter(const FilterState& filter_state) override;

      force_inline void tick(poly_float audio_in, poly_float coefficient,
                             poly_float resonance, poly_float drive);
    
      void reset(poly_mask reset_mask) override;
      void hardReset() override;

      poly_float getDrive() { return drive_; }
      poly_float getResonance() { return resonance_; }
      poly_float getStageScale(int index) { return stage_scales_[index]; }

    private:
      void setStageScales(const FilterState& filter_state);

      poly_float resonance_;
      poly_float drive_;
      poly_float post_multiply_;
      poly_float stage_scales_[kNumStages + 1];

      OnePoleFilter<futils::algebraicSat> stages_[kNumStages];

      poly_float filter_input_;

      JUCE_LEAK_DETECTOR(LadderFilter)
  };
} // namespace vital

