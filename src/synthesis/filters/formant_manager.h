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

  class DigitalSvf;

  class FormantManager : public ProcessorRouter {
    public:
      static constexpr mono_float kMinResonance = 4.0f;
      static constexpr mono_float kMaxResonance = 30.0f;

      FormantManager(int num_formants = 4);
      virtual ~FormantManager() { }

      virtual void init() override;
      void reset(poly_mask reset_mask) override;
      void hardReset() override;

      virtual Processor* clone() const override {
        return new FormantManager(*this);
      }

      DigitalSvf* getFormant(int index = 0) { return formants_[index]; }
      int numFormants() { return static_cast<int>(formants_.size()); }

    protected:
      std::vector<DigitalSvf*> formants_;

      JUCE_LEAK_DETECTOR(FormantManager)
  };
} // namespace vital

