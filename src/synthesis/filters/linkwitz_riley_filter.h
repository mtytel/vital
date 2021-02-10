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
#include "synth_constants.h"

namespace vital {

  class LinkwitzRileyFilter : public Processor {
    public:
      enum {
        kAudio,
        kNumInputs
      };

      enum {
        kAudioLow,
        kAudioHigh,
        kNumOutputs
      };

      LinkwitzRileyFilter(mono_float cutoff);
      virtual ~LinkwitzRileyFilter() { }

      virtual Processor* clone() const override { return new LinkwitzRileyFilter(*this); }
      virtual void process(int num_samples) override;
      void processWithInput(const poly_float* audio_in, int num_samples) override;

      void computeCoefficients();
      void setSampleRate(int sample_rate) override;
      void setOversampleAmount(int oversample_amount) override;
      void reset(poly_mask reset_mask) override;

    private:
      mono_float cutoff_;
      
      // Coefficients.
      mono_float low_in_0_, low_in_1_, low_in_2_;
      mono_float low_out_1_, low_out_2_;
      mono_float high_in_0_, high_in_1_, high_in_2_;
      mono_float high_out_1_, high_out_2_;

      // Past input and output values.
      poly_float past_in_1a_[kNumOutputs], past_in_2a_[2 * kNumOutputs];
      poly_float past_out_1a_[2 * kNumOutputs], past_out_2a_[2 * kNumOutputs];
      poly_float past_in_1b_[kNumOutputs], past_in_2b_[2 * kNumOutputs];
      poly_float past_out_1b_[2 * kNumOutputs], past_out_2b_[2 * kNumOutputs];

      JUCE_LEAK_DETECTOR(LinkwitzRileyFilter)
  };
} // namespace vital

