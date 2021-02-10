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
#include "sample_source.h"

namespace vital {
  class SampleModule : public SynthModule {
    public:
      enum {
        kReset,
        kMidi,
        kNoteCount,
        kNumInputs
      };

      enum {
        kRaw,
        kLevelled,
        kNumOutputs
      };

      SampleModule();
      virtual ~SampleModule() { }

      void process(int num_samples) override;
      void init() override;
      virtual Processor* clone() const override { return new SampleModule(*this); }

      Sample* getSample() { return sampler_->getSample(); }
      force_inline Output* getPhaseOutput() const { return sampler_->getPhaseOutput(); }

    protected:
      std::shared_ptr<bool> was_on_;
      SampleSource* sampler_;
      Value* on_;

      JUCE_LEAK_DETECTOR(SampleModule)
  };
} // namespace vital

