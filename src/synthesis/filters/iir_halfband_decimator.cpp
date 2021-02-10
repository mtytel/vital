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

#include "iir_halfband_decimator.h"

namespace vital {
  poly_float IirHalfbandDecimator::kTaps9[kNumTaps9] = {
    { 0.167135116548925f, 0.0413554705262319f },
    { 0.742130012538075f, 0.3878932830211427f },
  };

  poly_float IirHalfbandDecimator::kTaps25[kNumTaps25] = {
    { 0.093022421467960f, 0.024388383731296f },
    { 0.312318050871736f, 0.194029987625265f },
    { 0.548379093159427f, 0.433855675727187f },
    { 0.737198546150414f, 0.650124972769370f },
    { 0.872234992057129f, 0.810418671775866f },
    { 0.975497791832324f, 0.925979700943193f }
  };

  IirHalfbandDecimator::IirHalfbandDecimator() : Processor(kNumInputs, 1), sharp_cutoff_(false) {
    reset(constants::kFullMask);
  }

  void IirHalfbandDecimator::process(int num_samples) {
    int num_taps = kNumTaps9;
    const poly_float* taps = kTaps9;
    if (sharp_cutoff_) {
      num_taps = kNumTaps25;
      taps = kTaps25;
    }

    const poly_float* audio = input(kAudio)->source->buffer;
    int output_buffer_size = num_samples;
    VITAL_ASSERT(input(kAudio)->source->buffer_size >= 2 * output_buffer_size);

    poly_float* audio_out = output()->buffer;
    for (int i = 0; i < output_buffer_size; ++i) {
      int audio_in_index = 2 * i;
      poly_float result = utils::consolidateAudio(audio[audio_in_index], audio[audio_in_index + 1]);
      for (int tap_index = 0; tap_index < num_taps; ++tap_index) {
        poly_float delta = result - out_memory_[tap_index];
        poly_float new_result = utils::mulAdd(in_memory_[tap_index], taps[tap_index], delta);
        in_memory_[tap_index] = result;
        out_memory_[tap_index] = new_result;
        result = new_result;
      }

      audio_out[i] = utils::sumSplitAudio(result) * 0.5f;
    }
  }

  void IirHalfbandDecimator::reset(poly_mask reset_mask) {
    for (int i = 0; i < kNumTaps25; ++i) {
      in_memory_[i] = 0.0f;
      out_memory_[i] = 0.0f;
    }
  }
} // namespace vital
