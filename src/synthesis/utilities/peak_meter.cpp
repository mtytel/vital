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

#include "peak_meter.h"
#include "utils.h"
#include "synth_constants.h"

namespace vital {

  namespace {
    constexpr mono_float kSampleDecay = 8096.0f;
    constexpr mono_float kRememberedDecay = 20000.0f;
    constexpr mono_float kRememberedHold = 50000.0f;
  } // namespace

  PeakMeter::PeakMeter() : Processor(1, 2), current_peak_(0.0f), current_square_sum_(0.0f),
                           remembered_peak_(0.0f), samples_since_remembered_(0) { }

  void PeakMeter::process(int num_samples) {
    const poly_float* audio_in = input(0)->source->buffer;
    poly_float peak = utils::peak(audio_in, num_samples);

    mono_float samples = getOversampleAmount() * kSampleDecay;
    mono_float mult = (samples - 1.0f) / samples;
    poly_float current_peak = current_peak_;

    mono_float remembered_samples = getOversampleAmount() * kRememberedDecay;
    mono_float remembered_mult = (remembered_samples - 1.0f) / remembered_samples;
    poly_float current_remembered_peak = remembered_peak_;

    poly_float current_square_sum = current_square_sum_;

    for (int i = 0; i < num_samples; ++i) {
      current_peak *= mult;
      current_remembered_peak *= remembered_mult;
      current_square_sum *= mult;
      poly_float sample = audio_in[i];
      current_square_sum += sample * sample;
    }

    current_peak_ = utils::max(current_peak, peak);
    samples_since_remembered_ += num_samples;
    samples_since_remembered_ &= poly_float::lessThan(current_peak_, current_remembered_peak);

    mono_float remembered_hold_samples = getOversampleAmount() * kRememberedHold;
    poly_mask hold_mask = poly_int::lessThan(samples_since_remembered_, remembered_hold_samples);
    current_remembered_peak = utils::maskLoad(current_remembered_peak, remembered_peak_, hold_mask);
    remembered_peak_ = utils::max(current_peak_, current_remembered_peak);
    current_square_sum_ = current_square_sum;

    poly_float rms = utils::sqrt(current_square_sum_ * (1.0f / samples));
    poly_float prepped_rms = utils::swapVoices(rms);
    output(kLevel)->buffer[0] = utils::maskLoad(prepped_rms, current_peak_, constants::kFirstMask);
    output(kMemoryPeak)->buffer[0] = remembered_peak_;
  }
} // namespace vital
