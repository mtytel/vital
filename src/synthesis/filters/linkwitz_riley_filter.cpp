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

#include "linkwitz_riley_filter.h"

namespace vital {

  LinkwitzRileyFilter::LinkwitzRileyFilter(mono_float cutoff) : Processor(kNumInputs, kNumOutputs) {
    cutoff_ = cutoff;
    low_in_0_ = low_in_1_ = low_in_2_ = 0.0f;
    low_out_1_ = low_out_2_ = 0.0f;
    high_in_0_ = high_in_1_ = high_in_2_ = 0.0f;
    high_out_1_ = high_out_2_ = 0.0f;
    reset(constants::kFullMask);
  }

  void LinkwitzRileyFilter::process(int num_samples) {
    processWithInput(input(kAudio)->source->buffer, num_samples);
  }

  void LinkwitzRileyFilter::processWithInput(const poly_float* audio_in, int num_samples) {
    poly_float* dest_low = output(kAudioLow)->buffer;
    for (int i = 0; i < num_samples; ++i) {
      poly_float audio = audio_in[i];

      poly_float low_in01 = utils::mulAdd(audio * low_in_0_, past_in_1a_[kAudioLow], low_in_1_);
      poly_float low_in = utils::mulAdd(low_in01, past_in_2a_[kAudioLow], low_in_2_);
      poly_float low_in_out1 = utils::mulAdd(low_in, past_out_1a_[kAudioLow], low_out_1_);
      poly_float low = utils::mulAdd(low_in_out1, past_out_2a_[kAudioLow], low_out_2_);

      past_in_2a_[kAudioLow] = past_in_1a_[kAudioLow];
      past_in_1a_[kAudioLow] = audio;
      past_out_2a_[kAudioLow] = past_out_1a_[kAudioLow];
      past_out_1a_[kAudioLow] = low;
      dest_low[i] = low;
    }

    for (int i = 0; i < num_samples; ++i) {
      poly_float audio = dest_low[i];

      poly_float low_in01 = utils::mulAdd(audio * low_in_0_, past_in_1b_[kAudioLow], low_in_1_);
      poly_float low_in = utils::mulAdd(low_in01, past_in_2b_[kAudioLow], low_in_2_);
      poly_float low_in_out1 = utils::mulAdd(low_in, past_out_1b_[kAudioLow], low_out_1_);
      poly_float low = utils::mulAdd(low_in_out1, past_out_2b_[kAudioLow], low_out_2_);

      past_in_2b_[kAudioLow] = past_in_1b_[kAudioLow];
      past_in_1b_[kAudioLow] = audio;
      past_out_2b_[kAudioLow] = past_out_1b_[kAudioLow];
      past_out_1b_[kAudioLow] = low;
      dest_low[i] = low;
    }

    poly_float* dest_high = output(kAudioHigh)->buffer;
    for (int i = 0; i < num_samples; ++i) {
      poly_float audio = audio_in[i];
      poly_float high_in01 = utils::mulAdd(audio * high_in_0_, past_in_1a_[kAudioHigh], high_in_1_);
      poly_float high_in = utils::mulAdd(high_in01, past_in_2a_[kAudioHigh], high_in_2_);
      poly_float high_in_out1 = utils::mulAdd(high_in, past_out_1a_[kAudioHigh], high_out_1_);
      poly_float high = utils::mulAdd(high_in_out1, past_out_2a_[kAudioHigh], high_out_2_);

      past_in_2a_[kAudioHigh] = past_in_1a_[kAudioHigh];
      past_in_1a_[kAudioHigh] = audio;
      past_out_2a_[kAudioHigh] = past_out_1a_[kAudioHigh];
      past_out_1a_[kAudioHigh] = high;
      dest_high[i] = high;
    }

    for (int i = 0; i < num_samples; ++i) {
      poly_float audio = dest_high[i];
      poly_float high_in01 = utils::mulAdd(audio * high_in_0_, past_in_1b_[kAudioHigh], high_in_1_);
      poly_float high_in = utils::mulAdd(high_in01, past_in_2b_[kAudioHigh], high_in_2_);
      poly_float high_in_out1 = utils::mulAdd(high_in, past_out_1b_[kAudioHigh], high_out_1_);
      poly_float high = utils::mulAdd(high_in_out1, past_out_2b_[kAudioHigh], high_out_2_);

      past_in_2b_[kAudioHigh] = past_in_1b_[kAudioHigh];
      past_in_1b_[kAudioHigh] = audio;
      past_out_2b_[kAudioHigh] = past_out_1b_[kAudioHigh];
      past_out_1b_[kAudioHigh] = high;
      dest_high[i] = high;
    }
  }

  void LinkwitzRileyFilter::computeCoefficients() {
    mono_float warp = 1.0f / tanf(kPi * cutoff_ / getSampleRate());
    mono_float warp2 = warp * warp;
    mono_float mult = 1.0f / (1.0f + kSqrt2 * warp + warp2);

    low_in_0_ = mult;
    low_in_1_ = 2.0f * mult;
    low_in_2_ = mult;
    low_out_1_ = -2.0f * (1.0f - warp2) * mult;
    low_out_2_ = -(1.0f - kSqrt2 * warp + warp2) * mult;

    high_in_0_ = warp2 * mult;
    high_in_1_ = -2.0f * high_in_0_;
    high_in_2_ = high_in_0_;
    high_out_1_ = low_out_1_;
    high_out_2_ = low_out_2_;
  }

  void LinkwitzRileyFilter::setSampleRate(int sample_rate) {
    Processor::setSampleRate(sample_rate);
    computeCoefficients();
  }

  void LinkwitzRileyFilter::setOversampleAmount(int oversample_amount) {
    Processor::setOversampleAmount(oversample_amount);
    computeCoefficients();
  }

  void LinkwitzRileyFilter::reset(poly_mask reset_mask) {
    for (int i = 0; i < kNumOutputs; ++i) {
      past_in_1a_[i] = utils::maskLoad(past_in_1a_[i], 0.0f, reset_mask);
      past_in_2a_[i] = utils::maskLoad(past_in_2a_[i], 0.0f, reset_mask);
      past_out_1a_[i] = utils::maskLoad(past_out_1a_[i], 0.0f, reset_mask);
      past_out_2a_[i] = utils::maskLoad(past_out_2a_[i], 0.0f, reset_mask);
      past_in_1b_[i] = utils::maskLoad(past_in_1b_[i], 0.0f, reset_mask);
      past_in_2b_[i] = utils::maskLoad(past_in_2b_[i], 0.0f, reset_mask);
      past_out_1b_[i] = utils::maskLoad(past_out_1b_[i], 0.0f, reset_mask);
      past_out_2b_[i] = utils::maskLoad(past_out_2b_[i], 0.0f, reset_mask);
    }
  }
} // namespace vital
