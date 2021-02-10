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
#include "synth_constants.h"

namespace vital {

  class IirHalfbandDecimator;

  class Decimator : public ProcessorRouter {
    public:
      enum {
        kAudio,
        kNumInputs
      };

      Decimator(int max_stages = 1);
      virtual ~Decimator();

      void init() override;
      void reset(poly_mask reset_mask) override;

      virtual Processor* clone() const override { VITAL_ASSERT(false); return nullptr; }

      virtual void process(int num_samples) override;
      virtual void setOversampleAmount(int) override { }

    private:
      int num_stages_;
      int max_stages_;

      std::vector<IirHalfbandDecimator*> stages_;

      JUCE_LEAK_DETECTOR(Decimator)
  };
} // namespace vital

