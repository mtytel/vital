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

#include "memory.h"
#include "one_pole_filter.h"

namespace vital {

  class CombFilter : public Processor, public SynthFilter {
    public:
      enum FeedbackStyle {
        kComb,
        kPositiveFlange,
        kNegativeFlange,
        kNumFeedbackStyles
      };

      enum FilterStyle {
        kLowHighBlend,
        kBandSpread,
        kNumFilterStyles
      };

      static FeedbackStyle getFeedbackStyle(int style) {
        return static_cast<FeedbackStyle>(style % kNumFeedbackStyles);
      }

      static FilterStyle getFilterStyle(int style) {
        return static_cast<FilterStyle>(style / kNumFeedbackStyles);
      }

      static constexpr int kNumFilterTypes = kNumFilterStyles * kNumFeedbackStyles;
      static constexpr mono_float kBandOctaveRange = 8.0f;
      static constexpr mono_float kBandOctaveMin = 0.0f;
      static constexpr int kMinPeriod = 2;
      static constexpr mono_float kInputScale = 0.5f;
      static constexpr mono_float kMaxFeedback = 1.0f;

      CombFilter(int size = kMinPeriod);
      CombFilter(const CombFilter& other);
      virtual ~CombFilter();

      virtual Processor* clone() const override {
        return new CombFilter(*this);
      }

      void setupFilter(const FilterState& filter_state) override;

      virtual void process(int num_samples) override;

      template<poly_float(*tick)(poly_float, Memory*, OnePoleFilter<>&, OnePoleFilter<>&,
                                 poly_float, poly_float, poly_float, poly_float, poly_float, poly_float, poly_float)>
      void processFilter(int num_samples);

      void reset(poly_mask reset_mask) override;
      void hardReset() override;

      poly_float getDrive() { return scale_; }
      poly_float getResonance() { return feedback_; }
      poly_float getLowAmount() { return low_gain_; }
      poly_float getHighAmount() { return high_gain_; }
      poly_float getFilterMidiCutoff() { return filter_midi_cutoff_; }
      poly_float getFilter2MidiCutoff() { return filter2_midi_cutoff_; }

    protected:
      std::unique_ptr<Memory> memory_;

      FeedbackStyle feedback_style_;
      poly_float max_period_;
      poly_float feedback_;
      poly_float filter_coefficient_;
      poly_float filter2_coefficient_;
      poly_float low_gain_;
      poly_float high_gain_;
      poly_float scale_;

      poly_float filter_midi_cutoff_;
      poly_float filter2_midi_cutoff_;
      OnePoleFilter<> feedback_filter_;
      OnePoleFilter<> feedback_filter2_;

      JUCE_LEAK_DETECTOR(CombFilter)
  };
} // namespace vital

