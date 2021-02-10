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
 * GNU General Public License for more details.aaa
 *
 * You should have received a copy of the GNU General Public License
 * along with vital.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "processor.h"
#include "synth_filter.h"

#include "futils.h"
#include "one_pole_filter.h"

namespace vital {

  class DirtyFilter : public Processor, public SynthFilter {
    public:
      static constexpr mono_float kMinResonance = 0.1f;
      static constexpr mono_float kMaxResonance = 2.15f;
      static constexpr mono_float kSaturationBoost = 1.4f;
      static constexpr mono_float kMaxVisibleResonance = 2.0f;
      static constexpr mono_float kDriveResonanceBoost = 0.05f;

      static constexpr mono_float kMinCutoff = 1.0f;
      static constexpr mono_float kMinDrive = 0.1f;

      static constexpr mono_float kFlatResonance = 1.0f;

      force_inline poly_float tuneResonance(poly_float resonance, poly_float coefficient) {
        return resonance / utils::max(1.0f, coefficient * 0.25f + 0.97f);
      }

      DirtyFilter();
      virtual ~DirtyFilter() { }

      virtual Processor* clone() const override { return new DirtyFilter(*this); }
      virtual void process(int num_samples) override;

      void setupFilter(const FilterState& filter_state) override;

      void process12(int num_samples, poly_float current_resonance,
                     poly_float current_drive, poly_float current_drive_boost, poly_float current_drive_blend,
                     poly_float current_low, poly_float current_band, poly_float current_high);

      void process24(int num_samples, poly_float current_resonance,
                     poly_float current_drive, poly_float current_drive_boost, poly_float current_drive_blend,
                     poly_float current_low, poly_float current_band, poly_float current_high);

      void processDual(int num_samples, poly_float current_resonance,
                       poly_float current_drive, poly_float current_drive_boost,
                       poly_float current_drive_blend, poly_float current_drive_mult,
                       poly_float current_low, poly_float current_high);

      force_inline poly_float tick24(poly_float audio_in,
                                     poly_float coefficient, poly_float resonance,
                                     poly_float drive, poly_float feed_mult, poly_float normalizer,
                                     poly_float pre_feedback_mult, poly_float pre_normalizer,
                                     poly_float low, poly_float band, poly_float high);

      force_inline poly_float tickDual(poly_float audio_in,
                                       poly_float coefficient, poly_float resonance,
                                       poly_float drive, poly_float feed_mult, poly_float normalizer,
                                       poly_float pre_feedback_mult, poly_float pre_normalizer,
                                       poly_float low, poly_float high);

      force_inline poly_float tick(poly_float audio_in,
                                   poly_float coefficient, poly_float resonance,
                                   poly_float drive, poly_float feed_mult, poly_float normalizer,
                                   poly_float low, poly_float band, poly_float high);

      void reset(poly_mask reset_mask) override;
      void hardReset() override;

      force_inline poly_float getResonance() {
        poly_float resonance_in = utils::clamp(tuneResonance(resonance_, coefficient_ * 2.0f), 0.0f, 1.0f);
        return utils::interpolate(kMinResonance, kMaxResonance, resonance_in) + drive_boost_;
      }
      force_inline poly_float getDrive() {
        poly_float resonance = getResonance();
        poly_float scaled_drive = utils::max(poly_float(kMinDrive), drive_) / (resonance * resonance * 0.5f + 1.0f);
        return utils::interpolate(drive_, scaled_drive, drive_blend_);
      }
      force_inline poly_float getLowAmount() { return low_pass_amount_; }
      force_inline poly_float getBandAmount() { return band_pass_amount_; }
      force_inline poly_float getHighAmount() { return high_pass_amount_; }
      force_inline poly_float getLowAmount24(int style) {
        if (style == kDualNotchBand)
          return high_pass_amount_;
        return low_pass_amount_;
      }
      force_inline poly_float getHighAmount24(int style) {
        if (style == kDualNotchBand)
          return low_pass_amount_;
        return high_pass_amount_;
      }

    private:
      poly_float coefficient_, resonance_;
      poly_float drive_, drive_boost_, drive_blend_, drive_mult_;

      poly_float low_pass_amount_;
      poly_float band_pass_amount_;
      poly_float high_pass_amount_;

      OnePoleFilter<> pre_stage1_;
      OnePoleFilter<> pre_stage2_;
      OnePoleFilter<> stage1_;
      OnePoleFilter<> stage2_;
      OnePoleFilter<futils::quickTanh> stage3_;
      OnePoleFilter<futils::quickTanh> stage4_;

      JUCE_LEAK_DETECTOR(DirtyFilter)
  };
} // namespace vital

