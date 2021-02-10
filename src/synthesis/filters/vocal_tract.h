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

#include "processor_router.h"
#include "circular_queue.h"
#include "synth_constants.h"

namespace vital {

  class VocalTract : public ProcessorRouter {
    public:
      enum {
        kAudio,
        kReset,
        kBlend,
        kTonguePosition,
        kTongueHeight,
        kNumInputs
      };

      VocalTract();
      virtual ~VocalTract();

      virtual Processor* clone() const override { return new VocalTract(*this); }

      void reset(poly_mask reset_mask) override;
      void hardReset() override;

      virtual void process(int num_samples) override;
      virtual void processWithInput(const poly_float* audio_in, int num_samples) override;

    private:
      JUCE_LEAK_DETECTOR(VocalTract)
  };
} // namespace vital

