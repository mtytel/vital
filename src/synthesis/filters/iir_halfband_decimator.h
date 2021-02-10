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

  class IirHalfbandDecimator : public Processor {
    public:
      static constexpr int kNumTaps9 = 2;
      static constexpr int kNumTaps25 = 6;
      static poly_float kTaps9[kNumTaps9];
      static poly_float kTaps25[kNumTaps25];

      enum {
        kAudio,
        kNumInputs
      };

      IirHalfbandDecimator();
      virtual ~IirHalfbandDecimator() { }

      virtual Processor* clone() const override { VITAL_ASSERT(false); return nullptr; }

      virtual void process(int num_samples) override;
      void reset(poly_mask reset_mask) override;
      force_inline void setSharpCutoff(bool sharp_cutoff) { sharp_cutoff_ = sharp_cutoff; }

    private:
      bool sharp_cutoff_;
      poly_float in_memory_[kNumTaps25];
      poly_float out_memory_[kNumTaps25];

      JUCE_LEAK_DETECTOR(IirHalfbandDecimator)
  };
} // namespace vital

