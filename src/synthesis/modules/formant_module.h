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

#include "synth_module.h"
#include "formant_filter.h"

namespace vital {
  class FormantModule : public SynthModule {
    public:
      enum {
        kAudio,
        kReset,
        kResonance,
        kBlend,
        kStyle,
        kNumInputs
      };

      FormantModule(std::string prefix = "");
      virtual ~FormantModule() { }

      Output* createModControl(std::string name, bool audio_rate = false, bool smooth_value = false);

      void init() override;
      void process(int num_samples) override;
      void reset(poly_mask reset_mask) override;
      void hardReset() override;
      void setMono(bool mono) { mono_ = mono; }
      virtual Processor* clone() const override { return new FormantModule(*this); }

    protected:
      void setStyle(int new_style);

      std::string prefix_;

      Processor* formant_filters_[FormantFilter::kTotalFormantFilters];
      int last_style_;
      bool mono_;

      JUCE_LEAK_DETECTOR(FormantModule)
  };
} // namespace vital

