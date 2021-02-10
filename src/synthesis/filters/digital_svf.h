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

namespace vital {

  class DigitalSvf : public Processor, public SynthFilter {
    public:
      static constexpr mono_float kDefaultMinResonance = 0.5f;
      static constexpr mono_float kDefaultMaxResonance = 16.0f;
      static constexpr mono_float kMinCutoff = 1.0f;
      static constexpr mono_float kMaxGain = 15.0f;
      static constexpr mono_float kMinGain = -15.0f;

      static force_inline mono_float computeSvfOnePoleFilterCoefficient(mono_float frequency_ratio) {
        static constexpr float kMaxRatio = 0.499f;
        return std::tan(std::min(kMaxRatio, frequency_ratio) * vital::kPi);
      }

      typedef OneDimLookup<computeSvfOnePoleFilterCoefficient, 2048> SvfCoefficientLookup;
      static const SvfCoefficientLookup svf_coefficient_lookup_;
      static const SvfCoefficientLookup* getSvfCoefficientLookup() { return &svf_coefficient_lookup_; }

      struct FilterValues {
        poly_float v0, v1, v2;

        void hardReset() {
          v0 = 0.0f;
          v1 = 0.0f;
          v2 = 0.0f;
        }

        void reset(poly_mask reset_mask, const FilterValues& other) {
          v0 = utils::maskLoad(v0, other.v0, reset_mask);
          v1 = utils::maskLoad(v1, other.v1, reset_mask);
          v2 = utils::maskLoad(v2, other.v2, reset_mask);
        }

        FilterValues getDelta(const FilterValues& target, mono_float increment) {
          FilterValues result;
          result.v0 = (target.v0 - v0) * increment;
          result.v1 = (target.v1 - v1) * increment;
          result.v2 = (target.v2 - v2) * increment;
          return result;
        }

        force_inline void increment(const FilterValues& delta) {
          v0 += delta.v0;
          v1 += delta.v1;
          v2 += delta.v2;
        }
      };

      DigitalSvf();
      virtual ~DigitalSvf() { }

      virtual Processor* clone() const override { return new DigitalSvf(*this); }
      virtual void process(int num_samples) override;
      void processWithInput(const poly_float* audio_in, int num_samples) override;
      void reset(poly_mask reset_masks) override;
      void hardReset() override;

      void setupFilter(const FilterState& filter_state) override;
      void setResonanceBounds(mono_float min, mono_float max);

      void process12(const poly_float* audio_in, int num_samples,
                     poly_float current_resonance, poly_float current_drive,
                     poly_float current_post_multiply, FilterValues& blends);

      void processBasic12(const poly_float* audio_in, int num_samples,
                          poly_float current_resonance, poly_float current_drive,
                          poly_float current_post_multiply, FilterValues& blends);

      void process24(const poly_float* audio_in, int num_samples,
                     poly_float current_resonance, poly_float current_drive,
                     poly_float current_post_multiply, FilterValues& blends);

      void processBasic24(const poly_float* audio_in, int num_samples,
                          poly_float current_resonance, poly_float current_drive,
                          poly_float current_post_multiply, FilterValues& blends);

      void processDual(const poly_float* audio_in, int num_samples,
                       poly_float current_resonance, poly_float current_drive,
                       poly_float current_post_multiply,
                       FilterValues& blends1, FilterValues& blends2);

      force_inline poly_float tick(poly_float audio_in, poly_float drive,
                                   poly_float resonance, poly_float coefficient, FilterValues& blends);

      force_inline poly_float tickBasic(poly_float audio_in, poly_float coefficient,
                                        poly_float resonance, poly_float drive, FilterValues& blends);

      force_inline poly_float tick24(poly_float audio_in, poly_float coefficient,
                                     poly_float resonance, poly_float drive, FilterValues& blends);

      force_inline poly_float tickBasic24(poly_float audio_in, poly_float coefficient,
                                          poly_float resonance, poly_float drive, FilterValues& blends);

      force_inline poly_float tickDual(poly_float audio_in, poly_float coefficient,
                                       poly_float resonance, poly_float drive,
                                       FilterValues& blends1, FilterValues& blends2);

      poly_float getDrive() const { return drive_ * post_multiply_; }
      poly_float getMidiCutoff() const { return midi_cutoff_; }
      poly_float getResonance() const { return resonance_; }
      poly_float getLowAmount() const { return low_amount_; }
      poly_float getBandAmount() const { return band_amount_; }
      poly_float getHighAmount() const { return high_amount_; }

      poly_float getLowAmount24(int style) const {
        if (style == kDualNotchBand)
          return high_amount_;
        return low_amount_;
      }

      poly_float getHighAmount24(int style) const {
        if (style == kDualNotchBand)
          return low_amount_;
        return high_amount_;
      }

      void setBasic(bool basic) { basic_ = basic; }
      void setDriveCompensation(bool drive_compensation) { drive_compensation_ = drive_compensation; }

    private:
      poly_float midi_cutoff_, resonance_;
      FilterValues blends1_;
      FilterValues blends2_;
      poly_float drive_, post_multiply_;

      poly_float low_amount_, band_amount_, high_amount_;

      poly_float ic1eq_pre_, ic2eq_pre_;
      poly_float ic1eq_, ic2eq_;

      mono_float min_resonance_, max_resonance_;

      bool basic_;
      bool drive_compensation_;

      JUCE_LEAK_DETECTOR(DigitalSvf)
  };
} // namespace vital

