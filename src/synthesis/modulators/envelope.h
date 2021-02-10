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

  class Envelope : public Processor {
    public:
      enum {
        kDelay,
        kAttack,
        kAttackPower,
        kHold,
        kDecay,
        kDecayPower,
        kSustain,
        kRelease,
        kReleasePower,
        kTrigger,
        kNumInputs
      };

      enum ProcessorOutput {
        kValue,
        kPhase,
        kNumOutputs
      };

      Envelope();
      virtual ~Envelope() { }

      virtual Processor* clone() const override { return new Envelope(*this); }
      virtual void process(int num_samples) override;

    private:
      void processControlRate(int num_samples);
      void processAudioRate(int num_samples);

      poly_float processSection(poly_float* audio_out, int from, int to,
                                poly_float power, poly_float delta_power,
                                poly_float position, poly_float delta_position,
                                poly_float start, poly_float end, poly_float delta_end);

      poly_float current_value_;

      poly_float position_;
      poly_float value_;
      poly_float poly_state_;
      poly_float start_value_;

      poly_float attack_power_;
      poly_float decay_power_;
      poly_float release_power_;
      poly_float sustain_;

      JUCE_LEAK_DETECTOR(Envelope)
  };
} // namespace vital

