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

namespace vital {

  class PeakMeter : public Processor {
    public:
      static constexpr int kMaxRememberedPeaks = 16;

      enum {
        kLevel,
        kMemoryPeak,
        kNumOutputs
      };

      PeakMeter();

      virtual Processor* clone() const override { return new PeakMeter(*this); }
      void process(int num_samples) override;

    protected:
      poly_float current_peak_;
      poly_float current_square_sum_;

      poly_float remembered_peak_;
      poly_int samples_since_remembered_;

      JUCE_LEAK_DETECTOR(PeakMeter)
  };
} // namespace vital

