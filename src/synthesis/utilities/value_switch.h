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

#include "value.h"

namespace vital {

  class ValueSwitch : public cr::Value {
    public:
      enum {
        kValue,
        kSwitch,
        kNumOutputs
      };

      ValueSwitch(mono_float value = 0.0f);

      virtual Processor* clone() const override { return new ValueSwitch(*this); }
      virtual void process(int num_samples) override { }
      virtual void set(poly_float value) override;

      void addProcessor(Processor* processor) { processors_.push_back(processor); }

      virtual void setOversampleAmount(int oversample) override;

    private:
      void setBuffer(int source);
      void setSource(int source);

      std::vector<Processor*> processors_;

      JUCE_LEAK_DETECTOR(ValueSwitch)
  };
} // namespace vital

