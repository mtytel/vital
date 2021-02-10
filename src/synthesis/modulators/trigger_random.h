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
#include "utils.h"

namespace vital {

  class TriggerRandom : public Processor {
    public:
      enum {
        kReset,
        kNumInputs
      };

      TriggerRandom();
      virtual ~TriggerRandom() { }

      virtual Processor* clone() const override { return new TriggerRandom(*this); }
      virtual void process(int num_samples) override;

    private:
      poly_float value_;
      utils::RandomGenerator random_generator_;

      JUCE_LEAK_DETECTOR(TriggerRandom)
  };
} // namespace vital

