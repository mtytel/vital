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

#include "fir_halfband_decimator.h"
#include "poly_utils.h"

namespace vital {
  FirHalfbandDecimator::FirHalfbandDecimator() : Processor(kNumInputs, 1) {
    static const mono_float coefficients[kNumTaps] = {
      0.000088228877315364f,
      0.000487010018128278f,
      0.000852264975437944f,
      -0.001283563593466774f,
      -0.010130591831925894f,
      -0.025688727779244691f,
      -0.036346596505004387f,
      -0.024088355516718698f,
      0.012246773417129486f,
      0.040021434054637831f,
      0.017771298164062477f,
      -0.046866403416502632f,
      -0.075597513455990611f,
      0.013331126342402619f,
      0.202889888191404910f,
      0.362615173769444080f,
      0.362615173769444080f,
      0.202889888191404910f,
      0.013331126342402619f,
      -0.075597513455990611f,
      -0.046866403416502632f,
      0.017771298164062477f,
      0.040021434054637831f,
      0.012246773417129486f,
      -0.024088355516718698f,
      -0.036346596505004387f,
      -0.025688727779244691f,
      -0.010130591831925894f,
      -0.001283563593466774f,
      0.000852264975437944f,
      0.000487010018128278f,
      0.000088228877315364f,
    };

    for (int i = 0; i < kNumTaps / 2; ++i)
      taps_[i] = poly_float(coefficients[2 * i], coefficients[2 * i + 1]);
    reset(constants::kFullMask);
  }

  void FirHalfbandDecimator::saveMemory(int num_samples) {
    int input_buffer_size = 2 * num_samples;
    poly_float* audio = input(kAudio)->source->buffer;

    int start_audio_index = input_buffer_size - kNumTaps + 2;
    for (int i = 0; i < kNumTaps / 2 - 1; ++i) {
      int audio_index = start_audio_index + 2 * i;
      memory_[i] = utils::consolidateAudio(audio[audio_index], audio[audio_index + 1]);
    }
  }

  void FirHalfbandDecimator::process(int num_samples) {
    const poly_float* audio = input(kAudio)->source->buffer;
    int output_buffer_size = num_samples;
    VITAL_ASSERT(output_buffer_size > kNumTaps / 2);
    VITAL_ASSERT(input(kAudio)->source->buffer_size >= 2 * output_buffer_size);

    poly_float* audio_out = output()->buffer;
    for (int memory_start = 0; memory_start < kNumTaps / 2 - 1; ++memory_start) {
      poly_float sum = 0.0f;

      int tap_index = 0;
      int num_memory = kNumTaps / 2 - memory_start - 1;
      for (; tap_index < num_memory; ++tap_index)
        sum = utils::mulAdd(sum, memory_[tap_index + memory_start], taps_[tap_index]);

      int audio_index = 0;
      for (; tap_index < kNumTaps / 2; ++tap_index) {
        poly_float consolidated = utils::consolidateAudio(audio[audio_index], audio[audio_index + 1]);
        sum = utils::mulAdd(sum, consolidated, taps_[tap_index]);
        audio_index += 2;
      }

      audio_out[memory_start] = utils::sumSplitAudio(sum);
    }

    int out_index = kNumTaps / 2 - 1;
    int audio_start = 0;
    for (; out_index < output_buffer_size; ++out_index) {
      poly_float sum = 0.0f;
      int audio_index = audio_start;
      for (int tap_index = 0; tap_index < kNumTaps / 2; ++tap_index) {
        poly_float consolidated = utils::consolidateAudio(audio[audio_index], audio[audio_index + 1]);
        sum = utils::mulAdd(sum, consolidated, taps_[tap_index]);
        audio_index += 2;
      }
      audio_start += 2;

      audio_out[out_index] = utils::sumSplitAudio(sum);
    }

    saveMemory(num_samples);
  }

  void FirHalfbandDecimator::reset(poly_mask reset_mask) {
    for (int i = 0; i < kNumTaps / 2 - 1; ++i)
      memory_[i] = 0.0f;
  }
} // namespace vital
