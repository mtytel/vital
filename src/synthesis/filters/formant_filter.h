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

#include "synth_filter.h"

#include "formant_manager.h"

namespace vital {
  class DigitalSvf;

  class FormantFilter : public ProcessorRouter, public SynthFilter {
    public:
      enum FormantStyle {
        kAOIE,
        kAIUO,
        kNumFormantStyles,
        kVocalTract = kNumFormantStyles,
        kTotalFormantFilters
      };

      static constexpr float kCenterMidi = 80.0f;

      FormantFilter(int style = 0);
      virtual ~FormantFilter() { }
      
      void reset(poly_mask reset_mask) override;
      void hardReset() override;
      void init() override;

      virtual Processor* clone() const override { return new FormantFilter(*this); }

      void setupFilter(const FilterState& filter_state) override;

      DigitalSvf* getFormant(int index) { return formant_manager_->getFormant(index); }

    protected:
      FormantManager* formant_manager_;
      int style_;

      JUCE_LEAK_DETECTOR(FormantFilter)
  };
} // namespace vital

