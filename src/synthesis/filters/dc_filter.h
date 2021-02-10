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

  class DcFilter : public Processor {
    public:
      static constexpr mono_float kCoefficientToSrConstant = 1.0f;

      enum {
        kAudio,
        kReset,
        kNumInputs
      };

      DcFilter();
      virtual ~DcFilter() { }

      virtual Processor* clone() const override { return new DcFilter(*this); }
      virtual void process(int num_samples) override;
      virtual void processWithInput(const poly_float* audio_in, int num_samples) override;

      void setSampleRate(int sample_rate) override;
      void tick(const poly_float& audio_in, poly_float& audio_out);

    private:
      void reset(poly_mask reset_mask) override;

      mono_float coefficient_;

      // Past input and output values.
      poly_float past_in_;
      poly_float past_out_;

      JUCE_LEAK_DETECTOR(DcFilter)
  };
} // namespace vital

