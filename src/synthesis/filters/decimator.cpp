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

#include "decimator.h"

#include "iir_halfband_decimator.h"

namespace vital {
  Decimator::Decimator(int max_stages) : ProcessorRouter(kNumInputs, 1), max_stages_(max_stages) {
    num_stages_ = -1;
    for (int i = 0; i < max_stages_; ++i) {
      IirHalfbandDecimator* stage = new IirHalfbandDecimator();
      stage->setOversampleAmount(1 << (max_stages_ - i - 1));
      addProcessor(stage);
      stages_.push_back(stage);
    }
  }

  Decimator::~Decimator() { }

  void Decimator::init() {
    stages_[0]->useInput(input(kAudio));
    stages_[0]->useOutput(output());
    for (int i = 1; i < max_stages_; ++i) {
      stages_[i]->plug(stages_[i - 1], IirHalfbandDecimator::kAudio);
      stages_[i]->useOutput(output());
    }
  }

  void Decimator::reset(poly_mask reset_mask) {
    for (int i = 0; i < max_stages_; ++i)
      stages_[i]->reset(reset_mask);
  }

  void Decimator::process(int num_samples) {
    int num_stages = 0;
    if (input(kAudio)->source->owner) {
      int input_sample_rate = input(kAudio)->source->owner->getSampleRate();
      int output_sample_rate = getSampleRate();
      while(input_sample_rate > output_sample_rate) {
        num_stages++;
        input_sample_rate /= 2;
      }

      VITAL_ASSERT(num_stages <= max_stages_);
      VITAL_ASSERT(input_sample_rate == output_sample_rate);
    }

    if (num_stages == 0) {
      utils::copyBuffer(output()->buffer, input(kAudio)->source->buffer, num_samples);
      return;
    }

    if (num_stages != num_stages_) {
      for (int i = 0; i < num_stages; ++i)
        stages_[i]->reset(constants::kFullMask);

      num_stages_ = num_stages;

      for (int i = 0; i < max_stages_; ++i) {
        IirHalfbandDecimator* stage = stages_[i];
        bool should_enable = i < num_stages;
        stage->enable(should_enable);
        stage->setSharpCutoff(i == num_stages - 1);

        if (should_enable) {
          int oversample_amount = 1 << (num_stages - i - 1);
          stage->setOversampleAmount(oversample_amount);
        }
      }
    }

    ProcessorRouter::process(num_samples);
  }
} // namespace vital
