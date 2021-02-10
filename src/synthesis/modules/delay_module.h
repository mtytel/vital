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

#include "synth_constants.h"
#include "synth_module.h"

#include "delay.h"

namespace vital {

  class DelayModule : public SynthModule {
    public:
      static constexpr mono_float kMaxDelayTime = 4.0f;
    
      DelayModule(const Output* beats_per_second);
      virtual ~DelayModule();

      virtual void init() override;
      virtual void hardReset() override { delay_->hardReset(); }
      virtual void enable(bool enable) override {
        SynthModule::enable(enable);
        process(1);
        if (!enable)
          delay_->hardReset();
      }
      virtual void setSampleRate(int sample_rate) override;
      virtual void setOversampleAmount(int oversample) override;
      virtual void processWithInput(const poly_float* audio_in, int num_samples) override;
      virtual Processor* clone() const override { return new DelayModule(*this); }
    
    protected:
      const Output* beats_per_second_;
      StereoDelay* delay_;

      JUCE_LEAK_DETECTOR(DelayModule)
  };
} // namespace vital

