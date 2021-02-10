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

#include "one_pole_filter.h"
#include "processor.h"
#include "synth_filter.h"

namespace vital {

  class SallenKeyFilter : public Processor, public SynthFilter {
    public:
      static constexpr mono_float kMinResonance = 0.0f;
      static constexpr mono_float kMaxResonance = 2.15f;
      static constexpr mono_float kDriveResonanceBoost = 1.1f;
      static constexpr mono_float kMaxVisibleResonance = 2.0f;
      static constexpr mono_float kMinCutoff = 1.0f;

      static poly_float tuneResonance(poly_float resonance, poly_float coefficient) {
        return resonance / utils::max(1.0f, coefficient * 0.09f + 0.97f);
      }

      SallenKeyFilter();
      virtual ~SallenKeyFilter() { }

      virtual Processor* clone() const override { return new SallenKeyFilter(*this); }
      void process(int num_samples) override;
      void processWithInput(const poly_float* audio_in, int num_samples) override;

      void setupFilter(const FilterState& filter_state) override;

      void process12(const poly_float* audio_in, int num_samples,
                     poly_float current_resonance,
                     poly_float current_drive, poly_float current_post_multiply, 
                     poly_float current_low, poly_float current_band, poly_float current_high);

      void process24(const poly_float* audio_in, int num_samples,
                     poly_float current_resonance,
                     poly_float current_drive, poly_float current_post_multiply,
                     poly_float current_low, poly_float current_band, poly_float current_high);

      void processDual(const poly_float* audio_in, int num_samples,
                       poly_float current_resonance,
                       poly_float current_drive, poly_float current_post_multiply, 
                       poly_float current_low, poly_float current_high);
    
      force_inline void tick(poly_float audio_in, poly_float coefficient, poly_float resonance,
                             poly_float stage1_feedback_mult, poly_float drive, poly_float normalizer);

      force_inline void tick24(poly_float audio_in, poly_float coefficient, poly_float resonance,
                               poly_float stage1_feedback_mult, poly_float drive,
                               poly_float pre_normalizer, poly_float normalizer,
                               poly_float low, poly_float band, poly_float high);
    
      void reset(poly_mask reset_mask) override;
      void hardReset() override;

      poly_float getResonance() { return resonance_; }
      poly_float getDrive() { return drive_; }
      poly_float getLowAmount() { return low_pass_amount_; }
      poly_float getBandAmount() { return band_pass_amount_; }
      poly_float getHighAmount() { return high_pass_amount_; }
      poly_float getLowAmount24(int style) {
        if (style == kDualNotchBand)
          return high_pass_amount_;
        return low_pass_amount_;
      }
      poly_float getHighAmount24(int style) {
        if (style == kDualNotchBand)
          return low_pass_amount_;
        return high_pass_amount_;
      }

    private:
      poly_float cutoff_;
      poly_float resonance_;
      poly_float drive_;
      poly_float post_multiply_;
      poly_float low_pass_amount_;
      poly_float band_pass_amount_;
      poly_float high_pass_amount_;

      poly_float stage1_input_;

      OnePoleFilter<> pre_stage1_;
      OnePoleFilter<> pre_stage2_;
      OnePoleFilter<> stage1_;
      OnePoleFilter<> stage2_;

      JUCE_LEAK_DETECTOR(SallenKeyFilter)
  };
} // namespace vital

